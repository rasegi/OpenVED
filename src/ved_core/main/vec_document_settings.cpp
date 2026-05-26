#include "vec_document_settings.h"

#include <utility>

namespace {

constexpr double kRealPerMm = 1000.0;

TDVecPageSettings makePageSettings(std::string name,
                                    double shortSideMm,
                                    double longSideMm,
                                    TDVecPageOrientation orientation)
{
    TDVecPageSettings page;
    page.formatName = std::move(name);
    page.orientation = orientation;
    if (orientation == TDVecPageOrientation::Landscape) {
        page.widthReal = longSideMm * kRealPerMm;
        page.heightReal = shortSideMm * kRealPerMm;
    } else {
        page.widthReal = shortSideMm * kRealPerMm;
        page.heightReal = longSideMm * kRealPerMm;
    }
    return page;
}

} // namespace

TDVecPageSettings TDVecPageFormats::A4(TDVecPageOrientation orientation)
{
    return makePageSettings("A4", 210.0, 297.0, orientation);
}

TDVecPageSettings TDVecPageFormats::A3(TDVecPageOrientation orientation)
{
    return makePageSettings("A3", 297.0, 420.0, orientation);
}

TDVecPageSettings TDVecPageFormats::A5(TDVecPageOrientation orientation)
{
    return makePageSettings("A5", 148.0, 210.0, orientation);
}

TDVecPageSettings TDVecPageFormats::Letter(TDVecPageOrientation orientation)
{
    return makePageSettings("Letter", 215.9, 279.4, orientation);
}

TDVecPageSettings TDVecPageFormats::Custom(std::string name, double widthReal, double heightReal,
                                            TDVecPageOrientation orientation)
{
    TDVecPageSettings page;
    page.formatName = std::move(name);
    page.widthReal = widthReal;
    page.heightReal = heightReal;
    page.orientation = orientation;
    return page;
}
