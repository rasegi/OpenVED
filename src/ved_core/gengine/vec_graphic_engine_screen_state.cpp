#include "vec_graphic_engine_screen_state.h"

#include <algorithm>
#include <cmath>

TDGraphicEngineScreenState::TDGraphicEngineScreenState()
    : clientSize_(DEFAULT_CLIENTSIZE),
      realViewRange_{0.0, 0.0, 0.0, 0.0},
      realWorldSpaceRange_{0.0, 0.0, 0.0, 0.0},
      realWorkSpaceRange_{0.0, 0.0, 0.0, 0.0},
      realPerPixelX_(0.0),
      realPerPixelY_(0.0),
      zoom_(1.0),
      zoomFraction_{1.0, 1.0},
      inchPerReal_(INCH_PER_MICROMETER),
      dpiX_(96),
      dpiY_(96),
      mouseTolerance_(5),
      minDistPixels_(2),
      marginX_(0.0),
      marginY_(0.0),
      panX_(0),
      panY_(0) {
    SetWorkSpaceRange(DEFAULT_WORKSPACE_RANGE);
    SetWorldSpaceRange(DEFAULT_WORLDSPACE_RANGE);
    SetViewRange(DEFAULT_VIEW_RANGE);
    SetZoomToRealSize();
}

long TDGraphicEngineScreenState::RealToXPos(double x) const {
    return MatRound((x - realViewRange_.Left) / realPerPixelX_);
}

long TDGraphicEngineScreenState::RealToYPos(double y) const {
    return MatRound((y - realViewRange_.Top) / realPerPixelY_);
}

long TDGraphicEngineScreenState::RealToXPos(long x) const {
    return MatRound((static_cast<double>(x) - realViewRange_.Left) / realPerPixelX_);
}

long TDGraphicEngineScreenState::RealToYPos(long y) const {
    return MatRound((static_cast<double>(y) - realViewRange_.Top) / realPerPixelY_);
}

long TDGraphicEngineScreenState::RealToXVal(double x) const {
    return MatRound(x / std::fabs(realPerPixelX_));
}

long TDGraphicEngineScreenState::RealToYVal(double y) const {
    return MatRound(y / std::fabs(realPerPixelY_));
}

long TDGraphicEngineScreenState::RealToXVal(long x) const {
    return MatRound(static_cast<double>(x) / std::fabs(realPerPixelX_));
}

long TDGraphicEngineScreenState::RealToYVal(long y) const {
    return MatRound(static_cast<double>(y) / std::fabs(realPerPixelY_));
}

double TDGraphicEngineScreenState::ScreenToXPos(long x) const {
    return realViewRange_.Left + static_cast<double>(x) * realPerPixelX_;
}

double TDGraphicEngineScreenState::ScreenToYPos(long y) const {
    return realViewRange_.Top + static_cast<double>(y) * realPerPixelY_;
}

double TDGraphicEngineScreenState::ScreenToXVal(long x) const {
    return static_cast<double>(x) * std::fabs(realPerPixelX_);
}

double TDGraphicEngineScreenState::ScreenToYVal(long y) const {
    return static_cast<double>(y) * std::fabs(realPerPixelY_);
}

bool TDGraphicEngineScreenState::IsRectVisible(TDMatRect matRect) const {
    const double rectLeft = std::min({matRect.P1.x, matRect.P2.x, matRect.P3.x, matRect.P4.x});
    const double rectRight = std::max({matRect.P1.x, matRect.P2.x, matRect.P3.x, matRect.P4.x});
    const double rectTop = std::min({matRect.P1.y, matRect.P2.y, matRect.P3.y, matRect.P4.y});
    const double rectBottom = std::max({matRect.P1.y, matRect.P2.y, matRect.P3.y, matRect.P4.y});
    const double viewLeft = std::min(realViewRange_.Left, realViewRange_.Right);
    const double viewRight = std::max(realViewRange_.Left, realViewRange_.Right);
    const double viewTop = std::min(realViewRange_.Top, realViewRange_.Bottom);
    const double viewBottom = std::max(realViewRange_.Top, realViewRange_.Bottom);

    return rectRight >= viewLeft && rectLeft <= viewRight && rectBottom >= viewTop && rectTop <= viewBottom;
}

void TDGraphicEngineScreenState::SetPanX(long nPanX) {
    panX_ = nPanX;
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetPanY(long nPanY) {
    panY_ = nPanY;
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetPanAllowingWorldExpansion(long nPanX, long nPanY) {
    if (std::fabs(realPerPixelX_) <= 0.0 || std::fabs(realPerPixelY_) <= 0.0) {
        SetPanX(nPanX);
        SetPanY(nPanY);
        return;
    }

    const double desiredLeft = realWorldSpaceRange_.Left + realPerPixelX_ * static_cast<double>(nPanX);
    const double desiredTop = realWorldSpaceRange_.Top + realPerPixelY_ * static_cast<double>(nPanY);
    const double desiredRight = desiredLeft + static_cast<double>(clientSize_.Width) * realPerPixelX_;
    const double desiredBottom = desiredTop + static_cast<double>(clientSize_.Height) * realPerPixelY_;

    if (realWorldSpaceRange_.Left <= realWorldSpaceRange_.Right) {
        realWorldSpaceRange_.Left = std::min(realWorldSpaceRange_.Left, desiredLeft);
        realWorldSpaceRange_.Right = std::max(realWorldSpaceRange_.Right, desiredRight);
    } else {
        realWorldSpaceRange_.Left = std::max(realWorldSpaceRange_.Left, desiredLeft);
        realWorldSpaceRange_.Right = std::min(realWorldSpaceRange_.Right, desiredRight);
    }
    if (realWorldSpaceRange_.Top <= realWorldSpaceRange_.Bottom) {
        realWorldSpaceRange_.Top = std::min(realWorldSpaceRange_.Top, desiredTop);
        realWorldSpaceRange_.Bottom = std::max(realWorldSpaceRange_.Bottom, desiredBottom);
    } else {
        realWorldSpaceRange_.Top = std::max(realWorldSpaceRange_.Top, desiredTop);
        realWorldSpaceRange_.Bottom = std::min(realWorldSpaceRange_.Bottom, desiredBottom);
    }

    marginX_ = std::max(
        std::fabs(realWorldSpaceRange_.Left - realWorkSpaceRange_.Left),
        std::fabs(realWorldSpaceRange_.Right - realWorkSpaceRange_.Right));
    marginY_ = std::max(
        std::fabs(realWorldSpaceRange_.Top - realWorkSpaceRange_.Top),
        std::fabs(realWorldSpaceRange_.Bottom - realWorkSpaceRange_.Bottom));
    AdjustNewValues();

    panX_ = MatRound((desiredLeft - realWorldSpaceRange_.Left) / realPerPixelX_);
    panY_ = MatRound((desiredTop - realWorldSpaceRange_.Top) / realPerPixelY_);
    AdjustNewValues();
}

void TDGraphicEngineScreenState::ChangePanX(long xDiff) {
    panX_ += xDiff;
    AdjustNewValues();
}

void TDGraphicEngineScreenState::ChangePanY(long yDiff) {
    panY_ += yDiff;
    AdjustNewValues();
}

long TDGraphicEngineScreenState::GetPanX() const {
    return panX_;
}

long TDGraphicEngineScreenState::GetPanY() const {
    return panY_;
}

double TDGraphicEngineScreenState::GetMarginX() const {
    return marginX_;
}

double TDGraphicEngineScreenState::GetMarginY() const {
    return marginY_;
}

void TDGraphicEngineScreenState::SetMarginX(double x) {
    marginX_ = x;
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetMarginY(double y) {
    marginY_ = y;
    AdjustNewValues();
}

long TDGraphicEngineScreenState::GetMouseTolerance() const {
    return mouseTolerance_;
}

double TDGraphicEngineScreenState::GetMinimumDistance() const {
    return (ScreenToXVal(minDistPixels_) + ScreenToYVal(minDistPixels_)) / 2.0;
}

void TDGraphicEngineScreenState::SetMinimumDistance(long nPixels) {
    if (nPixels >= 0) {
        minDistPixels_ = nPixels;
    }
}

void TDGraphicEngineScreenState::SetInchPerReal(double nNewInchPerReal) {
    inchPerReal_ = nNewInchPerReal;
}

double TDGraphicEngineScreenState::GetInchPerReal() const {
    return inchPerReal_;
}

void TDGraphicEngineScreenState::SetDpi(long dpiX, long dpiY) {
    dpiX_ = std::max(1L, dpiX);
    dpiY_ = std::max(1L, dpiY);
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetWorldSpaceRange(const TDCoordinateSystem2D& newRange) {
    SetWorldSpaceRange(newRange.Left, newRange.Top, newRange.Right, newRange.Bottom);
}

void TDGraphicEngineScreenState::SetWorldSpaceRange(double newLeft, double newTop, double newRight, double newBottom) {
    realWorldSpaceRange_ = {newLeft, newTop, newRight, newBottom};
    AdjustCompatibleValues();
}

void TDGraphicEngineScreenState::SetWorkSpaceRange(const TDCoordinateSystem2D& newRange) {
    SetWorkSpaceRange(newRange.Left, newRange.Top, newRange.Right, newRange.Bottom);
}

void TDGraphicEngineScreenState::SetWorkSpaceRange(double newLeft, double newTop, double newRight, double newBottom) {
    realWorkSpaceRange_ = {newLeft, newTop, newRight, newBottom};
    AdjustCompatibleValues();
}

void TDGraphicEngineScreenState::SetViewRange(const TDCoordinateSystem2D& newRange) {
    SetViewRange(newRange.Left, newRange.Top, newRange.Right, newRange.Bottom);
}

void TDGraphicEngineScreenState::SetViewRange(double newLeft, double newTop, double newRight, double newBottom) {
    if (std::fabs(newRight - newLeft) < MIN_POSITIV_REAL_VALUE) {
        if (newRight < newLeft) {
            newRight = newLeft - std::fabs(DEFAULT_VIEW_RANGE.Right - DEFAULT_VIEW_RANGE.Left);
        } else {
            newRight = newLeft + std::fabs(DEFAULT_VIEW_RANGE.Right - DEFAULT_VIEW_RANGE.Left);
        }
    }

    if (std::fabs(newTop - newBottom) < MIN_POSITIV_REAL_VALUE) {
        if (newBottom < newTop) {
            newBottom = newTop - std::fabs(DEFAULT_VIEW_RANGE.Bottom - DEFAULT_VIEW_RANGE.Top);
        } else {
            newTop = newBottom + std::fabs(DEFAULT_VIEW_RANGE.Bottom - DEFAULT_VIEW_RANGE.Top);
        }
    }

    realViewRange_ = {newLeft, newTop, newRight, newBottom};
    realPerPixelX_ = (realViewRange_.Right - realViewRange_.Left) / static_cast<double>(clientSize_.Width);
    realPerPixelY_ = (realViewRange_.Bottom - realViewRange_.Top) / static_cast<double>(clientSize_.Height);
    NormalizeViewProportion();
    zoom_ = std::fabs(1.0 / (inchPerReal_ * static_cast<double>(dpiX_) * realPerPixelX_));
    zoomFraction_ = {zoom_, 1.0};
    AdjustCompatibleValues();
}

double TDGraphicEngineScreenState::GetZoom() const {
    return zoom_;
}

void TDGraphicEngineScreenState::SetZoom(double nNewZoom) {
    if (nNewZoom <= 0.0) {
        return;
    }

    zoom_ = nNewZoom;
    zoomFraction_ = {nNewZoom, 1.0};
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetZoomAtScreenPoint(double nNewZoom, long x, long y) {
    if (nNewZoom <= 0.0) {
        return;
    }

    const double anchorX = ScreenToXPos(x);
    const double anchorY = ScreenToYPos(y);

    zoom_ = nNewZoom;
    zoomFraction_ = {nNewZoom, 1.0};
    AdjustNewValues();

    const double requiredLeft = anchorX - static_cast<double>(x) * realPerPixelX_;
    const double requiredRight = requiredLeft + static_cast<double>(clientSize_.Width) * realPerPixelX_;
    const double requiredTop = anchorY - static_cast<double>(y) * realPerPixelY_;
    const double requiredBottom = requiredTop + static_cast<double>(clientSize_.Height) * realPerPixelY_;

    if (realWorldSpaceRange_.Left <= realWorldSpaceRange_.Right) {
        realWorldSpaceRange_.Left = std::min(realWorldSpaceRange_.Left, requiredLeft);
        realWorldSpaceRange_.Right = std::max(realWorldSpaceRange_.Right, requiredRight);
    } else {
        realWorldSpaceRange_.Left = std::max(realWorldSpaceRange_.Left, requiredLeft);
        realWorldSpaceRange_.Right = std::min(realWorldSpaceRange_.Right, requiredRight);
    }
    if (realWorldSpaceRange_.Top <= realWorldSpaceRange_.Bottom) {
        realWorldSpaceRange_.Top = std::min(realWorldSpaceRange_.Top, requiredTop);
        realWorldSpaceRange_.Bottom = std::max(realWorldSpaceRange_.Bottom, requiredBottom);
    } else {
        realWorldSpaceRange_.Top = std::max(realWorldSpaceRange_.Top, requiredTop);
        realWorldSpaceRange_.Bottom = std::min(realWorldSpaceRange_.Bottom, requiredBottom);
    }
    marginX_ = std::max(
        std::fabs(realWorldSpaceRange_.Left - realWorkSpaceRange_.Left),
        std::fabs(realWorldSpaceRange_.Right - realWorkSpaceRange_.Right));
    marginY_ = std::max(
        std::fabs(realWorldSpaceRange_.Top - realWorkSpaceRange_.Top),
        std::fabs(realWorldSpaceRange_.Bottom - realWorkSpaceRange_.Bottom));
    AdjustNewValues();

    if (std::fabs(realPerPixelX_) > 0.0) {
        panX_ = MatRound(((anchorX - realWorldSpaceRange_.Left) / realPerPixelX_) - static_cast<double>(x));
    }
    if (std::fabs(realPerPixelY_) > 0.0) {
        panY_ = MatRound(((anchorY - realWorldSpaceRange_.Top) / realPerPixelY_) - static_cast<double>(y));
    }
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetZoom(TDFraction newZoom) {
    zoomFraction_ = newZoom;
    if (std::fabs(zoomFraction_.mnDenominator) < MIN_POSITIV_REAL_VALUE) {
        zoomFraction_.mnDenominator = zoomFraction_.mnDenominator < 0.0 ? -MIN_POSITIV_REAL_VALUE : MIN_POSITIV_REAL_VALUE;
    }

    zoom_ = zoomFraction_.mnCounter / zoomFraction_.mnDenominator / 1000.0;
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetZoomToRealSize() {
    zoom_ = 1.0;
    zoomFraction_ = {1.0, 1.0};
    AdjustNewValues();
}

void TDGraphicEngineScreenState::SetViewSize(TDRecSize recSize) {
    SetViewSize(recSize.Width, recSize.Height);
}

void TDGraphicEngineScreenState::SetViewSize(long width, long height) {
    clientSize_.Width = width < 1 ? DEFAULT_CLIENTSIZE.Width : width;
    clientSize_.Height = height < 1 ? DEFAULT_CLIENTSIZE.Height : height;
}

const TDCoordinateSystem2D& TDGraphicEngineScreenState::GetViewRange() const {
    return realViewRange_;
}

const TDCoordinateSystem2D& TDGraphicEngineScreenState::GetWorldSpaceRange() const {
    return realWorldSpaceRange_;
}

const TDCoordinateSystem2D& TDGraphicEngineScreenState::GetWorkSpaceRange() const {
    return realWorkSpaceRange_;
}

void TDGraphicEngineScreenState::AdjustCompatibleValues() {
    marginX_ = std::fabs(realWorldSpaceRange_.Left - realWorkSpaceRange_.Left);
    marginY_ = std::fabs(realWorldSpaceRange_.Top - realWorkSpaceRange_.Top);

    if (std::fabs(realPerPixelX_ * realPerPixelY_) > 0.0) {
        panX_ = MatRound(std::fabs((realViewRange_.Left - realWorldSpaceRange_.Left) / realPerPixelX_));
        panY_ = MatRound(std::fabs((realViewRange_.Top - realWorldSpaceRange_.Top) / realPerPixelY_));
    }
}

void TDGraphicEngineScreenState::AdjustNewValues() {
    if (Signum(realWorldSpaceRange_.Left - realWorldSpaceRange_.Right) *
            Signum(realWorkSpaceRange_.Left - realWorkSpaceRange_.Right) <
        0) {
        SwapReal(realWorldSpaceRange_.Left, realWorldSpaceRange_.Right);
    }

    if (Signum(realWorldSpaceRange_.Top - realWorldSpaceRange_.Bottom) *
            Signum(realWorkSpaceRange_.Top - realWorkSpaceRange_.Bottom) <
        0) {
        SwapReal(realWorldSpaceRange_.Top, realWorldSpaceRange_.Bottom);
    }

    if (realWorldSpaceRange_.Right < realWorldSpaceRange_.Left) {
        realWorldSpaceRange_.Left = realWorkSpaceRange_.Left + marginX_;
        if (realWorldSpaceRange_.Right > realWorldSpaceRange_.Left) {
            realWorldSpaceRange_.Right = realWorldSpaceRange_.Left - MIN_POSITIV_REAL_VALUE;
        }
    } else {
        realWorldSpaceRange_.Left = realWorkSpaceRange_.Left - marginX_;
        if (realWorldSpaceRange_.Right < realWorldSpaceRange_.Left) {
            realWorldSpaceRange_.Right = realWorldSpaceRange_.Left + MIN_POSITIV_REAL_VALUE;
        }
    }

    if (realWorldSpaceRange_.Bottom < realWorldSpaceRange_.Top) {
        realWorldSpaceRange_.Top = realWorkSpaceRange_.Top + marginY_;
        if (realWorldSpaceRange_.Bottom > realWorldSpaceRange_.Top) {
            realWorldSpaceRange_.Bottom = realWorldSpaceRange_.Top - MIN_POSITIV_REAL_VALUE;
        }
    } else {
        realWorldSpaceRange_.Top = realWorkSpaceRange_.Top - marginY_;
        if (realWorldSpaceRange_.Bottom < realWorldSpaceRange_.Top) {
            realWorldSpaceRange_.Bottom = realWorldSpaceRange_.Top + MIN_POSITIV_REAL_VALUE;
        }
    }

    realPerPixelX_ = realWorldSpaceRange_.Left > realWorldSpaceRange_.Right
        ? -1.0 / (inchPerReal_ * static_cast<double>(dpiX_) * zoom_)
        : 1.0 / (inchPerReal_ * static_cast<double>(dpiX_) * zoom_);
    realPerPixelY_ = realWorldSpaceRange_.Top > realWorldSpaceRange_.Bottom
        ? -1.0 / (inchPerReal_ * static_cast<double>(dpiY_) * zoom_)
        : 1.0 / (inchPerReal_ * static_cast<double>(dpiY_) * zoom_);

    realViewRange_.Left = realWorldSpaceRange_.Left + realPerPixelX_ * static_cast<double>(panX_);
    realViewRange_.Top = realWorldSpaceRange_.Top + realPerPixelY_ * static_cast<double>(panY_);
    realViewRange_.Right = realViewRange_.Left + static_cast<double>(clientSize_.Width) * realPerPixelX_;
    realViewRange_.Bottom = realViewRange_.Top + static_cast<double>(clientSize_.Height) * realPerPixelY_;
}

void TDGraphicEngineScreenState::NormalizeViewProportion() {
    if (clientSize_.Width == 0 || clientSize_.Height == 0) {
        return;
    }

    double range = 0.0;
    if (std::fabs(static_cast<double>(dpiX_) * realPerPixelX_) > std::fabs(static_cast<double>(dpiY_) * realPerPixelY_)) {
        realPerPixelY_ = realPerPixelY_ < 0.0
            ? -std::fabs((static_cast<double>(dpiX_) / static_cast<double>(dpiY_)) * realPerPixelX_)
            : std::fabs((static_cast<double>(dpiX_) / static_cast<double>(dpiY_)) * realPerPixelX_);
        range = static_cast<double>(clientSize_.Height) * realPerPixelY_;
        realViewRange_.Top = (realViewRange_.Bottom + realViewRange_.Top - range) / 2.0;
        realViewRange_.Bottom = realViewRange_.Top + range;
    } else {
        realPerPixelX_ = realPerPixelX_ < 0.0
            ? -std::fabs((static_cast<double>(dpiY_) / static_cast<double>(dpiX_)) * realPerPixelY_)
            : std::fabs((static_cast<double>(dpiY_) / static_cast<double>(dpiX_)) * realPerPixelY_);
        range = static_cast<double>(clientSize_.Width) * realPerPixelX_;
        realViewRange_.Left = (realViewRange_.Right + realViewRange_.Left - range) / 2.0;
        realViewRange_.Right = realViewRange_.Left + range;
    }
}
