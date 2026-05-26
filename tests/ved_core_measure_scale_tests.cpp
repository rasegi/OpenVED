#include "vec_measure_scale.h"

#include <cassert>
#include <cmath>

namespace {

constexpr double kEpsilon = 1e-9;

bool approxEqual(double a, double b)
{
    return std::fabs(a - b) < kEpsilon;
}

// --- NiceStepAtLeast ---

void testNiceStepAtLeastSmallValues()
{
    assert(NiceStepAtLeast(0.5) == 1);
    assert(NiceStepAtLeast(1.0) == 1);
    assert(NiceStepAtLeast(-1.0) == 1);
}

void testNiceStepAtLeastRoundValues()
{
    assert(NiceStepAtLeast(100.0) == 100);
    assert(NiceStepAtLeast(1000.0) == 1000);
    assert(NiceStepAtLeast(10000.0) == 10000);
}

void testNiceStepAtLeastBetweenValues()
{
    assert(NiceStepAtLeast(150.0) == 200);
    assert(NiceStepAtLeast(250.0) == 500);
    assert(NiceStepAtLeast(350.0) == 500);
    assert(NiceStepAtLeast(550.0) == 1000);
    assert(NiceStepAtLeast(1500.0) == 2000);
    assert(NiceStepAtLeast(3000.0) == 5000);
}

// --- CalculateForView ---

void testCalculateForViewHighZoom()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    const TDVecMeasureScale scale = calc.CalculateForView(
        1.0, 1, 10000.0, 10);

    assert(approxEqual(scale.minorStepReal, 1000.0));
    assert(approxEqual(scale.majorStepReal, 10000.0));
}

void testCalculateForViewLowZoom()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    const TDVecMeasureScale scale = calc.CalculateForView(
        50.0, 1, 10000.0, 10);

    assert(scale.minorStepReal >= 1000.0);
    assert(scale.majorStepReal >= scale.minorStepReal);
}

void testCalculateForViewVeryLowZoom()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    const TDVecMeasureScale scale = calc.CalculateForView(
        500.0, 1, 10000.0, 10);

    assert(scale.minorStepReal >= 1000.0);
    assert(scale.majorStepReal >= 10000.0);
}

void testCalculateForViewMinorNeverExceedsMajor()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    const double testRealPerPixel[] = {0.1, 1.0, 10.0, 50.0, 200.0, 1000.0};
    for (double rpp : testRealPerPixel) {
        const TDVecMeasureScale scale = calc.CalculateForView(
            rpp, 1, 10000.0, 10);
        assert(scale.minorStepReal <= scale.majorStepReal);
    }
}

// --- BuildTicks ---

void testBuildTicksBasic()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    TDVecMeasureScale scale;
    scale.minorStepReal = 1000.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(0.0, 10000.0, scale);
    assert(!ticks.empty());
    assert(ticks.size() == 11);
}

void testBuildTicksMajorAtCorrectPositions()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    TDVecMeasureScale scale;
    scale.minorStepReal = 1000.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(0.0, 20000.0, scale);
    int majorCount = 0;
    for (const auto& tick : ticks) {
        if (tick.major) {
            long value = static_cast<long>(tick.realValue);
            assert(value % 10000 == 0);
            ++majorCount;
        }
    }
    assert(majorCount == 3);
}

void testBuildTicksLabelsNotEmpty()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    TDVecMeasureScale scale;
    scale.minorStepReal = 1000.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(0.0, 10000.0, scale);
    for (const auto& tick : ticks) {
        if (tick.labeled) {
            assert(!tick.label.empty());
        }
    }
}

void testBuildTicksLabelsContainSuffix()
{
    TDVecUnitSettings settings;
    settings.displayUnit = TDVecDisplayUnit::Millimeter;
    TDVecMeasureScaleCalculator calc(settings);
    TDVecMeasureScale scale;
    scale.minorStepReal = 10000.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(0.0, 10000.0, scale);
    bool foundLabel = false;
    for (const auto& tick : ticks) {
        if (tick.labeled) {
            assert(tick.label.find("mm") != std::string::npos);
            foundLabel = true;
        }
    }
    assert(foundLabel);
}

void testBuildTicksEmptyRange()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    TDVecMeasureScale scale;
    scale.minorStepReal = 1000.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(5000.0, 5000.0, scale);
    assert(ticks.empty());
}

void testBuildTicksInvalidStep()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    TDVecMeasureScale scale;
    scale.minorStepReal = 0.0;
    scale.majorStepReal = 10000.0;
    scale.labelStepReal = 10000.0;

    const auto ticks = calc.BuildTicks(0.0, 10000.0, scale);
    assert(ticks.empty());
}

void testBuildTicksDefaultMatchesLegacyBehavior()
{
    TDVecMeasureScaleCalculator calc(TDVecUnitSettings{});
    const TDVecMeasureScale scale = calc.CalculateForView(
        1.0, 1, 10000.0, 10);

    assert(approxEqual(scale.minorStepReal, 1000.0));
    assert(approxEqual(scale.majorStepReal, 10000.0));

    const auto ticks = calc.BuildTicks(0.0, 10000.0, scale);
    assert(ticks.size() == 11);

    assert(ticks[0].major);
    assert(approxEqual(ticks[0].realValue, 0.0));
    assert(!ticks[1].major);
    assert(approxEqual(ticks[1].realValue, 1000.0));
    assert(ticks[10].major);
    assert(approxEqual(ticks[10].realValue, 10000.0));
}

} // namespace

int main()
{
    testNiceStepAtLeastSmallValues();
    testNiceStepAtLeastRoundValues();
    testNiceStepAtLeastBetweenValues();
    testCalculateForViewHighZoom();
    testCalculateForViewLowZoom();
    testCalculateForViewVeryLowZoom();
    testCalculateForViewMinorNeverExceedsMajor();
    testBuildTicksBasic();
    testBuildTicksMajorAtCorrectPositions();
    testBuildTicksLabelsNotEmpty();
    testBuildTicksLabelsContainSuffix();
    testBuildTicksEmptyRange();
    testBuildTicksInvalidStep();
    testBuildTicksDefaultMatchesLegacyBehavior();
    return 0;
}
