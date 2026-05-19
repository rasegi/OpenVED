#pragma once

#include "vec_contur_point.h"
#include "vec_object.h"

#include <vector>

constexpr int kEllipseHitSegments = 96;

double atanD2(double y, double x);
TDMatRect makeFrameFromBounds(double left, double top, double right, double bottom);
double distance(TDMatPoint a, TDMatPoint b);
double distancePointToSegment(TDMatPoint point, TDMatPoint start, TDMatPoint end);
double lineAngle(TDMatLine line);
TDMatRect frameMidEdges(TDMatRect frame);
TDMatPoint frameNode(TDMatRect frame, TDMatRect middle, long node);
TDMatPoint frameNode(TDMatRect frame, long node);
bool mouseOnNode(POINT node, long nodeHeight, POINT mouse, long tolerance);
bool realPointOnNode(TDMatPoint node, TDMatPoint point, double tolerance);
TDMatPoint projectPointOnLine(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point);
bool horizontalCut(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point, TDMatPoint& result);
bool verticalCut(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point, TDMatPoint& result);
TDMatRect frameFromPoints(const std::vector<TDMatPoint>& points);
TDMatRect frameFromConturPoints(const std::vector<TDMatConturPoint>& points);
TDVecLineHitResult hitClosedPolyline(TDMatPoint point, const std::vector<TDMatPoint>& points, double tolerance);
TDVecLineHitResult hitClosedPolyline(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerancePixels);
TDVecLineHitResult hitOpenPolyline(TDMatPoint point, const std::vector<TDMatPoint>& points, double tolerance);
TDVecLineHitResult hitOpenPolyline(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerancePixels);
std::vector<TDMatPoint> polyCurveDrawPoints(const std::vector<TDMatConturPoint>& points, unsigned int resolution);
double distancePointToSegment(double pointX, double pointY, double startX, double startY, double endX, double endY);
TDMatPoint ellipsePoint(double xCenter, double yCenter, double xRadius, double yRadius, double angle, double radians);
