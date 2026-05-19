//------------------------------------------------------------------------------------
// HEADER: Geometrischae Datentypen
//
// Qt port note:
// For the first ved_core GraphicEngine step it intentionally keeps only the
// geometry types required by TDGraphicEngine and the screen-engine state.
// The full mathlib port will replace this reduced slice when vecobjects and
// operations are moved into ved_core.
//------------------------------------------------------------------------------------
#ifndef __VEC_MATH_BASE_H
#define __VEC_MATH_BASE_H

#include <cmath>
#include <vector>

#include "vec_maindef.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD  (M_PI / 180.0)
#define RAD_TO_DEG  (180.0 / M_PI)
#define cosD(x)     cos((x) * DEG_TO_RAD)
#define sinD(x)     sin((x) * DEG_TO_RAD)
#define tanD(x)     tan((x) * DEG_TO_RAD)

struct POINT {
    long x;
    long y;
};

using TDPointArray = std::vector<POINT>;

struct TDMatPoint {
    double x = 0.0;
    double y = 0.0;

    void Create(double newX, double newY) {
        x = newX;
        y = newY;
    }

    void Create(const TDMatPoint* point) {
        if (!point) {
            return;
        }
        x = point->x;
        y = point->y;
    }
};

using TDMatPointsArray = std::vector<TDMatPoint>;

struct TDMatEllipse {
    double xCenter = 0.0;
    double yCenter = 0.0;
    double xRadius = 0.0;
    double yRadius = 0.0;
    double nAngle = 0.0;

    void CreateDiagonal(TDMatPoint topLeft, TDMatPoint bottomRight);
    void CreateCircleDiagonal(TDMatPoint topLeft, TDMatPoint bottomRight);
    bool IsDiagonal() const;
};

struct TDMatCircle {
    double xCenter = 0.0;
    double yCenter = 0.0;
    double r = 0.0;
};

struct TDMatLine {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;

    void InitialWithNULL() {
        x1 = 0.0;
        y1 = 0.0;
        x2 = 0.0;
        y2 = 0.0;
    }

    void Create(TDMatPoint start, TDMatPoint end) {
        x1 = start.x;
        y1 = start.y;
        x2 = end.x;
        y2 = end.y;
    }

    void Create(const TDMatLine* line) {
        if (!line) {
            return;
        }
        x1 = line->x1;
        y1 = line->y1;
        x2 = line->x2;
        y2 = line->y2;
    }

    double Length() const {
        return std::hypot(x2 - x1, y2 - y1);
    }
};

struct TDMatRect {
    TDMatPoint P1;
    TDMatPoint P2;
    TDMatPoint P3;
    TDMatPoint P4;
};

long MatRound(double value);
bool MatBelike2Double(double dValue1, double dValue2, unsigned short int nToleranz);
char Signum(double nX);
void SwapReal(double& a, double& b);
double MatDistance2Point(TDMatPoint p1, TDMatPoint p2);
double MatDistanceLinePoint(const TDMatLine& line, TDMatPoint point);
void MatEllipseMidpoint(TDMatEllipse* pE, TDPointArray* pData);
TDMatEllipse MatEllipseMitte3PointReal(TDMatPoint center, TDMatPoint axis, TDMatPoint point);
TDMatCircle MatCircleMitteReal(TDMatPoint center, TDMatPoint point);
TDMatCircle MatCircleDurchReal(TDMatPoint center, TDMatPoint point);
bool MatCircleKanteReal(TDMatPoint p1, TDMatPoint p2, TDMatPoint p3, TDMatCircle* circle);
void computeCoefficients(int n, int* c);
void computePoint(float u, TDMatPoint* pt, int nControls, const TDMatPointsArray* controls, int* c);
void Bezier(const TDMatPointsArray* controls, int m, TDMatPointsArray* curve);

#endif

//------------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------------
