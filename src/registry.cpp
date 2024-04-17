#include <clap-rci/coreplugin.h>
#include <clap-rci/registry.h>

#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/log/log_sink.h>

#include <cstring>
#include <format>

CLAP_RCI_BEGIN_NAMESPACE

void Registry::Instances::emplace(
    std::string_view key, std::unique_ptr<CorePlugin>&& value
)
{
    DLOG(INFO) << std::format(
        "PluginInstances emplaced: key {}, ptr {:p}", key,
        reinterpret_cast<const void*>(value.get())
    );
    sPluginInstances.emplace(key, std::move(value));
}

bool Registry::Instances::destroy(
    std::string_view key, const CorePlugin* instance
)
{
    auto range = sPluginInstances.equal_range(key);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.get() == instance) {
            DLOG(INFO) << std::format(
                "PluginInstances destroyed: key {}, ptr {:p}", key,
                reinterpret_cast<const void*>(instance)
            );
            sPluginInstances.erase(it);
            return true;
        }
    }
    return false;
}

CorePlugin* Registry::Instances::instance(uint64_t pluginId)
{
    for (const auto& e : sPluginInstances) {
        if (e.second->pluginId() == pluginId)
            return e.second.get();
    }
    return nullptr;
}

bool Registry::init(const char* path)
{
    if (Registry::entrySize() <= 0)
        return false;
    //    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    //    absl::InitializeLog();
    sClapPath = path;
    DLOG(INFO) << std::format("Registry::init({})", path);
    return true;
}

void Registry::deinit()
{
    assert(Registry::Instances::instances().empty());
    sClapPath = "";
}

uint32_t Registry::entrySize() noexcept
{
    return static_cast<uint32_t>(sPluginEntries.size());
}

const clap_plugin* Registry::create(const clap_host* host, const char* id)
{
    for (const auto& e : sPluginEntries) {
        if (strcmp(e.descriptor->id, id) == 0)
            return e.creator(host);
    }
    return nullptr;
}

const clap_plugin_descriptor* Registry::findDescriptor(std::string_view id)
{
    for (const auto& e : sPluginEntries) {
        if (strcmp(e.descriptor->id, id.data()) == 0)
            return e.descriptor;
    }
    return nullptr;
}

const clap_plugin_descriptor* Registry::findDescriptor(uint32_t pos)
{
    if (pos >= Registry::entrySize())
        return nullptr;
    return Registry::sPluginEntries.at(pos).descriptor;
}

CLAP_RCI_END_NAMESPACE
