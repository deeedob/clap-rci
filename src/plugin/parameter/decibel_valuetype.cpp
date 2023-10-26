#include "decibel_valuetype.h"

#include <cmath>

RCLAP_BEGIN_NAMESPACE

DecibelValueType::DecibelValueType(double minValue, double maxValue, double defaultValue)
    : m_minValue(minValue), m_maxValue(maxValue), m_defaultValue(defaultValue)
{}

std::string DecibelValueType::toText(double paramValue) const
{
    return std::to_string(paramValue) + " dB";
}

double DecibelValueType::fromText(const std::string &paramValueText) const
{
    return std::stod(paramValueText);
}

double DecibelValueType::toEngine(double paramValue) const
{
    return std::pow(10.0, paramValue / 20.0);
}

double DecibelValueType::toParam(double engineValue) const
{
    return 20.0 * std::log10(engineValue);
}

RCLAP_END_NAMESPACE
