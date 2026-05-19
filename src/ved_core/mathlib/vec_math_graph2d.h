//---------------------------------------------------------------------------
#ifndef __VEC_MATH_GRAPH2D_H
#define __VEC_MATH_GRAPH2D_H
//---------------------------------------------------------------------------
extern const double MAX_REAL_VALUE;  // vierte Wurzel aus der größtmöglichen double-Zahl
extern const double MIN_REAL_VALUE;  // ist gleich -MAX_REAL_VALUE
extern const double MAX_NEGATIV_REAL_VALUE; // Wenn ein negativer Wert diesen  überschreitet, wird sie als 0.0 interpretiert.
extern const double MIN_POSITIV_REAL_VALUE; // Wenn ein positiver Wert diesen unterschreitet, wird sie als 0.0 interpretiert.

extern const double INCH_PER_MICROMETER        ;
extern const double INCH_PER_TEN_MICROMETER    ;
extern const double INCH_PER_HUNDRED_MICROMETER;
extern const double INCH_PER_MILIMETER         ;
extern const double INCH_PER_CENTIMETER        ;
extern const double INCH_PER_DECIMETER         ;
extern const double INCH_PER_METER             ;
extern const double INCH_PER_TEN_METER         ;
extern const double INCH_PER_HUNDRED_METER     ;
extern const double INCH_PER_KILOMETER         ;
#define SetRealInInch(x)       SetInchPerReal(x)
#define SetRealInKiloeter(x)   SetInchPerReal((x) * INCH_PER_KILOMETER )
#define SetRealInMeter(x)      SetInchPerReal((x) * INCH_PER_METER     )
#define SetRealInMilimeter(x)  SetInchPerReal((x) * INCH_PER_MILIMETER )
#define SetRealInMicrometer(x) SetInchPerReal((x) * INCH_PER_MICROMETER)

typedef struct TDFraction
{
    double mnCounter;
    double mnDenominator;
    inline double AsDouble()
    {
        if((mnDenominator>0.0)||(mnDenominator<0.0))
        {
            return (double)(mnCounter/mnDenominator);
        }else
        {
            return 0.0;
        }
    };
} TDFraction;
typedef int TDGEScrollBarValue;
enum TDGEScrollCommand
{
    GE_SB_LINEUP          = 0,
    GE_SB_LINELEFT        = 0,
    GE_SB_LINEDOWN        = 1,
    GE_SB_LINERIGHT       = 1,
    GE_SB_PAGEUP          = 2,
    GE_SB_PAGELEFT        = 2,
    GE_SB_PAGEDOWN        = 3,
    GE_SB_PAGERIGHT       = 3,
    GE_SB_THUMBPOSITION   = 4,
    GE_SB_THUMBTRACK      = 5,
    GE_SB_TOP             = 6,
    GE_SB_LEFT            = 6,
    GE_SB_BOTTOM          = 7,
    GE_SB_RIGHT           = 7,
    GE_SB_ENDSCROLL       = 8
};
enum TDGEScrollType
{
    GE_SB_HORZ = 0,
    GE_SB_VERT = 1,
    GE_SB_CTL  = 2,
    GE_SB_BOTH = 3
};
// Scroll-Kommunikation
typedef struct
{
    TDGEScrollType       mScrollType;
    TDGEScrollCommand    mScrollCommand;
    TDGEScrollBarValue   mMin;
    TDGEScrollBarValue   mMax;
    TDGEScrollBarValue   mPos;
    long                 mnScrollPixels;
    bool                 mbScrollPixelsOutOfRange;
} TDGEScrollData;
// Real-Koordinatensystem (2 dimensional)
typedef struct
{
    double Left;
    double Top;
    double Right;
    double Bottom;
} TDCoordinateSystem2D;
extern const TDCoordinateSystem2D MAX_RANGE_2D;
extern const TDCoordinateSystem2D DEFAULT_VIEW_RANGE;
extern const TDCoordinateSystem2D DEFAULT_WORLDSPACE_RANGE;
extern const TDCoordinateSystem2D DEFAULT_WORKSPACE_RANGE;
typedef struct
{
    long Width;
    long Height;
} TDRecSize;
extern const TDRecSize DEFAULT_CLIENTSIZE;
#endif
