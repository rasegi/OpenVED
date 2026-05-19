#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

double atanD2(double y, double x) {
    return std::atan2(y, x) * RAD_TO_DEG;
}

TDMatRect makeFrameFromBounds(double left, double top, double right, double bottom) {
    return {
        {left, top},
        {right, top},
        {right, bottom},
        {left, bottom}
    };
}

double distance(TDMatPoint a, TDMatPoint b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

double distancePointToSegment(TDMatPoint point, TDMatPoint start, TDMatPoint end) {
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= std::numeric_limits<double>::epsilon()) {
        return distance(point, start);
    }

    const double t = std::clamp(
        ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared,
        0.0,
        1.0);
    const TDMatPoint projection{start.x + t * dx, start.y + t * dy};
    return distance(point, projection);
}

double lineAngle(TDMatLine line) {
    double nAngle = 0.0;
    if (line.x2 == line.x1) {
        if (line.y2 > line.y1) {
            nAngle = 90.0;
        } else if (line.y2 < line.y1) {
            nAngle = -90.0;
        } else {
            nAngle = 0.0;
        }
    } else {
        nAngle = RAD_TO_DEG * std::atan2((line.y2 - line.y1), (line.x2 - line.x1));
    }
    return nAngle;
}

TDMatRect frameMidEdges(TDMatRect frame) {
    return {
        {(frame.P1.x + frame.P2.x) / 2.0, (frame.P1.y + frame.P2.y) / 2.0},
        {(frame.P2.x + frame.P3.x) / 2.0, (frame.P2.y + frame.P3.y) / 2.0},
        {(frame.P3.x + frame.P4.x) / 2.0, (frame.P3.y + frame.P4.y) / 2.0},
        {(frame.P4.x + frame.P1.x) / 2.0, (frame.P4.y + frame.P1.y) / 2.0}
    };
}

TDMatPoint frameNode(TDMatRect frame, TDMatRect middle, long node) {
    switch (node) {
    case 0:
        return frame.P1;
    case 1:
        return middle.P1;
    case 2:
        return frame.P2;
    case 3:
        return middle.P2;
    case 4:
        return frame.P3;
    case 5:
        return middle.P3;
    case 6:
        return frame.P4;
    case 7:
        return middle.P4;
    default:
        return {};
    }
}

TDMatPoint frameNode(TDMatRect frame, long node) {
    return frameNode(frame, frameMidEdges(frame), node);
}

bool mouseOnNode(POINT node, long nodeHeight, POINT mouse, long tolerance) {
    return mouse.x >= node.x - nodeHeight - tolerance &&
           mouse.x <= node.x + nodeHeight + tolerance &&
           mouse.y >= node.y - nodeHeight - tolerance &&
           mouse.y <= node.y + nodeHeight + tolerance;
}

bool realPointOnNode(TDMatPoint node, TDMatPoint point, double tolerance) {
    return point.x >= node.x - tolerance &&
           point.x <= node.x + tolerance &&
           point.y >= node.y - tolerance &&
           point.y <= node.y + tolerance;
}

TDMatPoint projectPointOnLine(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point) {
    const double dx = lineEnd.x - lineStart.x;
    const double dy = lineEnd.y - lineStart.y;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= std::numeric_limits<double>::epsilon()) {
        return lineStart;
    }
    const double t = ((point.x - lineStart.x) * dx + (point.y - lineStart.y) * dy) / lengthSquared;
    return {lineStart.x + (t * dx), lineStart.y + (t * dy)};
}

bool horizontalCut(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point, TDMatPoint& result) {
    if (MatBelike2Double(lineStart.y, lineEnd.y, 4)) {
        return false;
    }
    if (MatBelike2Double(lineStart.x, lineEnd.x, 4)) {
        result = {lineStart.x, point.y};
        return true;
    }

    const double m = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x);
    const double b = lineStart.y - (m * lineStart.x);
    result = {(point.y - b) / m, point.y};
    return true;
}

bool verticalCut(TDMatPoint lineStart, TDMatPoint lineEnd, TDMatPoint point, TDMatPoint& result) {
    if (MatBelike2Double(lineStart.x, lineEnd.x, 4)) {
        return false;
    }
    if (MatBelike2Double(lineStart.y, lineEnd.y, 4)) {
        result = {point.x, lineStart.y};
        return true;
    }

    const double m = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x);
    const double b = lineStart.y - (m * lineStart.x);
    result = {point.x, (m * point.x) + b};
    return true;
}

TDMatRect frameFromPoints(const std::vector<TDMatPoint>& points) {
    if (points.empty()) {
        return {};
    }

    double xLeft = points.front().x;
    double yTop = points.front().y;
    double xRight = points.front().x;
    double yBottom = points.front().y;
    for (const TDMatPoint& point : points) {
        xLeft = std::min(xLeft, point.x);
        yTop = std::min(yTop, point.y);
        xRight = std::max(xRight, point.x);
        yBottom = std::max(yBottom, point.y);
    }
    return makeFrameFromBounds(xLeft, yTop, xRight, yBottom);
}

TDMatRect frameFromConturPoints(const std::vector<TDMatConturPoint>& points) {
    std::vector<TDMatPoint> plainPoints;
    plainPoints.reserve(points.size());
    for (const TDMatConturPoint& point : points) {
        plainPoints.push_back({point.x, point.y});
    }
    return frameFromPoints(plainPoints);
}

TDVecLineHitResult hitClosedPolyline(TDMatPoint point, const std::vector<TDMatPoint>& points, double tolerance) {
    if (points.size() < 2 || tolerance < 0.0) {
        return {};
    }

    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < points.size(); ++i) {
        const TDMatPoint start = points[i];
        const TDMatPoint end = points[(i + 1) % points.size()];
        bestDistance = std::min(bestDistance, distancePointToSegment(point, start, end));
    }
    if (bestDistance <= tolerance) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult hitClosedPolyline(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerancePixels) {
    if (!pGE || points.size() < 2 || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < points.size(); ++i) {
        const TDMatPoint start = points[i];
        const TDMatPoint end = points[(i + 1) % points.size()];
        bestDistance = std::min(bestDistance, distancePointToSegment(
            {mouseX, mouseY},
            {static_cast<double>(pGE->RealToXPos(start.x)), static_cast<double>(pGE->RealToYPos(start.y))},
            {static_cast<double>(pGE->RealToXPos(end.x)), static_cast<double>(pGE->RealToYPos(end.y))}));
    }
    if (bestDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult hitOpenPolyline(TDMatPoint point, const std::vector<TDMatPoint>& points, double tolerance) {
    if (points.size() < 2 || tolerance < 0.0) {
        return {};
    }

    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 1; i < points.size(); ++i) {
        bestDistance = std::min(bestDistance, distancePointToSegment(point, points[i - 1], points[i]));
    }
    if (bestDistance <= tolerance) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult hitOpenPolyline(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerancePixels) {
    if (!pGE || points.size() < 2 || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 1; i < points.size(); ++i) {
        const TDMatPoint start = points[i - 1];
        const TDMatPoint end = points[i];
        bestDistance = std::min(bestDistance, distancePointToSegment(
            {mouseX, mouseY},
            {static_cast<double>(pGE->RealToXPos(start.x)), static_cast<double>(pGE->RealToYPos(start.y))},
            {static_cast<double>(pGE->RealToXPos(end.x)), static_cast<double>(pGE->RealToYPos(end.y))}));
    }
    if (bestDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

std::vector<TDMatPoint> polyCurveDrawPoints(const std::vector<TDMatConturPoint>& points, unsigned int resolution) {
    std::vector<TDMatPoint> drawPoints;
    if (points.empty()) {
        return drawPoints;
    }

    drawPoints.push_back({points.front().x, points.front().y});
    const TDMatConturPoint* previous = &points.front();
    for (std::size_t i = 1; i < points.size(); ++i) {
        const TDMatConturPoint* current = &points[i];
        if (current->eType == CPT_PRIME_LINE && previous->eType == CPT_PRIME_LINE) {
            drawPoints.push_back({current->x, current->y});
        } else if (current->eType == CPT_PRIME_QSPLINE && i + 1 < points.size()) {
            const TDMatConturPoint* next = &points[i + 1];
            const TDMatPoint pointA{previous->x, previous->y};
            const TDMatPoint pointB{current->x, current->y};
            const TDMatPoint pointC{next->x, next->y};
            for (unsigned int step = 0; step < resolution; ++step) {
                TDMatPoint qPoint;
                qPoint.x = (((pointA.x - 2 * pointB.x + pointC.x) * step * step) / (resolution * resolution)) +
                           (((2 * pointB.x - 2 * pointA.x) * step) / resolution) + pointA.x;
                qPoint.y = (((pointA.y - 2 * pointB.y + pointC.y) * step * step) / (resolution * resolution)) +
                           (((2 * pointB.y - 2 * pointA.y) * step) / resolution) + pointA.y;
                drawPoints.push_back(qPoint);
            }
            drawPoints.push_back(pointC);
        }
        previous = current;
    }
    return drawPoints;
}

double distancePointToSegment(double pointX, double pointY, double startX, double startY, double endX, double endY) {
    const double dx = endX - startX;
    const double dy = endY - startY;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= std::numeric_limits<double>::epsilon()) {
        return std::hypot(pointX - startX, pointY - startY);
    }

    const double t = std::clamp(
        ((pointX - startX) * dx + (pointY - startY) * dy) / lengthSquared,
        0.0,
        1.0);
    const double projectionX = startX + t * dx;
    const double projectionY = startY + t * dy;
    return std::hypot(pointX - projectionX, pointY - projectionY);
}

TDMatPoint ellipsePoint(double xCenter, double yCenter, double xRadius, double yRadius, double angle, double radians) {
    const double localX = std::cos(radians) * xRadius;
    const double localY = std::sin(radians) * yRadius;
    return {
        cosD(angle) * localX + sinD(angle) * localY + xCenter,
        -sinD(angle) * localX + cosD(angle) * localY + yCenter
    };
}
