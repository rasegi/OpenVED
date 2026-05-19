#include "vec_math_base.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <utility>

namespace {

void InsertPointToArray(TDPointArray* pEllipseArray, int x, int y, bool bRx) {
    (void)bRx;
    if (!pEllipseArray) {
        return;
    }

    POINT newPoint;
    if (pEllipseArray->empty()) {
        newPoint.x = x;
        newPoint.y = y;
        pEllipseArray->push_back(newPoint);
        return;
    }

    POINT& pEP = pEllipseArray->back();
    if (pEllipseArray->size() > 2) {
        POINT& pEP1 = (*pEllipseArray)[pEllipseArray->size() - 2];
        if (pEP.y == y) {
            pEP.x = x;
        }
        if (pEP.x == x) {
            pEP.y = y;
        }
        if ((pEP.x != x) && (pEP.y != y)) {
            if ((x == pEP.x - 1) && (y == pEP.y - 1)) {
                if ((pEP1.x == pEP.x + 1) && (pEP1.y == pEP.y + 1)) {
                    pEP.x = x;
                    pEP.y = y;
                    return;
                }
            }
            if ((x == pEP.x + 1) && (y == pEP.y - 1)) {
                if ((pEP1.x == pEP.x - 1) && (pEP1.y == pEP.y + 1)) {
                    pEP.x = x;
                    pEP.y = y;
                    return;
                }
            }
            if ((x == pEP.x + 1) && (y == pEP.y + 1)) {
                if ((pEP1.x == pEP.x - 1) && (pEP1.y == pEP.y - 1)) {
                    pEP.x = x;
                    pEP.y = y;
                    return;
                }
            }
            if ((x == pEP.x - 1) && (y == pEP.y + 1)) {
                if ((pEP1.x == pEP.x + 1) && (pEP1.y == pEP.y - 1)) {
                    pEP.x = x;
                    pEP.y = y;
                    return;
                }
            }
            newPoint.x = x;
            newPoint.y = y;
            pEllipseArray->push_back(newPoint);
        }
    } else {
        newPoint.x = x;
        newPoint.y = y;
        pEllipseArray->push_back(newPoint);
    }
}

} // namespace

long MatRound(double value) {
    constexpr double longMax = static_cast<double>(LONG_MAX);
    constexpr double longMin = static_cast<double>(LONG_MIN);
    double iV1;
    double iV2;
    long iResult;
    double dN;

    if (value >= longMax) {
        value = longMax;
    }
    if (value <= longMin) {
        value = longMin;
    }

    iV1 = std::floor(value);
    dN = value - iV1;
    dN = dN * 10;
    iV2 = std::floor(dN);
    if (iV2 >= 5) {
        if ((iV1 + 1) <= longMax) {
            iResult = static_cast<long>(iV1 + 1);
        } else {
            iResult = LONG_MAX;
        }
    } else {
        if ((iV1 + 1) <= longMax) {
            iResult = static_cast<long>(iV1);
        } else {
            iResult = LONG_MAX;
        }
    }

    return iResult;
}

bool MatBelike2Double(double dValue1, double dValue2, unsigned short int nToleranz) {
    bool bResult = false;
    if ((dValue1 < 0 && dValue2 > 0) || (dValue1 > 0 && dValue2 < 0)) {
        return bResult;
    }
    if ((dValue1 == 0 && dValue2 != 0) || (dValue1 != 0 && dValue2 == 0)) {
        return bResult;
    }

    if (dValue1 < 0) {
        dValue1 = (-1) * dValue1;
    }
    if (dValue2 < 0) {
        dValue2 = (-1) * dValue2;
    }
    double dDiff = dValue1 - dValue2;
    if (dDiff < 0) {
        dDiff = (-1) * dDiff;
    }
    if (dDiff != 0) {
        double dDiffPow = dDiff * std::pow(10, nToleranz);
        double lDiffPowFloor = std::floor(dDiffPow);
        if (lDiffPowFloor == 0.0) {
            bResult = true;
        } else {
            bResult = false;
        }
    } else {
        bResult = true;
    }

    return bResult;
}

char Signum(double nX) {
    if (nX > 0.0) {
        return 1;
    }
    if (nX < 0.0) {
        return -1;
    }
    return 0;
}

void SwapReal(double& a, double& b) {
    std::swap(a, b);
}

void TDMatEllipse::CreateDiagonal(TDMatPoint topLeft, TDMatPoint bottomRight) {
    xCenter = (topLeft.x + bottomRight.x) / 2.0;
    yCenter = (topLeft.y + bottomRight.y) / 2.0;
    xRadius = std::fabs(topLeft.x - bottomRight.x) / 2.0;
    yRadius = std::fabs(topLeft.y - bottomRight.y) / 2.0;
    nAngle = 0.0;
}

void TDMatEllipse::CreateCircleDiagonal(TDMatPoint topLeft, TDMatPoint bottomRight) {
    const double width = std::fabs(topLeft.x - bottomRight.x);
    const double height = std::fabs(topLeft.y - bottomRight.y);
    const double radius = std::max(width, height) / 2.0;
    xCenter = (topLeft.x + bottomRight.x) / 2.0;
    yCenter = (topLeft.y + bottomRight.y) / 2.0;
    xRadius = radius;
    yRadius = radius;
    nAngle = 0.0;
}

bool TDMatEllipse::IsDiagonal() const {
    bool bResult;
    double dSin = sinD(nAngle);
    double dCos = cosD(nAngle);

    if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0 || dSin == 1.0 || dSin == -1.0 || dCos == 0.0) {
        bResult = true;
    } else {
        if (MatBelike2Double(xRadius, yRadius, 4)) {
            bResult = true;
        } else {
            bResult = false;
        }
    }
    return bResult;
}

double MatDistance2Point(TDMatPoint p1, TDMatPoint p2) {
    return std::hypot(p1.x - p2.x, p1.y - p2.y);
}

double MatDistanceLinePoint(const TDMatLine& line, TDMatPoint point) {
    double m1;
    double m2;
    double b1;
    double b2;
    double x3;
    double y3;
    double result;

    m1 = (line.y2 - line.y1) / (line.x2 - line.x1);
    m2 = (-1.0 / m1);
    b1 = (line.y1 - (m1 * line.x1));
    b2 = ((point.x / m1) + point.y);
    x3 = ((b2 - b1) / (m1 - m2));
    y3 = ((m1 * x3) + b1);
    result = std::sqrt(std::pow((x3 - point.x), 2) + std::pow((y3 - point.y), 2));
    return result;
}

void MatEllipseMidpoint(TDMatEllipse* pE, TDPointArray* pData) {
    double ERx;
    double ERy;
    double winkel;
    if (!pE || !pData) {
        return;
    }

    if (pE->xRadius <= pE->yRadius) {
        ERx = pE->xRadius;
        ERy = pE->yRadius;
        winkel = ((-1) * pE->nAngle);
    } else {
        ERx = pE->yRadius;
        ERy = pE->xRadius;
        winkel = 90 - (pE->nAngle);
    }
    double rx2 = ERx * ERx;
    double ry2 = ERy * ERy;
    double twoRx2 = 2 * rx2;
    double twoRy2 = 2 * ry2;
    double p;
    double x = 0;
    double y = ERy;
    double px = 0;
    double py = twoRx2 * y;
    POINT point;
    bool bRx;

    if (pE->xRadius < pE->yRadius) {
        bRx = false;
    } else {
        bRx = true;
    }

    TDPointArray ellipseArray;
    ellipseArray.reserve(1000);
    InsertPointToArray(&ellipseArray, MatRound(x), MatRound(y), bRx);

    p = ry2 - (rx2 * ERy) + (0.25 * rx2);
    while (px < py) {
        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
        InsertPointToArray(&ellipseArray, MatRound(x), MatRound(y), bRx);
    }

    p = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y > 0) {
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
        InsertPointToArray(&ellipseArray, MatRound(x), MatRound(y), bRx);
    }

    for (std::size_t i = 0; i < ellipseArray.size(); i++) {
        const POINT& pPA = ellipseArray[i];
        point.x = MatRound(((pPA.x) * cosD(winkel)) - ((pPA.y) * sinD(winkel)));
        point.y = MatRound(((pPA.x) * sinD(winkel)) + ((pPA.y) * cosD(winkel)));
        point.x = static_cast<long>(pE->xCenter + point.x);
        point.y = static_cast<long>(pE->yCenter + point.y);
        pData->push_back(point);
    }
    for (int i = static_cast<int>(ellipseArray.size()) - 1; i >= 0; i--) {
        const POINT& pPA = ellipseArray[static_cast<std::size_t>(i)];
        point.x = MatRound(((pPA.x) * cosD(winkel)) - ((-(pPA.y)) * sinD(winkel)));
        point.y = MatRound(((pPA.x) * sinD(winkel)) + ((-(pPA.y)) * cosD(winkel)));
        point.x = static_cast<long>(pE->xCenter + point.x);
        point.y = static_cast<long>(pE->yCenter + point.y);
        pData->push_back(point);
    }
    for (std::size_t i = 0; i < ellipseArray.size(); i++) {
        const POINT& pPA = ellipseArray[i];
        point.x = MatRound(((-(pPA.x)) * cosD(winkel)) - ((-(pPA.y)) * sinD(winkel)));
        point.y = MatRound(((-(pPA.x)) * sinD(winkel)) + ((-(pPA.y)) * cosD(winkel)));
        point.x = static_cast<long>(pE->xCenter + point.x);
        point.y = static_cast<long>(pE->yCenter + point.y);
        pData->push_back(point);
    }
    for (int i = static_cast<int>(ellipseArray.size()) - 1; i >= 0; i--) {
        const POINT& pPA = ellipseArray[static_cast<std::size_t>(i)];
        point.x = MatRound(((-(pPA.x)) * cosD(winkel)) - ((pPA.y) * sinD(winkel)));
        point.y = MatRound(((-(pPA.x)) * sinD(winkel)) + ((pPA.y) * cosD(winkel)));
        point.x = static_cast<long>(pE->xCenter + point.x);
        point.y = static_cast<long>(pE->yCenter + point.y);
        pData->push_back(point);
    }
}

TDMatEllipse MatEllipseMitte3PointReal(TDMatPoint center, TDMatPoint axis, TDMatPoint point) {
    TDMatEllipse ellipse;
    double rx;
    double ry;
    double winkelDeg;

    rx = std::sqrt(std::pow((center.x - axis.x), 2) + std::pow((center.y - axis.y), 2));
    if (center.x == axis.x || center.y == axis.y) {
        if (center.x == axis.x) {
            winkelDeg = 90.0;
        }
        if (center.y == axis.y) {
            winkelDeg = 0.0;
        }
    } else {
        winkelDeg = -1.0 * std::atan2((axis.y - center.y), (axis.x - center.x));
        winkelDeg = winkelDeg * RAD_TO_DEG;
    }

    if (winkelDeg == 0.0 || winkelDeg == 90.0) {
        if (winkelDeg == 0.0) {
            ry = point.y - axis.y;
        }
        if (winkelDeg == 90.0) {
            ry = point.x - axis.x;
        }
    } else {
        TDMatPoint pl1;
        TDMatPoint pl2;
        TDMatPoint p;
        TDMatLine matLine;
        pl1.x = center.x;
        pl1.y = center.y;
        pl2.x = axis.x;
        pl2.y = axis.y;
        p.x = point.x;
        p.y = point.y;
        matLine.Create(pl1, pl2);
        ry = MatDistanceLinePoint(matLine, p);
    }

    ellipse.xCenter = center.x;
    ellipse.yCenter = center.y;
    ellipse.xRadius = std::abs(rx);
    ellipse.yRadius = std::abs(ry);
    ellipse.nAngle = winkelDeg;
    return ellipse;
}

TDMatCircle MatCircleMitteReal(TDMatPoint center, TDMatPoint point) {
    return {center.x, center.y, MatDistance2Point(center, point)};
}

TDMatCircle MatCircleDurchReal(TDMatPoint center, TDMatPoint point) {
    const TDMatPoint midpoint{(center.x + point.x) / 2.0, (center.y + point.y) / 2.0};
    return {midpoint.x, midpoint.y, MatDistance2Point(midpoint, point)};
}

bool MatCircleKanteReal(TDMatPoint p1, TDMatPoint p2, TDMatPoint p3, TDMatCircle* circle) {
    if (!circle) {
        return false;
    }

    long double iRadius;
    double x = 0.0;
    double y = 0.0;
    long double radius;
    TDMatPoint center;
    double a;
    double b;
    double k1;
    double c;
    double d;
    double k2;

    a = 2.0 * (p2.x - p1.x);
    b = 2.0 * (p2.y - p1.y);
    k1 = (p1.x * p1.x) + (p1.y * p1.y) - (p2.x * p2.x) - (p2.y * p2.y);
    c = 2.0 * (p2.x - p3.x);
    d = 2.0 * (p2.y - p3.y);
    k2 = (p3.x * p3.x) + (p3.y * p3.y) - (p2.x * p2.x) - (p2.y * p2.y);

    if (std::abs(p1.y - p2.y) > 1.0) {
        if (((c * b) - (a * d)) != 0.0) {
            x = ((d * k1) - (b * k2)) / ((c * b) - (a * d));
            center.x = (((d * k1) - (b * k2)) / ((c * b) - (a * d)));
        }

        if (b != 0.0) {
            y = ((a * center.x) + k1) / (-b);
            center.y = (((a * center.x) + k1) / (-b));
        }
    } else {
        return false;
    }

    radius = std::sqrt(std::pow(static_cast<long double>(x - p1.x), 2) +
                       std::pow(static_cast<long double>(y - p1.y), 2));
    iRadius = static_cast<double>(radius);
    circle->xCenter = center.x;
    circle->yCenter = center.y;
    circle->r = static_cast<double>(iRadius);
    return true;
}

void computeCoefficients(int n, int* c) {
    int k;
    int i;

    for (k = 0; k <= n; k++) {
        c[k] = 1;
        for (i = n; i >= k + 1; i--) {
            c[k] *= i;
        }
        for (i = n - k; i >= 2; i--) {
            c[k] /= i;
        }
    }
}

void computePoint(float u, TDMatPoint* pt, int nControls, const TDMatPointsArray* controls, int* c) {
    int k;
    int n = nControls - 1;
    double blend;

    if (!pt || !controls) {
        return;
    }

    pt->x = 0.0;
    pt->y = 0.0;

    for (k = 0; k < nControls; k++) {
        const TDMatPoint* pMatPoint = &(*controls)[static_cast<std::size_t>(k)];
        if ((u != 0 || k != 0) && ((1 - u) != 0 || (n - k) != 0)) {
            blend = c[k] * std::pow(static_cast<double>(u), static_cast<double>(k)) *
                    std::pow(static_cast<double>(1 - u), static_cast<double>(n - k));
        } else {
            blend = 1;
        }

        pt->x += pMatPoint->x * blend;
        pt->y += pMatPoint->y * blend;
    }
}

void Bezier(const TDMatPointsArray* controls, int m, TDMatPointsArray* curve) {
    if (!controls || !curve || controls->empty() || m <= 0) {
        return;
    }

    int nControls = static_cast<int>(controls->size());
    std::vector<int> c(static_cast<std::size_t>(nControls));
    computeCoefficients(nControls - 1, c.data());

    for (int i = 0; i <= m; i++) {
        TDMatPoint matPoint;
        computePoint(i / static_cast<float>(m), &matPoint, nControls, controls, c.data());
        curve->push_back(matPoint);
    }
}
