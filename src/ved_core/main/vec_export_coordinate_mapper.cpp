#include "vec_export_coordinate_mapper.h"

namespace {
constexpr double kPointsPerInch = 72.0;
constexpr double kMillimetersPerInch = 25.4;
} // namespace

TDVecExportCoordinateMapper::TDVecExportCoordinateMapper(
    const TDVecUnitSettings& unitSettings,
    const TDVecPageSettings& pageSettings)
    : TDVecExportCoordinateMapper(unitSettings, pageSettings, {})
{
}

TDVecExportCoordinateMapper::TDVecExportCoordinateMapper(
    const TDVecUnitSettings& unitSettings,
    const TDVecPageSettings& pageSettings,
    const TDVecExportSettings& exportSettings)
    : formatter_(unitSettings),
      pageSettings_(pageSettings),
      exportSettings_(exportSettings),
      pageHeightPoints_(MillimetersToPoints(formatter_.ToMillimeters(pageSettings_.heightReal)))
{
}

double TDVecExportCoordinateMapper::RealToMillimeters(double realValue) const {
    return formatter_.ToMillimeters(realValue);
}

double TDVecExportCoordinateMapper::MillimetersToPoints(double mm) const {
    return mm / kMillimetersPerInch * kPointsPerInch;
}

double TDVecExportCoordinateMapper::RealToPoints(double realValue) const {
    return MillimetersToPoints(RealToMillimeters(realValue));
}

double TDVecExportCoordinateMapper::MapX(double realX) const {
    const double relativeX = realX - pageSettings_.pageOriginX;
    return RealToPoints(relativeX) * exportSettings_.scaleFactor
        + MillimetersToPoints(exportSettings_.marginLeftMm);
}

double TDVecExportCoordinateMapper::MapY(double realY) const {
    const double relativeY = realY - pageSettings_.pageOriginY;
    return pageHeightPoints_ - RealToPoints(relativeY) * exportSettings_.scaleFactor
        + MillimetersToPoints(exportSettings_.marginBottomMm);
}

double TDVecExportCoordinateMapper::PageWidthPoints() const {
    return MillimetersToPoints(PageWidthMm());
}

double TDVecExportCoordinateMapper::PageHeightPoints() const {
    return pageHeightPoints_;
}

double TDVecExportCoordinateMapper::PageWidthMm() const {
    return formatter_.ToMillimeters(pageSettings_.widthReal);
}

double TDVecExportCoordinateMapper::PageHeightMm() const {
    return formatter_.ToMillimeters(pageSettings_.heightReal);
}
