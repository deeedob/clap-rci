#ifndef MODULE_H
#define MODULE_H

#include <core/global.h>
#include "../parameter/valuetype.h"
#include "../parameter/parameter.h"

#include <clap/clap.h>
#include <string>

RCLAP_BEGIN_NAMESPACE

class CorePlugin;

class Module
{
public:
    static constexpr uint32_t ParamOffset = 1024;

    Module(CorePlugin &plugin, std::string name, uint32_t moduleId);
    Module(const Module&) = default;
    virtual ~Module() = default;
    Module(Module&&) = delete;
    Module &operator=(const Module&) = delete;
    Module &operator=(Module&&) = delete;

    std::string_view name() const noexcept { return m_name; }

    virtual void init() noexcept = 0;
    virtual clap_process_status process(const clap_process *process, uint32_t frame) noexcept
    { return CLAP_PROCESS_SLEEP; }

    // Add a parameter to the plugin and return a non-owning pointer to it. Ownership is kept by the CorePlugin.
    Parameter *addParameter(uint32_t id,  const std::string &name, uint32_t flags, std::unique_ptr<ValueType> valueType);
    bool activate() noexcept
    {
        if (m_active)
            return false;
        m_active = true;
        return m_active;
    }

    void deactivate() noexcept
    {
        m_active = false;
    }

    bool startProcessing() noexcept
    {
        return m_active;
    }

    void stopProcessing() noexcept {}

    void reset() noexcept {}

protected:
    CorePlugin &m_plugin;
    std::string m_name;
    uint32_t m_moduleId;
    clap_id m_paramOffset;
    bool m_active = false;
};

RCLAP_END_NAMESPACE

#endif // MODULE_H
