#include <clap-rci/coreplugin.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>
// #include <absl/log/initialize.h>

#include <cstring>
#include <format>

CLAP_RCI_BEGIN_NAMESPACE

void PluginInstances::emplace(
    std::string_view key, std::unique_ptr<CorePlugin>&& value
)
{
    sPluginInstances.emplace(key, std::move(value));
}

bool PluginInstances::destroy(std::string_view id, CorePlugin* instance)
{
    auto range = sPluginInstances.equal_range(id);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.get() == instance) {
            sPluginInstances.erase(it);
            DLOG(INFO) << std::format(
                "Registry::destroyInstance({},{}) on {}", id, (void*)instance,
                (void*)&sPluginInstances
            );
            return true;
        }
    }
    return false;
}

CorePlugin* PluginInstances::instance(uint64_t idHash)
{
    for (const auto& e : sPluginInstances) {
        DLOG(INFO) << "search hash " << e.second->idHash() << " == " << idHash;
        if (e.second->idHash() == idHash)
            return e.second.get();
    }
    return nullptr;
}

std::string_view Registry::clapPath() noexcept
{
    return sClapPath;
}

bool Registry::init(const char* path)
{
    if (Registry::entrySize() <= 0)
        return false;
    // absl::InitializeLog();
    DLOG(INFO) << std::format("Registry::init({})", path);
    sClapPath = path;
    return true;
}

void Registry::deinit()
{
    DLOG(INFO) << "Registry::deinit()";
    sClapPath = "";
}

uint32_t Registry::entrySize()
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
