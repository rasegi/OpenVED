#pragma once

#include "vec_graphic_engine.h"
#include "vec_graphic_engine_screen_state.h"
#include "vec_measure_scale.h"

#include <QPixmap>

#include <vector>

class QPainter;

class TDGraphicEngineQt final : public TDGraphicEngine {
public:
    TDGraphicEngineQt();
    ~TDGraphicEngineQt() override = default;

    void SetPainter(QPainter* painter);
    QPainter* Painter() const;
    void SetDeviceMetrics(long width, long height, double dpiX, double dpiY);
    void LoadDefaultNodeImages();
    void SetNodeImages(std::vector<QPixmap> nodeImages);

    TDGraphicEngineScreenState& ScreenState();
    const TDGraphicEngineScreenState& ScreenState() const;

    void DrawEllipse(const TDMatEllipse* pParams, bool bOutLine) override;
    void DrawLine(const TDMatLine* pParams, bool bOutLine) override;
    void DrawRectangle(const TDMatRect* pParams, bool bOutLine) override;
    void DrawPolygon(const TDMatPointsArray* pParams, bool bOutLine) override;
    void DrawLineOutLine(const TDMatLine* pParams) override;
    void DrawConstructPolygon(const TDMatPointsArray* pParams) override;
    void DrawBoxOutLine(TDMatPoint MatPoint1, TDMatPoint MatPoint2) override;
    void DrawRulers(const TDVecMeasureScale& scale, const TDVecUnitFormatter& formatter) override;

    void DrawNode(double x, double y, TDNodeType eNodeType, bool bLock) override;
    void DrawFrame(const TDMatRect* pParams) override;

    long RealToXPos(double x) const override;
    long RealToYPos(double y) const override;
    long RealToXPos(long x) const override;
    long RealToYPos(long y) const override;
    long RealToXVal(double x) const override;
    long RealToYVal(double y) const override;
    long RealToXVal(long x) const override;
    long RealToYVal(long y) const override;

    double ScreenToXPos(long x) const override;
    double ScreenToYPos(long y) const override;
    double ScreenToXVal(long x) const override;
    double ScreenToYVal(long y) const override;

    void SetDrawColor(TDRgbColor nColor) override;
    TDRgbColor GetDrawColor() const override;

    int GetNodeSize() const override;
    void SetNodeSize(int nSize) override;

    bool IsRectVisible(TDMatRect MatRect) const override;
    long GetMouseTolerance() const override;
    double GetMinimumDistance() const override;

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

    void SetMinimumDistance(long nPixels);
    void SetInchPerReal(double nNewInchPerReal);
    double GetInchPerReal() const;

    void SetWorldSpaceRange(const TDCoordinateSystem2D& newRange);
    void SetWorldSpaceRange(double newLeft, double newTop, double newRight, double newBottom);
    void SetWorkSpaceRange(const TDCoordinateSystem2D& newRange);
    void SetWorkSpaceRange(double newLeft, double newTop, double newRight, double newBottom);
    void SetViewRange(const TDCoordinateSystem2D& newRange);
    void SetViewRange(double newLeft, double newTop, double newRight, double newBottom);
    const TDCoordinateSystem2D& GetViewRange() const;
    const TDCoordinateSystem2D& GetWorldSpaceRange() const;
    const TDCoordinateSystem2D& GetWorkSpaceRange() const;

    double GetZoom() const;
    void SetZoom(double nNewZoom);
    void SetZoomAtScreenPoint(double nNewZoom, long x, long y);
    void SetZoom(TDFraction newZoom);
    void SetZoomToRealSize();
    void SetViewSize(TDRecSize recSize);
    void SetViewSize(long width, long height);

    void DrawAuxLineX(long yPos);
    void DrawAuxLineY(long xPos);
    void DrawAuxLine(long x1, long y1, long x2, long y2);
    void DrawGrid(long nWidth, long nHeight, long nDist, long nSubDiv, TDRgbColor nColor, long nResLimit);
    void DrawMouseToleranz(double x, double y);
    void DrawMouseToleranceCross(double x, double y);

private:
    QPen drawPen(bool outline) const;
    QPen auxLinePen() const;
    QPen frameRectPen() const;
    void beginRasterNot();
    void beginRasterNotXorPen();

    TDGraphicEngineScreenState screenState_;
    QPainter* painter_;
    TDRgbColor drawColor_;
    int nodeSize_;
    std::vector<QPixmap> nodeImages_;
};
