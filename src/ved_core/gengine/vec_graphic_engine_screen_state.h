#pragma once

#include "vec_math_base.h"
#include "vec_math_graph2d.h"

class TDGraphicEngineScreenState {
public:
    TDGraphicEngineScreenState();

    long RealToXPos(double x) const;
    long RealToYPos(double y) const;
    long RealToXPos(long x) const;
    long RealToYPos(long y) const;
    long RealToXVal(double x) const;
    long RealToYVal(double y) const;
    long RealToXVal(long x) const;
    long RealToYVal(long y) const;

    double ScreenToXPos(long x) const;
    double ScreenToYPos(long y) const;
    double ScreenToXVal(long x) const;
    double ScreenToYVal(long y) const;

    bool IsRectVisible(TDMatRect matRect) const;

    void SetPanX(long nPanX);
    void SetPanY(long nPanY);
    void SetPanAllowingWorldExpansion(long nPanX, long nPanY);
    void ChangePanX(long xDiff);
    void ChangePanY(long yDiff);
    long GetPanX() const;
    long GetPanY() const;

    double GetMarginX() const;
    double GetMarginY() const;
    void SetMarginX(double x);
    void SetMarginY(double y);

    long GetMouseTolerance() const;
    double GetMinimumDistance() const;
    void SetMinimumDistance(long nPixels);

    void SetInchPerReal(double nNewInchPerReal);
    double GetInchPerReal() const;

    void SetDpi(long dpiX, long dpiY);

    void SetWorldSpaceRange(const TDCoordinateSystem2D& newRange);
    void SetWorldSpaceRange(double newLeft, double newTop, double newRight, double newBottom);

    void SetWorkSpaceRange(const TDCoordinateSystem2D& newRange);
    void SetWorkSpaceRange(double newLeft, double newTop, double newRight, double newBottom);

    void SetViewRange(const TDCoordinateSystem2D& newRange);
    void SetViewRange(double newLeft, double newTop, double newRight, double newBottom);

    double GetZoom() const;
    void SetZoom(double nNewZoom);
    void SetZoomAtScreenPoint(double nNewZoom, long x, long y);
    void SetZoom(TDFraction newZoom);
    void SetZoomToRealSize();

    void SetViewSize(TDRecSize recSize);
    void SetViewSize(long width, long height);

    const TDCoordinateSystem2D& GetViewRange() const;
    const TDCoordinateSystem2D& GetWorldSpaceRange() const;
    const TDCoordinateSystem2D& GetWorkSpaceRange() const;

private:
    void AdjustCompatibleValues();
    void AdjustNewValues();
    void NormalizeViewProportion();

    TDRecSize clientSize_;
    TDCoordinateSystem2D realViewRange_;
    TDCoordinateSystem2D realWorldSpaceRange_;
    TDCoordinateSystem2D realWorkSpaceRange_;
    double realPerPixelX_;
    double realPerPixelY_;
    double zoom_;
    TDFraction zoomFraction_;
    double inchPerReal_;
    long dpiX_;
    long dpiY_;
    long mouseTolerance_;
    long minDistPixels_;
    double marginX_;
    double marginY_;
    long panX_;
    long panY_;
};
