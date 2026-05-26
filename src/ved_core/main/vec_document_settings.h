#pragma once

#include "vec_units.h"

#include <string>

enum class TDVecPageOrientation {
    Portrait,
    Landscape
};

struct TDVecGridSettings {
    double majorStepReal = 10000.0;
    int subdivisions = 10;
    long resolutionLimitPixels = 1;
};

struct TDVecPageSettings {
    std::string formatName = "A4";
    double widthReal = 210000.0;
    double heightReal = 297000.0;
    double pageOriginX = 0.0;
    double pageOriginY = 0.0;
    TDVecPageOrientation orientation = TDVecPageOrientation::Portrait;
};

struct TDVecDocumentSettings {
    TDVecUnitSettings unitSettings;
    TDVecGridSettings gridSettings;
    TDVecPageSettings pageSettings;
};

class TDVecPageFormats {
public:
    static TDVecPageSettings A4(TDVecPageOrientation orientation);
    static TDVecPageSettings A3(TDVecPageOrientation orientation);
    static TDVecPageSettings A5(TDVecPageOrientation orientation);
    static TDVecPageSettings Letter(TDVecPageOrientation orientation);
    static TDVecPageSettings Custom(std::string name, double widthReal, double heightReal,
                                     TDVecPageOrientation orientation);
};
