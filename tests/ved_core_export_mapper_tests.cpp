#include "vec_export_coordinate_mapper.h"

#include <cassert>
#include <cmath>

namespace {

constexpr double kEpsilon = 0.01;

bool approx(double a, double b) {
    return std::fabs(a - b) < kEpsilon;
}

// A4 Portrait: 210 x 297 mm = 210000 x 297000 real (at 1000 real/mm)
// 210 mm = 595.28 pt, 297 mm = 841.89 pt

void testA4PageSizePoints() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.PageWidthPoints(), 595.28));
    assert(approx(mapper.PageHeightPoints(), 841.89));
    assert(approx(mapper.PageWidthMm(), 210.0));
    assert(approx(mapper.PageHeightMm(), 297.0));
}

void testA3PageSizePoints() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A3(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.PageWidthMm(), 297.0));
    assert(approx(mapper.PageHeightMm(), 420.0));
    assert(approx(mapper.PageWidthPoints(), 841.89));
    assert(approx(mapper.PageHeightPoints(), 1190.55));
}

void testLetterPageSizePoints() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::Letter(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.PageWidthMm(), 215.9));
    assert(approx(mapper.PageHeightMm(), 279.4));
    // Letter: 8.5 x 11 in = 612 x 792 pt
    assert(approx(mapper.PageWidthPoints(), 612.0));
    assert(approx(mapper.PageHeightPoints(), 792.0));
}

void testRealToMillimeters() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.RealToMillimeters(1000.0), 1.0));
    assert(approx(mapper.RealToMillimeters(210000.0), 210.0));
}

void testRealToPoints() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    // 25.4mm = 1in = 72pt → 25400 real = 72pt
    assert(approx(mapper.RealToPoints(25400.0), 72.0));
}

void testMapXOriginAtZero() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.MapX(0.0), 0.0));
    // 105mm = half A4 width = 105000 real → 297.64 pt
    assert(approx(mapper.MapX(105000.0), 297.64));
}

void testMapYFlipped() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);

    TDVecExportCoordinateMapper mapper(units, page);
    // Y=0 → top of page → PDF bottom = pageHeight
    assert(approx(mapper.MapY(0.0), 841.89));
    // Y=297000 (bottom of page) → PDF top = 0
    assert(approx(mapper.MapY(297000.0), 0.0));
}

void testMapWithPageOrigin() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);
    page.pageOriginX = 10000.0;
    page.pageOriginY = 20000.0;

    TDVecExportCoordinateMapper mapper(units, page);
    // Real X=10000 is at page left → PDF X=0
    assert(approx(mapper.MapX(10000.0), 0.0));
    // Real Y=20000 is at page top → PDF Y=pageHeight
    assert(approx(mapper.MapY(20000.0), 841.89));
}

void testMapWithScaleFactor() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);
    TDVecExportSettings exportSettings;
    exportSettings.scaleFactor = 2.0;

    TDVecExportCoordinateMapper mapper(units, page, exportSettings);
    // X=1000 real = 1mm = 2.835pt, scaled 2x = 5.67pt
    assert(approx(mapper.MapX(1000.0), 5.67));
}

void testMapWithMargins() {
    TDVecUnitSettings units;
    TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);
    TDVecExportSettings exportSettings;
    exportSettings.marginLeftMm = 10.0;
    exportSettings.marginBottomMm = 15.0;

    TDVecExportCoordinateMapper mapper(units, page, exportSettings);
    // X=0 with 10mm left margin → 10mm = 28.35pt
    assert(approx(mapper.MapX(0.0), 28.35));
    // Y at bottom of page with 15mm bottom margin
    assert(approx(mapper.MapY(297000.0), 42.52));
}

void testNonDefaultRealUnitsPerMm() {
    TDVecUnitSettings units;
    units.realUnitsPerMillimeter = 500.0;
    TDVecPageSettings page;
    page.widthReal = 105000.0;
    page.heightReal = 148500.0;

    TDVecExportCoordinateMapper mapper(units, page);
    assert(approx(mapper.PageWidthMm(), 210.0));
    assert(approx(mapper.PageHeightMm(), 297.0));
}

} // namespace

int main() {
    testA4PageSizePoints();
    testA3PageSizePoints();
    testLetterPageSizePoints();
    testRealToMillimeters();
    testRealToPoints();
    testMapXOriginAtZero();
    testMapYFlipped();
    testMapWithPageOrigin();
    testMapWithScaleFactor();
    testMapWithMargins();
    testNonDefaultRealUnitsPerMm();
    return 0;
}
