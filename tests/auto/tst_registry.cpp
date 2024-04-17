#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>

#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace clap::rci;

class Plugin1 : public CorePlugin
{
    REGISTER;

public:
    explicit Plugin1(const clap_host *host)
        : CorePlugin(descriptor(), host)
    {
        DLOG(INFO) << "Plugin1 created";
    }

    static const Descriptor *descriptor() noexcept
    {
        static Descriptor desc(
            "clap.rci.plugin1", "Plugin1", "vendor", "0.0.1"
        );
        return &desc;
    }
};

class Plugin2 : public CorePlugin
{
    REGISTER;

public:
    explicit Plugin2(const clap_host *host)
        : CorePlugin(descriptor(), host)
    {
        DLOG(INFO) << "Plugin2 created";
    }

    static const Descriptor *descriptor() noexcept
    {
        static Descriptor desc(
            "clap.rci.plugin2", "Plugin2", "vendor", "0.0.1"
        );
        return &desc;
    }
};

TEST_CASE("Registry API", "[registry]")
{
    std::string clapPath = "/path/to/clap.clap";
    Registry::init(clapPath.data());
    REQUIRE(Registry::clapPath() == clapPath);
    REQUIRE(Registry::entrySize() == 2u);
    REQUIRE(*Registry::findDescriptor(0) == *Plugin1::descriptor());
    REQUIRE(*Registry::findDescriptor(1) == *Plugin2::descriptor());
    REQUIRE(*Registry::findDescriptor(1) != *Plugin1::descriptor());
    REQUIRE(*Registry::findDescriptor(0) != *Plugin2::descriptor());
    REQUIRE(
        *Registry::findDescriptor(Plugin1::descriptor()->id())
        == *Plugin1::descriptor()
    );
    REQUIRE(
        *Registry::findDescriptor(Plugin2::descriptor()->id())
        == *Plugin2::descriptor()
    );
    Registry::deinit();
    REQUIRE(Registry::clapPath().empty());
}

TEST_CASE("Instances API", "[registry]")
{
    clap_host host {};
    auto p10 = std::make_unique<Plugin1>(&host);
    const auto *pp10 = p10.get();
    Registry::Instances::emplace(Plugin1::descriptor()->id(), std::move(p10));
    auto p20 = std::make_unique<Plugin2>(&host);
    const auto *pp20 = p20.get();
    Registry::Instances::emplace(Plugin2::descriptor()->id(), std::move(p20));

    auto p11 = std::make_unique<Plugin1>(&host);
    const auto *pp11 = p11.get();
    Registry::Instances::emplace(Plugin1::descriptor()->id(), std::move(p11));
    auto p21 = std::make_unique<Plugin2>(&host);
    const auto *pp21 = p21.get();
    Registry::Instances::emplace(Plugin2::descriptor()->id(), std::move(p21));

    const auto &instances = Registry::Instances::instances();
    REQUIRE(instances.size() == 4);

    auto bucketSize = [](auto &&range) {
        size_t size = 0;
        auto it = range.first;
        while (it != range.second) {
            ++it;
            ++size;
        }
        return size;
    };

    REQUIRE(
        bucketSize(instances.equal_range(Plugin1::descriptor()->id())) == 2
    );
    REQUIRE(
        bucketSize(instances.equal_range(Plugin2::descriptor()->id())) == 2
    );

    REQUIRE(Registry::Instances::instance(pp10->pluginId()) == pp10);
    REQUIRE(Registry::Instances::instance(pp11->pluginId()) == pp11);
    REQUIRE(Registry::Instances::instance(pp20->pluginId()) == pp20);
    REQUIRE(Registry::Instances::instance(pp21->pluginId()) == pp21);

    REQUIRE(Registry::Instances::destroy(pp10->descriptor()->id(), pp10));
    REQUIRE(
        bucketSize(instances.equal_range(Plugin1::descriptor()->id())) == 1
    );
    REQUIRE(Registry::Instances::destroy(pp20->descriptor()->id(), pp20));
    REQUIRE(
        bucketSize(instances.equal_range(Plugin2::descriptor()->id())) == 1
    );

    REQUIRE(Registry::Instances::destroy(pp11->descriptor()->id(), pp11));
    REQUIRE(Registry::Instances::destroy(pp21->descriptor()->id(), pp21));
    REQUIRE(instances.empty());
    REQUIRE(!Registry::Instances::destroy(Plugin1::descriptor()->id(), pp21));
}
