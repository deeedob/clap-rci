#pragma once

#include <clap-rci/global.h>
#include <clap-rci/registry.h>
#include <cstring>

using namespace CLAP_RCI_NAMESPACE;

const clap_plugin_factory pluginFactory = {
    .get_plugin_count =
        [](const clap_plugin_factory*) {
            return Registry::entrySize();
        },

    .get_plugin_descriptor =
        [](const clap_plugin_factory*, uint32_t idx) {
            return Registry::findDescriptor(idx);
        },

    .create_plugin = [](const clap_plugin_factory*, const clap_host* host,
                        const char* id) -> const clap_plugin_t* {
        return Registry::create(host, id);
    },
};

extern "C" CLAP_EXPORT const clap_plugin_entry clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = Registry::init,
    .deinit = Registry::deinit,
    .get_factory = [](const char* factoryId) -> const void* {
        if (strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0) {
            return &pluginFactory;
        }
        return nullptr;
    },
};
