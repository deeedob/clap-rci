#pragma once

#include <clap-rci/descriptor.h>
#include <clap-rci/global.h>
#include <clap/plugin.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

CLAP_RCI_BEGIN_NAMESPACE

class CorePlugin;

class Registry
{
    struct Entry {
        const clap_plugin_descriptor* descriptor;
        std::function<const clap_plugin*(const clap_host*)> creator;
    };

public:
    Registry() = delete;

    template <typename T>
        requires requires(T t) {
            { t.descriptor() } -> std::convertible_to<const Descriptor*>;
            { T(std::declval<const clap_host*>()) };
        } && std::derived_from<T, CorePlugin>
    inline static bool add()
    {
        // TODO: transform into constexpr check somehow
        assert(
            is_unique_id(T::descriptor()->id()) && "Plugin id is not unique."
        );

        sPluginEntries.emplace_back(
            static_cast<const clap_plugin_descriptor*>(*T::descriptor()),
            [](const clap_host* host) -> const clap_plugin* {
                // entry point
                auto p = std::make_unique<T>(host);
                auto* ptr = p.get();
                Instances::emplace(T::descriptor()->id(), std::move(p));
                return ptr->clapPlugin();
            }
        );
        return true;
    }

    static std::string_view clapPath() noexcept { return sClapPath; }

    static uint32_t entrySize() noexcept;
    static const clap_plugin_descriptor* findDescriptor(std::string_view id);
    static const clap_plugin_descriptor* findDescriptor(uint32_t pos);

    static bool init(const char* path);
    static void deinit();
    static const clap_plugin* create(const clap_host* host, const char* id);

    class Instances
    {
        using PluginMap = std::unordered_multimap<
            std::string_view, std::unique_ptr<CorePlugin>>;

    public:
        Instances() = delete;

        static void
        emplace(std::string_view key, std::unique_ptr<CorePlugin>&& value);
        static bool destroy(std::string_view key, const CorePlugin* instance);
        static const PluginMap& instances() { return sPluginInstances; }
        static CorePlugin* instance(uint64_t pluginId);

    private:
        inline static PluginMap sPluginInstances;
    };

private:
    inline static std::vector<Entry> sPluginEntries;
    inline static std::string_view sClapPath;

    static bool is_unique_id(std::string_view id)
    {
        return std::ranges::all_of(sPluginEntries, [id](const auto& entry) {
            return entry.descriptor->id != id;
        });
    }
};

namespace Private {
template <typename T>
struct AutoRegisterType {
    using Type = typename std::remove_const<
        typename std::remove_reference<T>::type>::type;
    // Static initialization trick to auto register types.
    inline static bool isRegistered = Registry::add<Type>();
};
}

#define REGISTER                                                               \
    const void* _auto_registering_method() const                               \
    {                                                                          \
        return &Private::AutoRegisterType<decltype(*this)>::isRegistered;      \
    }

CLAP_RCI_END_NAMESPACE
