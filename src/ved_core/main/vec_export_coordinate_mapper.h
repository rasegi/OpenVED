#pragma once

#include "vec_document_settings.h"
#include "vec_units.h"

struct TDVecExportSettings {
    double marginTopMm = 0.0;
    double marginBottomMm = 0.0;
    double marginLeftMm = 0.0;
    double marginRightMm = 0.0;
    double scaleFactor = 1.0;
};

class TDVecExportCoordinateMapper {
public:
    TDVecExportCoordinateMapper(const TDVecUnitSettings& unitSettings,
                                 const TDVecPageSettings& pageSettings);
    TDVecExportCoordinateMapper(const TDVecUnitSettings& unitSettings,
                                 const TDVecPageSettings& pageSettings,
                                 const TDVecExportSettings& exportSettings);

    double RealToMillimeters(double realValue) const;
    double MillimetersToPoints(double mm) const;
    double RealToPoints(double realValue) const;

    double MapX(double realX) const;
    double MapY(double realY) const;

    double PageWidthPoints() const;
    double PageHeightPoints() const;
    double PageWidthMm() const;
    double PageHeightMm() const;

private:
    TDVecUnitFormatter formatter_;
    TDVecPageSettings pageSettings_;
    TDVecExportSettings exportSettings_;
    double pageHeightPoints_;
};
