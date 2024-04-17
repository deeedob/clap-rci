#include "coreplugin_p.hpp"
#include <clap-rci/coreplugin.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>

#include <format>

CLAP_RCI_BEGIN_NAMESPACE

namespace {

CorePlugin* self(const clap_plugin* plugin)
{
    return static_cast<CorePlugin*>(plugin->plugin_data);
}

// MurmurHash3
// Since (*this) is already unique in this process, all we really want to do is
// propagate that uniqueness evenly across all the bits, so that we can use
// a subset of the bits while reducing collisions significantly.
std::uint64_t toHash(void* ptr)
{
    uint64_t h = reinterpret_cast<uint64_t>(ptr); // NOLINT
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    return h ^ (h >> 33);
}

void updateEventMessage(
    const clap_event_note* in, api::Note* out, api::Note::Type type
)
{
    out->set_type(type);
    out->set_noteid(in->note_id);
    out->set_portindex(in->port_index);
    out->set_channel(in->channel);
    out->set_key(in->key);
    out->set_velocity(in->velocity);
}

void updateEventMessage(const clap_event_midi* in, api::Midi* out)
{
    out->set_port_index(in->port_index);
    out->set_data(in->data, 3);
}

void updateEventMessage(const clap_event_midi_sysex* in, api::Midi* out)
{
    out->set_port_index(in->port_index);
    out->set_data(in->buffer, in->size);
}

void updateEventMessage(const clap_event_midi2* in, api::Midi* out)
{
    out->set_port_index(in->port_index);
    out->set_data(in->data, 4);
}

} // namespace

CorePlugin::CorePlugin(const Descriptor* descriptor, const clap_host* host)
    : mPluginId(toHash(this))
    , mDescriptor(descriptor)
    , dPtr(CorePluginPrivate::create(host))
{
    DLOG(INFO) << "Created plugin with id: " << mPluginId;
    mPlugin.desc = *mDescriptor;
    mPlugin.plugin_data = this;
    mPlugin.init = [](const clap_plugin* plugin) {
        return self(plugin)->init();
    };
    mPlugin.destroy = [](const struct clap_plugin* plugin) {
        auto* p = self(plugin);
        p->destroy();
        p->dPtr->cancelAllClients();
        // self destroy. This will invoke all destructors
        if (!Registry::Instances::destroy(p->mDescriptor->id(), p))
            LOG(ERROR) << std::format(
                "Failed to destroy instance: id {}", p->mDescriptor->id()
            );
    };
    mPlugin.activate = [](const clap_plugin* plugin, double sampleRate,
                          uint32_t minFramesCount, uint32_t maxFramesCount) {
        self(plugin)->mSampleRate = sampleRate;
        self(plugin)->mMinFramesCount = minFramesCount;
        self(plugin)->mMaxFramesCount = maxFramesCount;
        self(plugin)->mIsActive = true;
        return self(plugin)->activate();
    };
    mPlugin.deactivate = [](const clap_plugin* plugin) {
        self(plugin)->mIsActive = false;
        return self(plugin)->deactivate();
    };
    mPlugin.start_processing = [](const clap_plugin* plugin) {
        self(plugin)->mIsProcessing = true;

        return self(plugin)->startProcessing();
    };
    mPlugin.stop_processing = [](const clap_plugin* plugin) {
        self(plugin)->mIsProcessing = false;
        return self(plugin)->stopProcessing();
    };
    mPlugin.reset = [](const clap_plugin* plugin) {
        return self(plugin)->reset();
    };
    mPlugin.process = [](const clap_plugin* plugin,
                         const clap_process* process) {
        auto* s = self(plugin);
        const auto* inEvents = process->in_events;
        uint32_t eventSize = inEvents->size(inEvents);
        if (process->transport && s->dPtr->mWantsTransport) {
            if (s->dPtr->mTransportWatcher.update(*process->transport)) {
                s->pushEvent(s->dPtr->mTransportWatcher.message());
            }
        }
        api::PluginEventMessage outMsg;
        for (uint32_t i = 0; i < eventSize; ++i) {
            const clap_event_header* ev = inEvents->get(inEvents, i);
            if (ev->space_id != CLAP_CORE_EVENT_SPACE_ID)
                continue;

            switch (ev->type) {
                case CLAP_EVENT_TRANSPORT: {
                    DLOG(INFO) << "transport";
                    const auto* note = reinterpret_cast<
                        const clap_event_transport*>(ev);
                    (void)note;
                } break;

                case CLAP_EVENT_NOTE_ON: {
                    DLOG(INFO) << "note on";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_note*>(ev),
                        outMsg.mutable_note(), api::Note::NoteOn
                    );
                    self(plugin)->pushEvent(outMsg);
                } break;
                case CLAP_EVENT_NOTE_OFF: {
                    DLOG(INFO) << "note off";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_note*>(ev),
                        outMsg.mutable_note(), api::Note::NoteOff
                    );
                    self(plugin)->pushEvent(outMsg);
                } break;
                case CLAP_EVENT_NOTE_CHOKE: {
                    DLOG(INFO) << "note choke";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_note*>(ev),
                        outMsg.mutable_note(), api::Note::NoteChoke
                    );
                    self(plugin)->pushEvent(outMsg);
                } break;
                case CLAP_EVENT_NOTE_END: {
                    DLOG(INFO) << "note end";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_note*>(ev),
                        outMsg.mutable_note(), api::Note::NoteEnd
                    );
                    self(plugin)->pushEvent(outMsg);
                } break;
                case CLAP_EVENT_NOTE_EXPRESSION: {
                    DLOG(INFO) << "note expression";
                    const auto* note = reinterpret_cast<
                        const clap_event_note_expression*>(ev);
                    (void)note;
                } break;
                case CLAP_EVENT_MIDI: {
                    DLOG(INFO) << "midi";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_midi*>(ev),
                        outMsg.mutable_midi()
                    );
                } break;
                case CLAP_EVENT_MIDI_SYSEX: {
                    DLOG(INFO) << "midi sysex";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_midi_sysex*>(ev),
                        outMsg.mutable_midi()
                    );
                } break;
                case CLAP_EVENT_MIDI2: {
                    DLOG(INFO) << "midi2";
                    updateEventMessage(
                        reinterpret_cast<const clap_event_midi2*>(ev),
                        outMsg.mutable_midi()
                    );
                } break;

                default:
                    break;
            }
            // outMsg.Clear();
        }
        return self(plugin)->process(process);
    };
    mPlugin.get_extension = [](const clap_plugin* plugin,
                               const char* id) -> const void* {
        const auto* s = self(plugin);
        if (!strcmp(id, CLAP_EXT_NOTE_PORTS) && s->isExtNotePortsEnabled()) {
            return &s->mPluginNotePorts;
        }
        return nullptr;
    };
    mPlugin.on_main_thread = [](const clap_plugin* /* plugin */) {
    };

    mPluginNotePorts.count = [](const clap_plugin* plugin,
                                bool isInput) -> uint32_t {
        if (isInput) {
            return static_cast<uint32_t>(
                self(plugin)->dPtr->mNotePortInfosIn.size()
            );
        }
        return static_cast<uint32_t>(self(plugin)->dPtr->mNotePortInfosOut.size(
        ));
    };
    mPluginNotePorts.get = [](const clap_plugin* plugin, uint32_t index,
                              bool isInput, clap_note_port_info* info) -> bool {
        const auto* s = self(plugin);
        auto& container = s->dPtr->mNotePortInfosOut;
        if (isInput)
            container = s->dPtr->mNotePortInfosIn;
        if (index >= container.size())
            return false;
        info = &container[index];
        return true;
    };
}

CorePlugin::~CorePlugin() = default;

void CorePlugin::hostRequestRestart()
{
    dPtr->hostRequestRestart();
}

void CorePlugin::hostRequestProcess()
{
    dPtr->hostRequestProcess();
}

bool CorePlugin::init()
{
    return true;
};

void CorePlugin::destroy() { }

bool CorePlugin::activate()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEventMessage::Activate);
    pushEvent(msg);
    return true;
}

void CorePlugin::deactivate()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEventMessage::Deactivate);
    pushEvent(msg);
}

bool CorePlugin::startProcessing()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEventMessage::StartProcessing);
    pushEvent(msg);
    return true;
}

void CorePlugin::stopProcessing()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEventMessage::StopProcessing);
    pushEvent(msg);
}

void CorePlugin::reset()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEventMessage::Reset);
    pushEvent(msg);
};

clap_process_status CorePlugin::process(const clap_process_t* /* process */)
{
    return CLAP_PROCESS_CONTINUE;
};

void CorePlugin::withWantsTransport(bool value)
{
    dPtr->mWantsTransport = value;
}

bool CorePlugin::wantsTransport() const noexcept
{
    return dPtr->mWantsTransport;
}

bool CorePlugin::withNotePortInfoIn(clap_note_port_info in)
{
    if (mIsActive)
        return false;
    dPtr->mNotePortInfosIn.push_back(in);
    return true;
}

bool CorePlugin::withNotePortInfoOut(clap_note_port_info out)
{
    if (mIsActive)
        return false;
    dPtr->mNotePortInfosOut.push_back(out);
    return true;
}

bool CorePlugin::pushEvent(const api::PluginEventMessage& event)
{
    if (dPtr->mClients.empty())
        return false;
    if (!dPtr->pluginToClientsQueue().push(event))
        return false;
    return dPtr->notifyPluginQueueReady();
}

// Getters

uint64_t CorePlugin::pluginId() const noexcept
{
    return mPluginId;
}

bool CorePlugin::isActive() const noexcept
{
    return mIsActive;
}

bool CorePlugin::isProcessing() const noexcept
{
    return mIsProcessing;
}

double CorePlugin::sampleRate() const noexcept
{
    return mSampleRate;
}

uint32_t CorePlugin::minFramesCount() const noexcept
{
    return mMinFramesCount;
}

uint32_t CorePlugin::maxFramesCount() const noexcept
{
    return mMaxFramesCount;
}

const clap_plugin* CorePlugin::clapPlugin() const noexcept
{
    return &mPlugin;
}

CLAP_RCI_END_NAMESPACE
