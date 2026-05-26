#pragma once

#include <string>

enum class TDVecDisplayUnit {
    RawVed,
    Millimeter,
    Centimeter,
    Inch
};

struct TDVecUnitSettings {
    double realUnitsPerMillimeter = 1000.0;
    TDVecDisplayUnit displayUnit = TDVecDisplayUnit::Millimeter;
};

class TDVecUnitFormatter {
public:
    explicit TDVecUnitFormatter(TDVecUnitSettings settings);

    double ToMillimeters(double realValue) const;
    double FromMillimeters(double millimeters) const;
    double ToDisplayUnit(double realValue) const;
    double FromDisplayUnit(double displayValue) const;
    std::string FormatCoordinate(double realValue) const;
    std::string FormatLength(double realLength) const;
    const TDVecUnitSettings& Settings() const;

private:
    TDVecUnitSettings settings_;
};
