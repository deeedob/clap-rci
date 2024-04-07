#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/export.h>

#include <catch2/catch_test_macros.hpp>
#include <iostream>

class Plugin1 : public CorePlugin {
    REGISTER;
public:
    explicit Plugin1(const clap_host* host)
        : CorePlugin(descriptor(), host)
    {
        std::cout << "I got called" << std::endl;
    }

    static const Descriptor *descriptor() noexcept
    {
        static Descriptor desc("clap.rci.plugin1", "Plugin1", "vendor", "0.0.1");
        return &desc;
    }
};

class Plugin2 : public CorePlugin {
    REGISTER;
public:
    explicit Plugin2(const clap_host* host)
        : CorePlugin(descriptor(), host)
    {
        std::cout << "I got called" << std::endl;
    }

    static const Descriptor *descriptor() noexcept
    {
        static Descriptor desc("clap.rci.plugin2", "Plugin2", "vendor", "0.0.1");
        return &desc;
    }
};

TEST_CASE("Registry", "[registry]")
{
    std::string p = "/custom/path/to/clap.clap";
    REQUIRE(clap_entry.init(p.data()));
    REQUIRE(Registry::clapPath() == p);

    const auto *fact = reinterpret_cast<const clap_plugin_factory *>(
        clap_entry.get_factory(CLAP_PLUGIN_FACTORY_ID)
    );
    REQUIRE(fact != nullptr);
    const auto count = fact->get_plugin_count(fact);
    REQUIRE(count == 2);

    const auto *d1 = fact->get_plugin_descriptor(fact, 0);
    const auto *d2 = fact->get_plugin_descriptor(fact, 1);
    REQUIRE(*d1 == *Plugin1::descriptor());
    REQUIRE(*d2 == *Plugin2::descriptor());

    clap_host testhost = {};
    const auto *plug = fact->create_plugin(fact, &testhost, d1->id);
    REQUIRE(*plug->desc == *Plugin1::descriptor());
}
