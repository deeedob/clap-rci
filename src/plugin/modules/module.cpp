#include "module.h"
#include "../coreplugin.h"

RCLAP_BEGIN_NAMESPACE

Module::Module(CorePlugin &plugin, std::string name, uint32_t moduleId)
    : m_plugin(plugin) , m_name(std::move(name)) , m_moduleId(moduleId)
    , m_paramOffset(m_moduleId * ParamOffset)
{}

Parameter *Module::addParameter(uint32_t id,  const std::string &name, uint32_t flags, std::unique_ptr<ValueType> valueType)
{
    clap_param_info info = {
        .id = m_paramOffset + id,
        .flags = flags,
        .cookie = nullptr,
        .name = {},
        .module = {},
        .min_value = valueType->minValue(),
        .max_value = valueType->maxValue(),
        .default_value = valueType->defaultValue(),
    };
    if (name.size() > sizeof(info.name) || m_name.size() > sizeof(info.module))
        throw std::invalid_argument("Name or module name too long!");
    safe_str_copy(info.name, sizeof(info.name), name.c_str());
    safe_str_copy(info.module, sizeof(info.module), m_name.c_str());
    return m_plugin.addParameter(info, std::move(valueType));
}

RCLAP_END_NAMESPACE
