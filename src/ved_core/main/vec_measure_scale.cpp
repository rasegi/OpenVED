#include "vec_measure_scale.h"

#include <algorithm>
#include <cmath>

long NiceStepAtLeast(double minimumStep)
{
    if (minimumStep <= 1.0) {
        return 1;
    }

    const double exponent = std::floor(std::log10(minimumStep));
    const double base = std::pow(10.0, exponent);
    for (const double multiplier : {1.0, 2.0, 5.0, 10.0}) {
        const double candidate = multiplier * base;
        if (candidate >= minimumStep) {
            return static_cast<long>(std::ceil(candidate));
        }
    }

    return static_cast<long>(std::ceil(10.0 * base));
}

TDVecMeasureScaleCalculator::TDVecMeasureScaleCalculator(TDVecUnitSettings unitSettings)
    : formatter_(unitSettings)
{
}

TDVecMeasureScale TDVecMeasureScaleCalculator::CalculateForView(
    double realPerPixel,
    long minimumPixelDistance,
    double preferredMajorStepReal,
    int subdivisions) const
{
    const double minStepByPixel = std::fabs(realPerPixel) * static_cast<double>(std::max(1L, minimumPixelDistance));
    const double minorFromPreferred = (subdivisions > 0)
        ? preferredMajorStepReal / static_cast<double>(subdivisions)
        : preferredMajorStepReal;
    const double minimumStep = std::max(minorFromPreferred, minStepByPixel);
    const long minorStep = NiceStepAtLeast(minimumStep);
    const long majorStep = std::max(static_cast<long>(preferredMajorStepReal), minorStep);

    TDVecMeasureScale scale;
    scale.minorStepReal = static_cast<double>(minorStep);
    scale.majorStepReal = static_cast<double>(majorStep);
    scale.labelStepReal = scale.majorStepReal;
    return scale;
}

std::vector<TDVecMeasureTick> TDVecMeasureScaleCalculator::BuildTicks(
    double visibleMinReal,
    double visibleMaxReal,
    const TDVecMeasureScale& scale) const
{
    std::vector<TDVecMeasureTick> ticks;

    if (scale.minorStepReal <= 0.0 || visibleMinReal >= visibleMaxReal) {
        return ticks;
    }

    const long step = static_cast<long>(scale.minorStepReal);
    if (step <= 0) {
        return ticks;
    }

    const long majorStep = static_cast<long>(scale.majorStepReal);
    const long labelStep = static_cast<long>(scale.labelStepReal);

    const long first = static_cast<long>(std::floor(visibleMinReal / static_cast<double>(step))) * step;
    const long last = static_cast<long>(std::ceil(visibleMaxReal / static_cast<double>(step))) * step;

    ticks.reserve(static_cast<std::size_t>((last - first) / step + 1));

    for (long value = first; value <= last; value += step) {
        TDVecMeasureTick tick;
        tick.realValue = static_cast<double>(value);
        tick.major = (majorStep > 0 && value % majorStep == 0);
        tick.labeled = (labelStep > 0 && value % labelStep == 0);
        if (tick.labeled) {
            tick.label = formatter_.FormatCoordinate(tick.realValue);
        }
        ticks.push_back(std::move(tick));
    }

    return ticks;
}
