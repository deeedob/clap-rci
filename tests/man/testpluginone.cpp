#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/export.h>

#include <absl/log/log.h>

class TestPlugin : public CorePlugin {
    REGISTER;

public:
    explicit TestPlugin(const clap_host* host)
        : CorePlugin(descriptor(), host)
    {
        DLOG(INFO) << "TestPluginOne Created";
    }

    ~TestPlugin() override
    {
        DLOG(INFO) << "TestPluginOne Destroyed";
    }

    static const Descriptor* descriptor() noexcept
    {
        static Descriptor desc(
            "clap.rci.one", "TestPluginOne", "vendorone", "0.0.1"
        );
        return &desc;
    }

    bool init() override
    {
        DLOG(INFO) << "init";
        return CorePlugin::init();
    }
    void destroy() override
    {
        DLOG(INFO) << "destroy";
        return CorePlugin::destroy();
    }
    bool activate() override
    {
        DLOG(INFO) << "activate";
        return CorePlugin::activate();
    }
    void deactivate() override
    {
        DLOG(INFO) << "deactivate";
        return CorePlugin::deactivate();
    }
    bool startProcessing() override
    {
        DLOG(INFO) << "startProcessing";
        return CorePlugin::startProcessing();
    }
    void stopProcessing() override
    {
        DLOG(INFO) << "stopProcessing";
        return CorePlugin::stopProcessing();
    }
    void reset() override
    {
        DLOG(INFO) << "reset";
        return CorePlugin::reset();
    }

private:
};
