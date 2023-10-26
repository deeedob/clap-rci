#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include "server/wrappers.h"
#include "settings.h"
#include <core/global.h>

#include <clap/clap.h>
#include <clap/helpers/plugin.hh>

RCLAP_BEGIN_NAMESPACE

class Module;
class CtrlData;
class Parameter;
class ValueType;

struct CorePluginPrivate;

using Plugin = clap::helpers::Plugin <
    clap::helpers::MisbehaviourHandler::Terminate,
    clap::helpers::CheckingLevel::Maximal
>;

class CorePlugin : public Plugin
{
    friend class Module;
public:
    CorePlugin(const clap_plugin_descriptor *desc, const clap_host *host);
    CorePlugin(const Settings &settings, const clap_plugin_descriptor *desc, const clap_host *host, std::unique_ptr<Module> rootModule);
    ~CorePlugin() override;

    CorePlugin(const CorePlugin&) = delete;
    CorePlugin(CorePlugin&&) noexcept = delete;
    CorePlugin& operator=(const CorePlugin&) = delete;
    CorePlugin& operator=(CorePlugin&&) noexcept = delete;

    // #### AUDIO PROCESSING #####
    bool init() noexcept override;
    bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
    void deactivate() noexcept override;
    bool startProcessing() noexcept override;
    void stopProcessing() noexcept override;
    void reset() noexcept override;
    void processGuiEvents(const clap_output_events *ov);
    clap_process_status process(const clap_process *process) noexcept override;
    void processEvent(const clap_event_header_t *evHdr) noexcept;

    // #### PARAMS ####
    bool implementsParams() const noexcept override { return true; }
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;
    bool paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept override;
    bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;
    void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override;
    Parameter *addParameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType);
    Parameter *getParameterByIndex(uint32_t index) const noexcept;
    Parameter *getParameterById(clap_id paramId) const noexcept;
    // #### GUI ####
    bool implementsGui() const noexcept override;
    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override { return isFloating; };
    bool guiCreate(const char *api, bool isFloating) noexcept override;
    bool guiSetTransient(const clap_window *window) noexcept override;
    bool guiShow() noexcept override;
    bool guiHide() noexcept override;
    void guiDestroy() noexcept override;
    // #### AUDIO PORTS ####
    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool is_input) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool is_input, clap_audio_port_info *info) const noexcept override;
    std::vector<clap_audio_port_info>& audioPortsInfoIn() noexcept;
    std::vector<clap_audio_port_info>& audioPortsInfoOut() noexcept;
    // #### NOTE PORTS ####
    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool is_input) const noexcept override;
    bool notePortsInfo(uint32_t index, bool is_input, clap_note_port_info *info) const noexcept override;
    std::vector<clap_note_port_info>& notePortsInfoIn() noexcept;
    std::vector<clap_note_port_info>& notePortsInfoOut() noexcept;

    void logInfo();

protected:
    std::unique_ptr<CorePluginPrivate> dPtr;

private:
    bool pushToMainQueue(ServerEventWrapper &&ev);
    void pushToMainQueueBlocking(ServerEventWrapper &&ev);
    void pushToProcessQueue(ServerEventWrapper &&ev);
    void enqueueAuxiliaries();
};

RCLAP_END_NAMESPACE

#endif //COREPLUGIN_H
