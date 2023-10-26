#ifndef VALUETYPE_H
#define VALUETYPE_H

#include <core/global.h>
#include <limits>

RCLAP_BEGIN_NAMESPACE

class ValueType
{
public:
    virtual ~ValueType() = default;
    /* The sample rate may be required to do the conversion to engine */
    virtual void setSampleRate(double sampleRate) noexcept {}

    [[nodiscard]] virtual bool isStepped() const noexcept { return false; }

    [[nodiscard]] virtual double defaultValue() const noexcept { return std::numeric_limits<double>::lowest(); }
    [[nodiscard]] virtual double minValue() const noexcept { return std::numeric_limits<double>::lowest(); }
    [[nodiscard]] virtual double maxValue() const noexcept { return std::numeric_limits<double>::max(); }

    [[nodiscard]] virtual std::string toText(double paramValue) const = 0;
    [[nodiscard]] virtual double fromText(const std::string &paramValueText) const = 0;

    /* Domain conversion */
    [[nodiscard]] virtual bool hasEngineDomain() const { return false; };
    [[nodiscard]] virtual double toParam(double engineValue) const { return engineValue; };
    [[nodiscard]] virtual double toEngine(double paramValue) const { return paramValue; };
};

RCLAP_END_NAMESPACE

#endif // VALUETYPE_H
