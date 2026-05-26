#pragma once

#include "vec_units.h"

#include <string>
#include <vector>

struct TDVecMeasureTick {
    double realValue = 0.0;
    bool major = false;
    bool labeled = false;
    std::string label;
};

struct TDVecMeasureScale {
    double minorStepReal = 1000.0;
    double majorStepReal = 10000.0;
    double labelStepReal = 10000.0;
};

long NiceStepAtLeast(double minimumStep);

class TDVecMeasureScaleCalculator {
public:
    explicit TDVecMeasureScaleCalculator(TDVecUnitSettings unitSettings);

    TDVecMeasureScale CalculateForView(
        double realPerPixel,
        long minimumPixelDistance,
        double preferredMajorStepReal,
        int subdivisions) const;

    std::vector<TDVecMeasureTick> BuildTicks(
        double visibleMinReal,
        double visibleMaxReal,
        const TDVecMeasureScale& scale) const;

private:
    TDVecUnitFormatter formatter_;
};
