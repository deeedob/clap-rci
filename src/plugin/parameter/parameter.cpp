#include "parameter.h"

RCLAP_BEGIN_NAMESPACE

Parameter::Parameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType, uint32_t index)
    : m_index(static_cast<int32_t>(index)), m_info(info) , m_valueType(std::move(valueType))
{
    m_info.cookie = this;
    m_value = m_valueType->defaultValue();
}

RCLAP_END_NAMESPACE
