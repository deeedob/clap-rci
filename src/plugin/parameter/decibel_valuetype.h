#ifndef DECIBEL_VALUETYPE_H
#define DECIBEL_VALUETYPE_H

#include "valuetype.h"
#include <string>

RCLAP_BEGIN_NAMESPACE

class DecibelValueType : public ValueType
{
public:
    DecibelValueType(double minValue, double maxValue, double defaultValue);
    [[nodiscard]] double minValue() const noexcept override { return m_minValue; }
    [[nodiscard]] double maxValue() const noexcept override { return m_maxValue; }
    [[nodiscard]] double defaultValue() const noexcept override { return m_defaultValue; }

    [[nodiscard]] std::string toText(double paramValue) const override;
    [[nodiscard]] double fromText(const std::string &paramValueText) const override;

    [[nodiscard]] bool hasEngineDomain() const override { return true; }
    [[nodiscard]] double toEngine(double paramValue) const override;
    [[nodiscard]] double toParam(double engineValue) const override;
private:
    double m_minValue;
    double m_maxValue;
    double m_defaultValue;
};

RCLAP_END_NAMESPACE

#endif
