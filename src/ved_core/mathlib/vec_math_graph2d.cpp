//---------------------------------------------------------------------------
#include <cmath>
#include <limits>

#include "vec_math_graph2d.h"


const double MAX_REAL_VALUE = std::sqrt(std::sqrt(std::numeric_limits<double>::max()));        // vierte Wurzel aus der größtmöglichen double-Zahl
const double MIN_REAL_VALUE = -MAX_REAL_VALUE;              // ist gleich -MAX_REAL_VALUE
const double MIN_POSITIV_REAL_VALUE =  (1/MAX_REAL_VALUE);  // Wenn ein positiver Wert diesen unterschreitet, wird sie als 0.0 interpretiert.
const double MAX_NEGATIV_REAL_VALUE = -(1/MAX_REAL_VALUE);  // Wenn ein negativer Wert diesen  überschreitet, wird sie als 0.0 interpretiert.

const double INCH_PER_MICROMETER          =        0.001 / 25.4; //  1µm sind   0.001/25.4 In
const double INCH_PER_TEN_MICROMETER      =        0.01  / 25.4;
const double INCH_PER_HUNDRED_MICROMETER  =        0.1   / 25.4;
const double INCH_PER_MILIMETER           =        1.0   / 25.4; //  1mm sind       1/25.4 In
const double INCH_PER_CENTIMETER          =       10.0   / 25.4;
const double INCH_PER_DECIMETER           =      100.0   / 25.4;
const double INCH_PER_METER               =     1000.0   / 25.4; //  1m  sind    1000/25.4 In
const double INCH_PER_TEN_METER           =    10000.0   / 25.4;
const double INCH_PER_HUNDRED_METER       =   100000.0   / 25.4;
const double INCH_PER_KILOMETER           =  1000000.0   / 25.4; //  1km sind 1000000/25.4 In

const TDCoordinateSystem2D MAX_RANGE_2D = // Definiert den größtmöglichen 2D-Vektorraum
                                 {
                                     MIN_REAL_VALUE,
                                     MAX_REAL_VALUE,
                                     MIN_REAL_VALUE,
                                     MAX_REAL_VALUE
                                 };
const TDCoordinateSystem2D DEFAULT_WORLDSPACE_RANGE = {-4000000, 3000000, -4000000, 3000000};
const TDCoordinateSystem2D DEFAULT_WORKSPACE_RANGE  = {0.0, 0.0, 0.21, 0.2969848}; // in Meter: Din-A4
const TDCoordinateSystem2D DEFAULT_VIEW_RANGE       = {0.0, 0.0, 0.21, 0.2969848}; // in Meter: Din-A4
const TDRecSize            DEFAULT_CLIENTSIZE       = {32, 32};
//---------------------------------------------------------------------------
// EOF
//---------------------------------------------------------------------------
