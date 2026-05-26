#include "vec_units.h"

#include <cassert>
#include <cmath>
#include <string>

namespace {

constexpr double kEpsilon = 1e-9;

bool approxEqual(double a, double b)
{
    return std::fabs(a - b) < kEpsilon;
}

void testToMillimetersDefault()
{
    TDVecUnitFormatter fmt(TDVecUnitSettings{});
    assert(approxEqual(fmt.ToMillimeters(1000.0), 1.0));
    assert(approxEqual(fmt.ToMillimeters(0.0), 0.0));
    assert(approxEqual(fmt.ToMillimeters(210000.0), 210.0));
    assert(approxEqual(fmt.ToMillimeters(-5000.0), -5.0));
}

void testFromMillimetersDefault()
{
    TDVecUnitFormatter fmt(TDVecUnitSettings{});
    assert(approxEqual(fmt.FromMillimeters(1.0), 1000.0));
    assert(approxEqual(fmt.FromMillimeters(0.0), 0.0));
    assert(approxEqual(fmt.FromMillimeters(210.0), 210000.0));
}

void testToDisplayUnitMillimeter()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Millimeter;
    TDVecUnitFormatter fmt(settings);
    assert(approxEqual(fmt.ToDisplayUnit(1000.0), 1.0));
    assert(approxEqual(fmt.ToDisplayUnit(210000.0), 210.0));
}

void testToDisplayUnitCentimeter()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Centimeter;
    TDVecUnitFormatter fmt(settings);
    assert(approxEqual(fmt.ToDisplayUnit(10000.0), 1.0));
    assert(approxEqual(fmt.ToDisplayUnit(210000.0), 21.0));
}

void testToDisplayUnitInch()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Inch;
    TDVecUnitFormatter fmt(settings);
    assert(approxEqual(fmt.ToDisplayUnit(25400.0), 1.0));
}

void testToDisplayUnitRawVed()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::RawVed;
    TDVecUnitFormatter fmt(settings);
    assert(approxEqual(fmt.ToDisplayUnit(12345.0), 12345.0));
}

void testFromDisplayUnitRoundtrip()
{
    const double testValues[] = {0.0, 1000.0, 50000.0, 210000.0, -7500.0};
    const TDVecDisplayUnit units[] = {
        TDVecDisplayUnit::Millimeter,
        TDVecDisplayUnit::Centimeter,
        TDVecDisplayUnit::Inch,
        TDVecDisplayUnit::RawVed
    };

    for (TDVecDisplayUnit unit : units) {
        TDVecUnitSettings settings;
        settings.displayUnit = unit;
        TDVecUnitFormatter fmt(settings);
        for (double value : testValues) {
            const double roundtrip = fmt.FromDisplayUnit(fmt.ToDisplayUnit(value));
            assert(approxEqual(roundtrip, value));
        }
    }
}

void testFormatCoordinateMillimeter()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Millimeter;
    TDVecUnitFormatter fmt(settings);
    assert(fmt.FormatCoordinate(1000.0) == "1.00 mm");
    assert(fmt.FormatCoordinate(210000.0) == "210.00 mm");
    assert(fmt.FormatCoordinate(0.0) == "0.00 mm");
}

void testFormatCoordinateCentimeter()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Centimeter;
    TDVecUnitFormatter fmt(settings);
    assert(fmt.FormatCoordinate(10000.0) == "1.00 cm");
}

void testFormatCoordinateInch()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Inch;
    TDVecUnitFormatter fmt(settings);
    assert(fmt.FormatCoordinate(25400.0) == "1.00 in");
}

void testFormatCoordinateRawVed()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::RawVed;
    TDVecUnitFormatter fmt(settings);
    assert(fmt.FormatCoordinate(12345.0) == "12345.00 ved");
}

void testFormatLengthIsAbsolute()
{
    TDVecUnitFormatter fmt(TDVecUnitSettings{});
    assert(fmt.FormatLength(-5000.0) == "5.00 mm");
    assert(fmt.FormatLength(5000.0) == "5.00 mm");
}

void testNonDefaultRealUnitsPerMillimeter()
{
    TDVecUnitSettings settings;
    settings.realUnitsPerMillimeter = 500.0;
    settings.displayUnit = TDVecDisplayUnit::Millimeter;
    TDVecUnitFormatter fmt(settings);
    assert(approxEqual(fmt.ToMillimeters(500.0), 1.0));
    assert(approxEqual(fmt.FromMillimeters(1.0), 500.0));
    assert(approxEqual(fmt.ToDisplayUnit(500.0), 1.0));
    assert(fmt.FormatCoordinate(500.0) == "1.00 mm");
}

void testSettingsAccessor()
{
    TDVecUnitSettings settings;
    settings.realUnitsPerMillimeter = 2000.0;
    settings.displayUnit = TDVecDisplayUnit::Inch;
    TDVecUnitFormatter fmt(settings);
    assert(fmt.Settings().realUnitsPerMillimeter == 2000.0);
    assert(fmt.Settings().displayUnit == TDVecDisplayUnit::Inch);
}

} // namespace

int main()
{
    testToMillimetersDefault();
    testFromMillimetersDefault();
    testToDisplayUnitMillimeter();
    testToDisplayUnitCentimeter();
    testToDisplayUnitInch();
    testToDisplayUnitRawVed();
    testFromDisplayUnitRoundtrip();
    testFormatCoordinateMillimeter();
    testFormatCoordinateCentimeter();
    testFormatCoordinateInch();
    testFormatCoordinateRawVed();
    testFormatLengthIsAbsolute();
    testNonDefaultRealUnitsPerMillimeter();
    testSettingsAccessor();
    return 0;
}
