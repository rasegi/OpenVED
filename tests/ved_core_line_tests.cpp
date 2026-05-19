#include "vec_line.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool nearlyEqual(double a, double b) {
    return std::fabs(a - b) < 0.000001;
}

class LinearGraphicEngine final : public TDGraphicEngine {
public:
    void DrawEllipse(const TDMatEllipse*, bool) override {}
    void DrawLine(const TDMatLine*, bool) override {}
    void DrawRectangle(const TDMatRect*, bool) override {}
    void DrawPolygon(const TDMatPointsArray*, bool) override {}
    void DrawLineOutLine(const TDMatLine*) override {}
    void DrawConstructPolygon(const TDMatPointsArray*) override {}
    void DrawBoxOutLine(TDMatPoint, TDMatPoint) override {}
    void DrawNode(double, double, TDNodeType, bool) override {}
    void DrawFrame(const TDMatRect*) override {}

    long RealToXPos(double x) const override { return MatRound(x / realPerPixelX); }
    long RealToYPos(double y) const override { return MatRound(y / realPerPixelY); }
    long RealToXPos(long x) const override { return RealToXPos(static_cast<double>(x)); }
    long RealToYPos(long y) const override { return RealToYPos(static_cast<double>(y)); }
    long RealToXVal(double x) const override { return MatRound(x / realPerPixelX); }
    long RealToYVal(double y) const override { return MatRound(y / realPerPixelY); }
    long RealToXVal(long x) const override { return RealToXVal(static_cast<double>(x)); }
    long RealToYVal(long y) const override { return RealToYVal(static_cast<double>(y)); }
    double ScreenToXPos(long x) const override { return static_cast<double>(x) * realPerPixelX; }
    double ScreenToYPos(long y) const override { return static_cast<double>(y) * realPerPixelY; }
    double ScreenToXVal(long x) const override { return static_cast<double>(x) * realPerPixelX; }
    double ScreenToYVal(long y) const override { return static_cast<double>(y) * realPerPixelY; }

    void SetDrawColor(TDRgbColor color) override { drawColor = color; }
    TDRgbColor GetDrawColor() const override { return drawColor; }
    int GetNodeSize() const override { return 3; }
    void SetNodeSize(int) override {}
    long GetMouseTolerance() const override { return 5; }

private:
    double realPerPixelX = 1.0;
    double realPerPixelY = 10.0;
    TDRgbColor drawColor = 0;
};

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expectHitKind(const TDVecLine& line, TDMatPoint point, double tolerance, TDVecLineHitKind expected, const std::string& label) {
    const TDVecLineHitResult result = line.HitTest(point, tolerance);
    if (result.kind != expected) {
        return fail(label);
    }
    return 0;
}

int expectLine(const TDVecLine& line, double x1, double y1, double x2, double y2, const std::string& label) {
    const TDMatLine actual = line.GetLine();
    if (!nearlyEqual(actual.x1, x1) ||
        !nearlyEqual(actual.y1, y1) ||
        !nearlyEqual(actual.x2, x2) ||
        !nearlyEqual(actual.y2, y2)) {
        return fail(label);
    }
    return 0;
}

} // namespace

int main() {
    TDVecLine line(0.0, 0.0, 100.0, 0.0);

    if (int result = expectHitKind(line, {0.0, 0.0}, 2.0, TDVecLineHitKind::StartNode, "start node hit failed")) {
        return result;
    }
    if (int result = expectHitKind(line, {100.0, 0.0}, 2.0, TDVecLineHitKind::EndNode, "end node hit failed")) {
        return result;
    }
    if (int result = expectHitKind(line, {50.0, 0.0}, 2.0, TDVecLineHitKind::MidpointNode, "midpoint node hit failed")) {
        return result;
    }
    if (int result = expectHitKind(line, {25.0, 1.0}, 2.0, TDVecLineHitKind::Body, "line body hit failed")) {
        return result;
    }
    if (int result = expectHitKind(line, {25.0, 5.0}, 2.0, TDVecLineHitKind::None, "line miss failed")) {
        return result;
    }

    LinearGraphicEngine graphicEngine;
    if (line.HitTest(&graphicEngine, {25.0, 49.0}, graphicEngine.GetMouseTolerance()).kind != TDVecLineHitKind::Body) {
        return fail("screen-space line hit inside visible tolerance failed");
    }
    if (line.HitTest(&graphicEngine, {25.0, 60.0}, graphicEngine.GetMouseTolerance()).IsHit()) {
        return fail("screen-space line miss outside visible tolerance failed");
    }
    if (line.HitTest(&graphicEngine, {0.0, 49.0}, graphicEngine.GetMouseTolerance()).kind != TDVecLineHitKind::StartNode) {
        return fail("screen-space node hit inside visible tolerance failed");
    }

    line.MoveBy(10.0, -5.0);
    if (int result = expectLine(line, 10.0, -5.0, 110.0, -5.0, "MoveBy failed")) {
        return result;
    }

    if (!line.MoveNode(TDVecLineHitKind::StartNode, {1.0, 2.0})) {
        return fail("MoveNode start returned false");
    }
    if (int result = expectLine(line, 1.0, 2.0, 110.0, -5.0, "MoveNode start failed")) {
        return result;
    }

    if (!line.MoveNode(TDVecLineHitKind::EndNode, {120.0, 7.0})) {
        return fail("MoveNode end returned false");
    }
    if (int result = expectLine(line, 1.0, 2.0, 120.0, 7.0, "MoveNode end failed")) {
        return result;
    }

    const TDMatPoint oldMidpoint = line.GetMidpoint();
    if (!line.MoveNode(TDVecLineHitKind::MidpointNode, {oldMidpoint.x + 5.0, oldMidpoint.y + 6.0})) {
        return fail("MoveNode midpoint returned false");
    }
    if (int result = expectLine(line, 6.0, 8.0, 125.0, 13.0, "MoveNode midpoint failed")) {
        return result;
    }

    if (line.MoveNode(TDVecLineHitKind::Body, {0.0, 0.0})) {
        return fail("MoveNode accepted body hit");
    }

    line.SetLockPosition(true);
    if (line.MoveBy(10.0, 10.0)) {
        return fail("MoveBy accepted position-locked line");
    }
    if (line.MoveNode(TDVecLineHitKind::MidpointNode, {50.0, 50.0})) {
        return fail("MoveNode midpoint accepted position-locked line");
    }
    if (int result = expectLine(line, 6.0, 8.0, 125.0, 13.0, "position-locked line changed")) {
        return result;
    }

    line.SetLockPosition(false);
    line.SetLockResize(true);
    if (line.MoveNode(TDVecLineHitKind::StartNode, {50.0, 50.0})) {
        return fail("MoveNode start accepted resize-locked line");
    }
    if (line.ToScale({0.0, 0.0}, 2.0, 2.0)) {
        return fail("ToScale accepted resize-locked line");
    }
    if (int result = expectLine(line, 6.0, 8.0, 125.0, 13.0, "resize-locked line changed")) {
        return result;
    }

    return 0;
}
