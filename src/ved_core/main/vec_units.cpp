#include "vec_units.h"

#include <cmath>
#include <cstdio>

namespace {

constexpr double kMillimetersPerCentimeter = 10.0;
constexpr double kMillimetersPerInch = 25.4;

const char* unitSuffix(TDVecDisplayUnit unit)
{
    switch (unit) {
    case TDVecDisplayUnit::Millimeter: return " mm";
    case TDVecDisplayUnit::Centimeter: return " cm";
    case TDVecDisplayUnit::Inch:       return " in";
    case TDVecDisplayUnit::RawVed:     return " ved";
    }
    return "";
}

} // namespace

TDVecUnitFormatter::TDVecUnitFormatter(TDVecUnitSettings settings)
    : settings_(settings)
{
}

double TDVecUnitFormatter::ToMillimeters(double realValue) const
{
    if (settings_.realUnitsPerMillimeter == 0.0) {
        return 0.0;
    }
    return realValue / settings_.realUnitsPerMillimeter;
}

double TDVecUnitFormatter::FromMillimeters(double millimeters) const
{
    return millimeters * settings_.realUnitsPerMillimeter;
}

double TDVecUnitFormatter::ToDisplayUnit(double realValue) const
{
    switch (settings_.displayUnit) {
    case TDVecDisplayUnit::Millimeter:
        return ToMillimeters(realValue);
    case TDVecDisplayUnit::Centimeter:
        return ToMillimeters(realValue) / kMillimetersPerCentimeter;
    case TDVecDisplayUnit::Inch:
        return ToMillimeters(realValue) / kMillimetersPerInch;
    case TDVecDisplayUnit::RawVed:
        return realValue;
    }
    return realValue;
}

double TDVecUnitFormatter::FromDisplayUnit(double displayValue) const
{
    switch (settings_.displayUnit) {
    case TDVecDisplayUnit::Millimeter:
        return FromMillimeters(displayValue);
    case TDVecDisplayUnit::Centimeter:
        return FromMillimeters(displayValue * kMillimetersPerCentimeter);
    case TDVecDisplayUnit::Inch:
        return FromMillimeters(displayValue * kMillimetersPerInch);
    case TDVecDisplayUnit::RawVed:
        return displayValue;
    }
    return displayValue;
}

std::string TDVecUnitFormatter::FormatCoordinate(double realValue) const
{
    const double displayValue = ToDisplayUnit(realValue);
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f", displayValue);
    return std::string(buffer) + unitSuffix(settings_.displayUnit);
}

std::string TDVecUnitFormatter::FormatLength(double realLength) const
{
    return FormatCoordinate(std::fabs(realLength));
}

const TDVecUnitSettings& TDVecUnitFormatter::Settings() const
{
    return settings_;
}
