#include "coreplugin.hpp"
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

} // namespace

CorePlugin::CorePlugin(const Descriptor* descriptor, const clap_host* host)
    : mDescriptor(descriptor)
    , mIdHash(toHash(this))
    , dPtr(std::make_unique<CorePluginPrivate>(host))
{
    DLOG(INFO) << "Created plugin with id: " << mIdHash;
    mPlugin.desc = *mDescriptor;
    mPlugin.plugin_data = this;
    mPlugin.init = [](const clap_plugin* plugin) {
        return self(plugin)->init();
    };
    mPlugin.destroy = [](const struct clap_plugin* plugin) {
        auto* p = self(plugin);
        p->destroy();
        if (!PluginInstances::destroy(p->mDescriptor->id(), p))
            DLOG(INFO) << std::format(
                "Failed to destroy instance {}", p->mDescriptor->id()
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
        return self(plugin)->process(process);
    };
    mPlugin.get_extension = [](const clap_plugin* /* plugin */,
                               const char* /* id */) -> const void* {
        return nullptr;
    };
    mPlugin.on_main_thread = [](const clap_plugin* /* plugin */) {
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

void CorePlugin::destroy()
{
}

bool CorePlugin::activate()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEvent::Activate);
    pushEvent(msg);
    return true;
}

void CorePlugin::deactivate()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEvent::Deactivate);
    pushEvent(msg);
}

bool CorePlugin::startProcessing()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEvent::StartProcessing);
    pushEvent(msg);
    return true;
}

void CorePlugin::stopProcessing()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEvent::StopProcessing);
    pushEvent(msg);
}

void CorePlugin::reset()
{
    api::PluginEventMessage msg;
    msg.set_event(api::PluginEvent::Reset);
    pushEvent(msg);
};

clap_process_status CorePlugin::process(const clap_process_t* /* process */)
{
    return CLAP_PROCESS_CONTINUE;
};

bool CorePlugin::pushEvent(const api::PluginEventMessage& event)
{
    if (!dPtr->pluginToClientsQueue().push(event))
        return false;
    return dPtr->notifyPluginQueueReady();
}

// Getters

uint64_t CorePlugin::idHash() const noexcept
{
    return mIdHash;
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

CLAP_RCI_END_NAMESPACE
