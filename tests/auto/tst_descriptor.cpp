#include <clap-rci/descriptor.h>

#include <catch2/catch_test_macros.hpp>

using namespace CLAP_RCI_NAMESPACE;

TEST_CASE("Descriptor", "[construction]")
{
    Descriptor d1(
        "clap.one", "oneplugin", "generic", "v.15.6",
        {CLAP_PLUGIN_FEATURE_AMBISONIC, CLAP_PLUGIN_FEATURE_MULTI_EFFECTS,
         CLAP_PLUGIN_FEATURE_NOTE_EFFECT}
    );
    Descriptor d2(
        "clap.one", "oneplugin", "generic", "v.15.6",
        {CLAP_PLUGIN_FEATURE_AMBISONIC, CLAP_PLUGIN_FEATURE_MULTI_EFFECTS,
         CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_PITCH_SHIFTER}
    );

    REQUIRE(d1 != d2);

    const char* features[] = {
        CLAP_PLUGIN_FEATURE_AMBISONIC, CLAP_PLUGIN_FEATURE_MULTI_EFFECTS,
        CLAP_PLUGIN_FEATURE_NOTE_EFFECT, NULL
    };

    clap_plugin_descriptor d3 = {
        .clap_version = CLAP_VERSION,
        .id = "clap.one",
        .name = "oneplugin",
        .vendor = "generic",
        .url = nullptr,
        .manual_url = nullptr,
        .support_url = nullptr,
        .version = "v.15.6",
        .description = nullptr,
        .features = features
    };

    REQUIRE(d1 == d3);

    Descriptor d4(d3);
    Descriptor d5(
        {.clap_version = CLAP_VERSION,
         .id = "clap.one",
         .name = "oneplugin",
         .vendor = "generic",
         .url = nullptr,
         .manual_url = nullptr,
         .support_url = nullptr,
         .version = "v.15.6",
         .description = nullptr,
         .features = features}
    );
    REQUIRE(d4 == d5);
    REQUIRE(d5 != d2);

    const clap_plugin_descriptor* d2Ptr = d2;
    REQUIRE(d2 == d2Ptr);

    Descriptor d6("id", "name", "vendor", "version");
    d6.withUrl("url")
        .withManualUrl("manual")
        .withSupportUrl("support")
        .withDescription("description")
        .withFeature(CLAP_PLUGIN_FEATURE_PITCH_SHIFTER)
        .withFeature(CLAP_PLUGIN_FEATURE_PITCH_CORRECTION);

    Descriptor d7(
        "id", "name", "vendor", "url", "manual", "support", "version",
        "description",
        {CLAP_PLUGIN_FEATURE_PITCH_SHIFTER,
         CLAP_PLUGIN_FEATURE_PITCH_CORRECTION}
    );

    REQUIRE(d6 == d7);
}
