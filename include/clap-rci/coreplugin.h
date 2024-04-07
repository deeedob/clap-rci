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

class CorePlugin {
public:
    explicit CorePlugin(const Descriptor* descriptor, const clap_host* host);
    virtual ~CorePlugin();

    CorePlugin(const CorePlugin&) = delete;
    CorePlugin& operator=(const CorePlugin&) = delete;

    CorePlugin(CorePlugin&&) = default;
    CorePlugin& operator=(CorePlugin&&) = default;

    void hostRequestRestart();
    void hostRequestProcess();

    virtual bool init();
    virtual void destroy();
    virtual bool activate();
    virtual void deactivate();
    virtual bool startProcessing();
    virtual void stopProcessing();
    virtual void reset();
    virtual clap_process_status process(const clap_process_t* process);

    bool pushEvent(const api::PluginEventMessage& event);

    [[nodiscard]] uint64_t idHash() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isProcessing() const noexcept;
    [[nodiscard]] double sampleRate() const noexcept;
    [[nodiscard]] uint32_t minFramesCount() const noexcept;
    [[nodiscard]] uint32_t maxFramesCount() const noexcept;

private:
    const Descriptor* mDescriptor = nullptr;
    clap_plugin mPlugin = {};

    uint64_t mIdHash = 0;
    bool mIsActive = false;
    bool mIsProcessing = false;
    double mSampleRate = 0;
    uint32_t mMinFramesCount = 0;
    uint32_t mMaxFramesCount = 0;

private:
    std::unique_ptr<CorePluginPrivate> dPtr;
    friend CorePluginPrivate* getImplPtr(const CorePlugin* plugin);
    friend Registry;
};

CLAP_RCI_END_NAMESPACE
