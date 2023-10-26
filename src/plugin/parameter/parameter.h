#ifndef PARAMETER_H
#define PARAMETER_H

#include <core/global.h>
#include <clap/ext/params.h>
#include "decibel_valuetype.h"

#include <memory>

RCLAP_BEGIN_NAMESPACE

class Parameter
{
public:
    explicit Parameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType, uint32_t index);
    Parameter(const Parameter &) = delete;
    Parameter(Parameter &&) = delete;
    Parameter &operator=(const Parameter &) = delete;
    Parameter &operator=(Parameter &&) = delete;

    int32_t paramIndex() const noexcept { return m_index; }
    const clap_param_info &paramInfo() const noexcept { return m_info; }
    const std::unique_ptr<ValueType> &valueType() const noexcept { return m_valueType; }

    double engineValue() const noexcept { return m_valueType->toEngine(m_value); }
    double value() const noexcept { return m_value; }
    bool trySetValue(double value) noexcept
    {
        if (m_value == value)
            return false;
        m_value = value;
        return true;
    }
    void setValue(double value) noexcept { m_value = value; }
    void setModulation(double modulation) noexcept { mMod = modulation; }
private:
    int32_t m_index;
    clap_param_info m_info;
    std::unique_ptr<ValueType> m_valueType;
    double m_value;
    double mMod = 0.0;
};

RCLAP_END_NAMESPACE

#endif // PARAMETER_H
