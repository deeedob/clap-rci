#include "coreplugin.h"
#include "context.h"
#include "modules/module.h"
#include "parameter/parameter.h"
#include "spdlog/spdlog.h"

#include <core/logging.h>
#include <core/processhandle.h>

#include <server/serverctrl.h>
#include <server/shareddata.h>
#include <server/wrappers.h>

#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hxx>
#include <spdlog/fmt/bundled/core.h>
#include <crill/progressive_backoff_wait.h>

#include <string>
#include <chrono>

// NOLINTBEGIN
template class clap::helpers::Plugin<
    clap::helpers::MisbehaviourHandler::Terminate,
    clap::helpers::CheckingLevel::Maximal
>;
template class clap::helpers::HostProxy<
    clap::helpers::MisbehaviourHandler::Terminate,
    clap::helpers::CheckingLevel::Maximal
>;
// NOLINTEND

RCLAP_BEGIN_NAMESPACE

struct CorePluginPrivate
{
    CorePluginPrivate() = default;
    explicit CorePluginPrivate(const clap_plugin_descriptor *desc, std::unique_ptr<Module> rootModule)
        : desc(desc), rootModule(std::move(rootModule)) {}

    // #### Identification / SharedData ####
    const clap_plugin_descriptor *desc = nullptr;
    uint64_t hashCore {};
    std::shared_ptr<SharedData> sharedData {};

    // #### Client Main Module ####
    std::unique_ptr<Module> rootModule;

    // #### Processing ####
    Context context;
    std::vector<clap_audio_port_info> audioPortsInfoIn;
    std::vector<clap_audio_port_info> audioPortsInfoOut;
    std::vector<clap_note_port_info> notePortsInfoIn;
    std::vector<clap_note_port_info> notePortsInfoOut;

    // #### GUI ####
    std::unique_ptr<ProcessHandle> guiProc;

    // #### PARAMS ####
    std::vector<std::unique_ptr<Parameter>> params;
    std::unordered_map<clap_id, Parameter *> paramsHashed; // For fast access from cookies.

    // #### UTILITY ####
    std::unique_ptr<Settings> settings;
    std::shared_ptr<spdlog::logger> logger;
};


CorePlugin::CorePlugin(const clap_plugin_descriptor *desc, const clap_host *host)
    : Plugin(desc, host), dPtr(std::make_unique<CorePluginPrivate>())
{}

CorePlugin::CorePlugin(const Settings &settings, const clap_plugin_descriptor *desc, const clap_host *host, std::unique_ptr<Module> rootModule)
    : Plugin(desc, host), dPtr(std::make_unique<CorePluginPrivate>(desc, std::move(rootModule)))
{
    // Consume the settings from the caller.
    dPtr->settings = std::make_unique<Settings>(settings);

    // Start global logger if not already started
    const auto lf = dPtr->settings->logFile();
    if (!Log::Private::gLoggerSetup && lf)
        dPtr->logger = Log::setupLogger(*lf);

    // Identify this instance with the Server by creating a hash that is used as a global key.
    // Since ServerCtrl is a static instance, this can potentially hold multiple plugins
    // if the DSO and all its instances are loaded into the same process' address space.
    // Since a pointer is unique across the process address space, we use it as a hash seed.
    auto hc = ServerCtrl::instance().addPlugin(this);
    assert(hc);
    dPtr->hashCore = *hc;
    logInfo();
    // The shared data is used as a pipeline for this plugin instance to communicate with
    // the server and its clients
    dPtr->sharedData = ServerCtrl::instance().getSharedData(dPtr->hashCore);

    if (ServerCtrl::instance().start())
        SPDLOG_INFO("Server Instance started");
    assert(ServerCtrl::instance().isRunning());

    dPtr->rootModule->init();
}

CorePlugin::~CorePlugin()
{
    try {
        ServerCtrl::instance().removePlugin(dPtr->hashCore);
        if (!ServerCtrl::instance().stop())
            SPDLOG_DEBUG("Could not stop the server, connected plugins: {}", ServerCtrl::instance().nPlugins());
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception whilst shutting down: {}", e.what());
    }
}

// #### AUDIO PROCESSING #####
// [[ Main Thread ]]
bool CorePlugin::init() noexcept
{
    return true;
}
// [[ Main Thread & !active_state ]]
bool CorePlugin::activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
{
    dPtr->context.setSampleRate(sampleRate);
    dPtr->rootModule->activate();
    pushToMainQueue({Event::PluginActivate, ClapEventMainSyncWrapper{}});
    return true;
}
// [[ Main Thread & active_state ]]
void CorePlugin::deactivate() noexcept
{
    dPtr->rootModule->deactivate();
    pushToMainQueue({Event::PluginDeactivate, ClapEventMainSyncWrapper{}});
}
// [[ Audio Thread & active_state & !processing_state ]]
bool CorePlugin::startProcessing() noexcept
{
    dPtr->rootModule->startProcessing();
    pushToMainQueue({Event::PluginStartProcessing, ClapEventMainSyncWrapper{}});
    return true;
}

// [[ Audio Thread & active_state & processing_state ]]
void CorePlugin::stopProcessing() noexcept
{
    dPtr->rootModule->stopProcessing();
    pushToMainQueue({Event::PluginStopProcessing, ClapEventMainSyncWrapper{}});
}

void CorePlugin::reset() noexcept
{
    dPtr->rootModule->reset();
    pushToMainQueue({Event::PluginReset, ClapEventMainSyncWrapper{}});
}

void CorePlugin::processGuiEvents(const clap_output_events *ov)
{
    ClientParamWrapper clientEv;
    while (dPtr->sharedData->clientsToPluginQueue().pop(clientEv)) {
        switch (clientEv.ev) {
            case Param: {
                auto *param = getParameterById(clientEv.paramId);
                if (!param) {
                    SPDLOG_ERROR("Event process: parameter not found");
                    break;
                }
                SPDLOG_TRACE("Event process: parameter {} value {}", clientEv.paramId, clientEv.value);
                param->setValue(clientEv.value);
                clap_event_param_value ev;
                ev.header.time = 0;
                ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                ev.header.type = CLAP_EVENT_PARAM_VALUE;
                ev.header.size = sizeof(ev);
                ev.header.flags = 0;
                ev.param_id = clientEv.paramId;
                ev.value = clientEv.value;
                ev.channel = -1;
                ev.key = -1;
                ev.cookie = param;

               if (!ov->try_push(ov, &ev.header)) [[unlikely]]
                  return;
            } break;
            case ParamInfo:
                break;
            case Note:
                break;
            default:
                SPDLOG_ERROR("Unknown event");  // TODO: provide global converter between ev <> string
        }
    }
}

/*
 * At the core of the plugin is the process() function. It handles
 * events and audio processing together on a per-frame basis. It
 * requires realtime performance. This function:
 *   - dispatches incoming events as produced by the UI
 *   - handles all events on a per-frame basis
 *   - produces audio as needed.
 * [audio-thread & active_state & processing_state]
 */
clap_process_status CorePlugin::process(const clap_process *process) noexcept
{
    // Process all events from the Server
    processGuiEvents(process->out_events);

    clap_process_status retStatus = CLAP_PROCESS_SLEEP;

    auto chin = process->audio_outputs->channel_count;
    auto chout = process->audio_inputs->channel_count;
    if (chin != chout) {
        SPDLOG_CRITICAL("Process: channel count mismatch");
        return retStatus;
    }

    const auto *inEvs = process->in_events;
    auto nEvts = inEvs->size(inEvs);
    const clap_event_header_t *ev = nullptr;
    uint32_t evIdx = 0;
    if (nEvts != 0)
        ev = inEvs->get(inEvs, evIdx);

    // TODO: Try batch event-processing with a block size; enqueue the
    // processed ones and then batch process the audio. We don't want to
    // invoke methods for each call. This is very inefficient.
    uint32_t frame = 0;
    const uint32_t maxFrames = process->frames_count;
    while (frame < maxFrames) {
        // Process events at given @frame. There can be multiple events per frame.
        while (ev && ev->time == frame) {
            if (ev->time < frame) {
                hostMisbehaving("Event time is in the past");
                SPDLOG_CRITICAL("Process: event time is in the past");
            } else if (ev->time > frame) {
                SPDLOG_ERROR("Process: event time is in the future");
                break;
            }
            processEvent(ev);
            ++evIdx;
            ev = (evIdx < nEvts) ? inEvs->get(inEvs, evIdx) : nullptr;
        }
        // Process audio at given @frame
        retStatus = dPtr->rootModule->process(process, frame);
        ++frame;
    }

    // Plugin To Gui Producer Done
    return retStatus;
}

void CorePlugin::processEvent(const clap_event_header_t *evHdr) noexcept
{
    if (evHdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (evHdr->type) {

    case CLAP_EVENT_PARAM_VALUE: {
        // Try to find the parameter by cookie first and fallback to param_id.
        const auto *evParam = reinterpret_cast<const clap_event_param_value *>(evHdr);
        auto *param = reinterpret_cast<Parameter *>(evParam->cookie);
        if (!param) {
            SPDLOG_DEBUG("Event process: parameter not found in cookie");
            param = getParameterById(evParam->param_id);
            if (!param) {
                SPDLOG_ERROR("Event process: parameter not found");
                break;
            }
        }
        if (param->paramInfo().id != evParam->param_id) {
            hostMisbehaving("Parameter paramId mismatch");
            std::terminate();
        }
        param->setValue(evParam->value);
        pushToProcessQueue({ Event::Param, ClapEventParamWrapper(evParam) });
    } break;

    case CLAP_EVENT_PARAM_MOD: {
        // Try to find the parameter by cookie first and fallback to param_id.
        const auto *evParam = reinterpret_cast<const clap_event_param_mod *>(evHdr);
        auto *param = reinterpret_cast<Parameter *>(evParam->cookie);
        if (!param) {
            SPDLOG_DEBUG("Event process: parameter not found in cookie");
            param = getParameterById(evParam->param_id);
            if (!param) {
                SPDLOG_ERROR("Event process: parameter not found");
                break;
            }
        }
        if (param->paramInfo().id != evParam->param_id) {
            hostMisbehaving("Parameter paramId mismatch");
            std::terminate();
        }
        param->setModulation(evParam->amount);
        pushToProcessQueue({ Event::Param, ClapEventParamWrapper(evParam) });
    } break;

    case CLAP_EVENT_NOTE_ON: {
        const auto *evNote = reinterpret_cast<const clap_event_note *>(evHdr);
        pushToProcessQueue({ Event::Note, ClapEventNoteWrapper { evNote, CLAP_EVENT_NOTE_ON }});
    } break;

    case CLAP_EVENT_NOTE_OFF: {
        const auto *evNote = reinterpret_cast<const clap_event_note *>(evHdr);
        pushToProcessQueue({ Event::Note, ClapEventNoteWrapper { evNote, CLAP_EVENT_NOTE_OFF }});
    } break;

    case CLAP_EVENT_NOTE_CHOKE: {
        const auto *evNote = reinterpret_cast<const clap_event_note *>(evHdr);
        pushToProcessQueue({ Event::Note, ClapEventNoteWrapper { evNote, CLAP_EVENT_NOTE_CHOKE }});
    } break;

    case CLAP_EVENT_NOTE_END: {
        const auto *evNote = reinterpret_cast<const clap_event_note *>(evHdr);
        pushToProcessQueue({ Event::Note, ClapEventNoteWrapper { evNote, CLAP_EVENT_NOTE_END }});
    } break;

    case CLAP_EVENT_NOTE_EXPRESSION: {
        const auto *evNote = reinterpret_cast<const clap_event_note_expression *>(evHdr);
        pushToProcessQueue({ Event::Note, ClapEventNoteWrapper { evNote, CLAP_EVENT_NOTE_EXPRESSION, evNote->expression_id }});
    } break;

    case CLAP_EVENT_MIDI: {
        SPDLOG_INFO("CLAP_EVENT_MIDI not implemented yet");
    } break;

    case CLAP_EVENT_MIDI2: {
        SPDLOG_INFO("CLAP_EVENT_MIDI2 not implemented yet");
    } break;

    case CLAP_EVENT_MIDI_SYSEX: {
        SPDLOG_INFO("CLAP_EVENT_MIDI_SYSEX not implemented yet");
    } break;

    default:
        break;
    }
}

// #### PARAMS ####

uint32_t CorePlugin::paramsCount() const noexcept
{
    return static_cast<uint32_t>(dPtr->params.size());
}

bool CorePlugin::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
    auto *param = getParameterByIndex(paramIndex);
    if (!param)
        return false;
    *info = param->paramInfo();
    return true;
}

bool CorePlugin::paramsValue(clap_id paramId, double *value) noexcept
{
    auto *param = getParameterById(paramId);
    if (!param)
        return false;

    *value = param->value();
    return true;
}

bool CorePlugin::paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept
{
    auto *param = getParameterById(paramId);
    if (!param)
        return false;

    const auto text = param->valueType()->toText(value);
    safe_str_copy(display, size, text.c_str());
    return true;
}

bool CorePlugin::paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept
{
    auto *param = getParameterById(paramId);
    if (!param)
        return false;

    *value = param->valueType()->fromText(display);
    return true;
}

void CorePlugin::paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept
{
    SPDLOG_INFO("CorePlugin::paramsFlush(), not implemented yet.");
}

Parameter *CorePlugin::addParameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType)
{
    assert(dPtr);
    auto copy = std::make_unique<Parameter>(
        info, std::move(valueType), static_cast<uint32_t>(dPtr->params.size())
    );
    auto *ptr = copy.get();
    // Add a pointer to the map to find it by paramId for fast lookup. The host can provide this in the cookie.
    auto ret = dPtr->paramsHashed.insert_or_assign(info.id, ptr);
    if (!ret.second)
        throw std::logic_error(fmt::format("Parameter with id {} already exists!", info.id));
    // Store in the vector
    dPtr->params.push_back(std::move(copy));

    return ptr;
}

Parameter *CorePlugin::getParameterByIndex(uint32_t index) const noexcept
{
    if (index >= dPtr->params.size())
        return nullptr;
    return dPtr->params.at(index).get();
}

Parameter *CorePlugin::getParameterById(clap_id paramId) const noexcept
{
    auto it = dPtr->paramsHashed.find(paramId);
    if (it == dPtr->paramsHashed.end())
        return nullptr;
    return it->second;
}

// #### GUI ####
ServerEvent createSEMainSync(Event e, int64_t window_id = -1) // TODO
{
    ServerEvent se;
    se.set_event(e);
    if (window_id != -1)
        se.mutable_main_sync()->set_window_id(window_id);
    return se;
}

bool CorePlugin::implementsGui() const noexcept
{
    return dPtr->settings->hasExecutable();
}

bool CorePlugin::guiCreate(const char *api, bool isFloating) noexcept
{
    SPDLOG_DEBUG("GuiCreate, active streams: {}", dPtr->sharedData->nStreams());

    if (!ServerCtrl::instance().isRunning()) {
        SPDLOG_ERROR("Server is not running");
        return false;
    }

    if (dPtr->guiProc) {
        SPDLOG_ERROR("Gui Proc must be null upon gui creation");
        return false;
    }
    dPtr->guiProc = std::make_unique<ProcessHandle>();

    // Recheck the executable. It could be deleted in the meantime.
    if (!dPtr->settings->executable()) {
        SPDLOG_ERROR("executable not available");
        return false;
    }

    if (!dPtr->guiProc->setExecutable(*dPtr->settings->executable())) {
        SPDLOG_ERROR("Failed to set executable");
        return false;
    }

    const auto addr = ServerCtrl::instance().address();
    assert(addr);

    // Send the hash as argument to identify the plugin to this instance.
    const auto shash = std::to_string(dPtr->hashCore);
    if (!dPtr->guiProc->setArguments({ *addr, shash })) { // Prepare to launch the GUI
        SPDLOG_ERROR("Failed to set args for executable");
        return false;
    }

    if (!dPtr->guiProc->startChild()) {  // Start the GUI
        SPDLOG_ERROR("Failed to execute GUI");
        return false;
    }

    SPDLOG_INFO("Started GUI with PID: {}", dPtr->guiProc->getChildPid());
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // FIXME, sync issue with the server when restarting quickly
    // Wait for the polling queue to be ready. And send a verification event.
    crill::progressive_backoff_wait([&]{
        // TODO: Bail out after x time or process closed
//        SPDLOG_TRACE("Waiting for polling queue to be ready");
        return dPtr->sharedData->isPolling();
    });

    pushToMainQueueBlocking({Event::GuiCreate, ClapEventMainSyncWrapper{}});
    if (!dPtr->sharedData->blockingVerifyEvent(Event::GuiCreate)) {
        SPDLOG_WARN("GUI has failed to verify in time. Killing Gui process.");
        if (!dPtr->guiProc->terminateChild())
            SPDLOG_CRITICAL("GUI proc failed to killChild");
        return false;
    }
    enqueueAuxiliaries();

    return true;
}

bool CorePlugin::guiSetTransient(const clap_window *window) noexcept
{
    SPDLOG_TRACE("GuiSetTransient");
    assert(dPtr->sharedData->isPolling());

    if (!window) {
       SPDLOG_ERROR("clap_window is null");
       return false;
    }

    int64_t wid = -1;
    if (!strcmp(CLAP_WINDOW_API_COCOA, window->api)) {
        wid = reinterpret_cast<int64_t>(window->cocoa);
    } else if (!strcmp(CLAP_WINDOW_API_WIN32, window->api)) {
        wid = reinterpret_cast<int64_t>(window->win32);
    } else if (!strcmp(CLAP_WINDOW_API_X11, window->api)) {
        wid = static_cast<int64_t>(window->x11);
    } else if (!strcmp(CLAP_WINDOW_API_WAYLAND, window->api)) {
        SPDLOG_ERROR("Wayland does not support embedding");
    } else {
        SPDLOG_CRITICAL("Unsupported window api: {}", window->api);
        return false;
    }

    pushToMainQueueBlocking( {Event::GuiSetTransient, ClapEventMainSyncWrapper{ wid } });
    if (!dPtr->sharedData->blockingVerifyEvent(Event::GuiSetTransient)) {
        SPDLOG_ERROR("GuiSetTransient failed to get verification from client");
        return false;
    }
    return true;
}

bool CorePlugin::guiShow() noexcept
{
    SPDLOG_TRACE("GuiShow");
    assert(dPtr->sharedData->isPolling());

   pushToMainQueueBlocking({Event::GuiShow, ClapEventMainSyncWrapper{}});
    if (!dPtr->sharedData->blockingVerifyEvent(Event::GuiShow)) {
        SPDLOG_ERROR("GuiShow failed to get verification from client");
        return false;
    }
    return true;
}

bool CorePlugin::guiHide() noexcept
{
    SPDLOG_TRACE("GuiHide");
    assert(dPtr->sharedData->isPolling());

    pushToMainQueueBlocking({Event::GuiHide, ClapEventMainSyncWrapper{}});
    if (!dPtr->sharedData->blockingVerifyEvent(Event::GuiHide)) {
        SPDLOG_ERROR("GuiHide failed to get verification from client");
        return false;
    }
    return true;
}

void CorePlugin::guiDestroy() noexcept
{
    SPDLOG_TRACE("GuiDestroy");
//    assert(dPtr->sharedData->isPolling());
    try {
        pushToMainQueueBlocking({Event::GuiDestroy, ClapEventMainSyncWrapper{}});
        if (!dPtr->sharedData->blockingVerifyEvent(Event::GuiDestroy))
            SPDLOG_ERROR("GuiDestroy failed to get verification from client");
        dPtr->guiProc.reset();
    } catch (const std::exception &e) {
        SPDLOG_ERROR("guiDestroy(): {}", e.what());
    }
}

// #### AUDIO PORTS ####
uint32_t CorePlugin::audioPortsCount(bool is_input) const noexcept
{
    if (is_input)
        return static_cast<uint32_t>(dPtr->audioPortsInfoIn.size());
    return static_cast<uint32_t>(dPtr->audioPortsInfoOut.size());
}

bool CorePlugin::audioPortsInfo(uint32_t index, bool is_input, clap_audio_port_info *info) const noexcept
{
    *info = is_input ? dPtr->audioPortsInfoIn.at(index) : dPtr->audioPortsInfoOut.at(index);
    return true;
}

std::vector<clap_audio_port_info> &CorePlugin::audioPortsInfoIn() noexcept
{
    return dPtr->audioPortsInfoIn;
}

std::vector<clap_audio_port_info> &CorePlugin::audioPortsInfoOut() noexcept
{
    return dPtr->audioPortsInfoOut;
}

void CorePlugin::logInfo()
{
    constexpr std::string_view  PluginInfoMsg = "\n\n"
        "##################################################################\n"
        "         ******** {} ********\n"
        "Host:             {}, {}\n"
        "Clap:             {}\n"
        "Plugin:           {}\n"
        "CorePlugin hash:  {}\n"
        "Root Module:      {}\n"
        "Plugin directory: {}\n"
        "Log file:         {}\n"
        "Executable:       {}\n"
        "##################################################################\n";
    const auto stime = fmt::format("{}", std::chrono::system_clock::now());
    SPDLOG_INFO(PluginInfoMsg,
        stime,
        _host.host()->name, _host.host()->version,
        dPtr->settings->clapPath(),
        dPtr->desc->name, dPtr->hashCore,
        dPtr->rootModule->name(),
        dPtr->settings->pluginDirectory().value_or("No Plugin Directory!"),
        dPtr->settings->logFile().value_or("No Log File!"),
        dPtr->settings->executable().value_or("No Executable!").data()
    );
}

bool CorePlugin::pushToMainQueue(ServerEventWrapper &&ev)
{
    return dPtr->sharedData->pluginMainToClientsQueue().push(std::move(ev));
}

void CorePlugin::pushToMainQueueBlocking(ServerEventWrapper &&ev) {
    crill::progressive_backoff_wait([&] {
        if (!dPtr->sharedData->isValid())
            return false;
        return pushToMainQueue(std::move(ev));
    });
}

void CorePlugin::pushToProcessQueue(ServerEventWrapper &&ev)
{
    dPtr->sharedData->pluginToClientsQueue().push(std::move(ev));
}

void CorePlugin::enqueueAuxiliaries()
{
    for (const auto &p : dPtr->params) {
        pushToMainQueueBlocking({Event::ParamInfo, ClapEventParamInfoWrapper {
            .paramId = p->paramInfo().id,
            .name = p->paramInfo().name,
            .module = p->paramInfo().module,
            .minValue = p->paramInfo().min_value,
            .maxValue = p->paramInfo().max_value,
            .defaultValue = p->paramInfo().default_value
        }});
    }
}

uint32_t CorePlugin::notePortsCount(bool is_input) const noexcept
{
    return is_input ? static_cast<uint32_t>(dPtr->notePortsInfoIn.size()) : static_cast<uint32_t>(dPtr->notePortsInfoOut.size());
}

bool CorePlugin::notePortsInfo(uint32_t index, bool is_input, clap_note_port_info *info) const noexcept
{
    *info = is_input ? dPtr->notePortsInfoIn.at(index) : dPtr->notePortsInfoOut.at(index);
    return true;
}

std::vector<clap_note_port_info> &CorePlugin::notePortsInfoIn() noexcept
{
    return dPtr->notePortsInfoIn;
}

std::vector<clap_note_port_info> &CorePlugin::notePortsInfoOut() noexcept
{
    return dPtr->notePortsInfoOut;
}

RCLAP_END_NAMESPACE
