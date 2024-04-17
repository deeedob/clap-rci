#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/entry.h>
#include <clap-rci/registry.h>

using namespace clap::rci;

class TestPlugin : public CorePlugin
{
    REGISTER;

public:
    explicit TestPlugin(const clap_host* host)
        : CorePlugin(descriptor(), host)
    {
    }
    static const Descriptor* descriptor() noexcept
    {

        static Descriptor desc(
            "clap.rci.two", "TestPluginTwo", "vendortwo", "0.0.1"
        );
        return &desc;
    }

private:
};
