#include "vec_graphic_engine_screen_state.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expect(bool condition, const std::string& message) {
    return condition ? 0 : fail(message);
}

void setZoomAroundScreenPoint(TDGraphicEngineScreenState& state, long x, long y, double zoom) {
    state.SetZoomAtScreenPoint(zoom, x, y);
}

} // namespace

int main() {
    TDGraphicEngineScreenState state;
    state.SetViewSize(800, 600);
    state.SetDpi(96, 96);
    state.SetWorkSpaceRange(0.0, 0.0, 210000.0, 296985.0);
    state.SetWorldSpaceRange(-12000.0, -12000.0, 222000.0, 308985.0);
    state.SetViewRange(-12000.0, -12000.0, 222000.0, 308985.0);
    state.SetPanX(120);
    state.SetPanY(80);

    constexpr long mouseX = 321;
    constexpr long mouseY = 219;
    const double beforeX = state.ScreenToXPos(mouseX);
    const double beforeY = state.ScreenToYPos(mouseY);
    const double oldZoom = state.GetZoom();

    setZoomAroundScreenPoint(state, mouseX, mouseY, oldZoom * 1.15);

    const double afterX = state.ScreenToXPos(mouseX);
    const double afterY = state.ScreenToYPos(mouseY);
    const double onePixelX = std::fabs(state.ScreenToXVal(1));
    const double onePixelY = std::fabs(state.ScreenToYVal(1));
    if (int result = expect(std::fabs(beforeX - afterX) <= onePixelX, "zoom changed anchored x coordinate")) {
        return result;
    }
    if (int result = expect(std::fabs(beforeY - afterY) <= onePixelY, "zoom changed anchored y coordinate")) {
        return result;
    }
    if (int result = expect(std::abs(state.RealToXPos(beforeX) - mouseX) <= 1, "anchored x no longer maps to mouse x")) {
        return result;
    }
    if (int result = expect(std::abs(state.RealToYPos(beforeY) - mouseY) <= 1, "anchored y no longer maps to mouse y")) {
        return result;
    }
    if (int result = expect(state.GetPanX() != 120, "zoom did not update horizontal pan")) {
        return result;
    }
    if (int result = expect(state.GetPanY() != 80, "zoom did not update vertical pan")) {
        return result;
    }
    if (int result = expect(state.GetZoom() > oldZoom, "zoom in did not increase zoom")) {
        return result;
    }

    setZoomAroundScreenPoint(state, mouseX, mouseY, state.GetZoom() * 1.15);
    if (int result = expect(std::abs(state.RealToXPos(beforeX) - mouseX) <= 1, "second zoom in lost anchored x")) {
        return result;
    }
    if (int result = expect(std::abs(state.RealToYPos(beforeY) - mouseY) <= 1, "second zoom in lost anchored y")) {
        return result;
    }

    TDGraphicEngineScreenState rightEdgeState;
    rightEdgeState.SetViewSize(800, 600);
    rightEdgeState.SetDpi(96, 96);
    rightEdgeState.SetWorkSpaceRange(0.0, 0.0, 210000.0, 296985.0);
    rightEdgeState.SetWorldSpaceRange(-125000.0, -125000.0, 335000.0, 421985.0);
    rightEdgeState.SetViewRange(-125000.0, -125000.0, 335000.0, 421985.0);
    constexpr long rightMouseX = 400;
    constexpr long rightMouseY = 300;
    constexpr double rightDocumentX = 205000.0;
    constexpr double rightDocumentY = 140000.0;
    rightEdgeState.SetPanX(std::max(0L, rightEdgeState.RealToXPos(rightDocumentX) - rightMouseX));
    rightEdgeState.SetPanY(std::max(0L, rightEdgeState.RealToYPos(rightDocumentY) - rightMouseY));
    const double rightAnchorX = rightEdgeState.ScreenToXPos(rightMouseX);
    const double rightAnchorY = rightEdgeState.ScreenToYPos(rightMouseY);

    rightEdgeState.SetZoomAtScreenPoint(rightEdgeState.GetZoom() * 1.4, rightMouseX, rightMouseY);
    if (int result = expect(std::abs(rightEdgeState.RealToXPos(rightAnchorX) - rightMouseX) <= 1, "right edge zoom in lost anchored x")) {
        return result;
    }
    if (int result = expect(std::abs(rightEdgeState.RealToYPos(rightAnchorY) - rightMouseY) <= 1, "right edge zoom in lost anchored y")) {
        return result;
    }

    TDGraphicEngineScreenState panState;
    panState.SetViewSize(800, 600);
    panState.SetDpi(96, 96);
    panState.SetWorkSpaceRange(0.0, 0.0, 210000.0, 296985.0);
    panState.SetWorldSpaceRange(-125000.0, -125000.0, 335000.0, 421985.0);
    panState.SetViewRange(-125000.0, -125000.0, 335000.0, 421985.0);
    const TDCoordinateSystem2D panStartView = panState.GetViewRange();
    const TDCoordinateSystem2D panStartWorld = panState.GetWorldSpaceRange();
    const double panPerPixelX = (panStartView.Right - panStartView.Left) / 800.0;
    const double panPerPixelY = (panStartView.Bottom - panStartView.Top) / 600.0;
    const long targetPanX = panState.GetPanX() - 350;
    const long targetPanY = panState.GetPanY() - 250;
    const double desiredLeft = panStartWorld.Left + static_cast<double>(targetPanX) * panPerPixelX;
    const double desiredTop = panStartWorld.Top + static_cast<double>(targetPanY) * panPerPixelY;
    panState.SetPanAllowingWorldExpansion(targetPanX, targetPanY);
    if (int result = expect(std::fabs(panState.GetViewRange().Left - desiredLeft) <= std::fabs(panPerPixelX), "free pan left clamped at world edge")) {
        return result;
    }
    if (int result = expect(std::fabs(panState.GetViewRange().Top - desiredTop) <= std::fabs(panPerPixelY), "free pan top clamped at world edge")) {
        return result;
    }

    return 0;
}
