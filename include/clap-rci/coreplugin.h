#pragma once

#include <clap-rci/descriptor.h>
#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/global.h>

#include <clap/clap.h>

#include <memory>

CLAP_RCI_BEGIN_NAMESPACE

using namespace CLAP_RCI_API_VERSION;

class Registry;
class CorePluginPrivate;

class CorePlugin
{
public:
    explicit CorePlugin(const Descriptor* descriptor, const clap_host* host);
    virtual ~CorePlugin();

    CorePlugin(const CorePlugin&) = delete;
    CorePlugin& operator=(const CorePlugin&) = delete;

    CorePlugin(CorePlugin&&) = delete;
    CorePlugin& operator=(CorePlugin&&) = delete;

    void hostRequestRestart();
    void hostRequestProcess();

    virtual bool init();
    virtual void destroy();
    virtual bool activate();
    virtual void deactivate();
    virtual bool startProcessing();
    virtual void stopProcessing();
    virtual void reset();
    virtual clap_process_status process(const clap_process* process);
    void withWantsTransport(bool value);
    bool wantsTransport() const noexcept;

    virtual bool isExtNotePortsEnabled() const noexcept { return true; }
    bool withNotePortInfoIn(clap_note_port_info in);
    bool withNotePortInfoOut(clap_note_port_info out);

    bool pushEvent(const api::PluginEventMessage& event);

    [[nodiscard]] uint64_t pluginId() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isProcessing() const noexcept;
    [[nodiscard]] double sampleRate() const noexcept;
    [[nodiscard]] uint32_t minFramesCount() const noexcept;
    [[nodiscard]] uint32_t maxFramesCount() const noexcept;

    const clap_plugin* clapPlugin() const noexcept;

protected:
    clap_plugin mPlugin = {};

private:
    uint64_t mPluginId = 0;
    const Descriptor* mDescriptor = nullptr;
    clap_plugin_note_ports mPluginNotePorts = {};

    bool mIsActive = false;
    bool mIsProcessing = false;
    double mSampleRate = 0;
    uint32_t mMinFramesCount = 0;
    uint32_t mMaxFramesCount = 0;

private:
    std::shared_ptr<CorePluginPrivate> dPtr;
    friend std::shared_ptr<CorePluginPrivate> getPimpl(const CorePlugin* plugin
    );
    friend const std::shared_ptr<CorePluginPrivate>&
    getPimplRef(const CorePlugin* plugin);
};

CLAP_RCI_END_NAMESPACE
