#include "vec_edit_cad.h"
#include "vec_bspline.h"
#include "vec_graphic_engine.h"
#include "vec_model.h"
#include "vec_polygon.h"
#include "vec_polycurve.h"
#include "vec_view_interface.h"
#include "voc_polycurve.h"
#include "vop_manager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expect(bool condition, const std::string& message) {
    return condition ? 0 : fail(message);
}

bool nearlyEqual(double a, double b) {
    return std::fabs(a - b) < 0.000001;
}

std::string constructSummary(const std::vector<std::vector<TDMatPoint>>& polygons) {
    std::ostringstream stream;
    stream << "construct polygons:";
    for (const auto& polygon : polygons) {
        stream << " [";
        for (const TDMatPoint& point : polygon) {
            stream << '(' << point.x << ',' << point.y << ')';
        }
        stream << ']';
    }
    return stream.str();
}

class RecordingGraphicEngine : public TDGraphicEngine {
public:
    void DrawEllipse(const TDMatEllipse*, bool) override {}
    void DrawLine(const TDMatLine*, bool) override {
        ++lineDraws;
    }
    void DrawRectangle(const TDMatRect*, bool) override {}
    void DrawPolygon(const TDMatPointsArray* points, bool) override {
        ++polygonDraws;
        lastPolygonPointCount = points ? static_cast<int>(points->size()) : 0;
    }
    void DrawLineOutLine(const TDMatLine*) override {}
    void DrawConstructPolygon(const TDMatPointsArray* points) override {
        ++constructPolygonDraws;
        lastConstructPolygonPoints.clear();
        if (points) {
            lastConstructPolygonPoints.assign(points->begin(), points->end());
            constructPolygons.push_back(lastConstructPolygonPoints);
        }
    }
    void DrawBoxOutLine(TDMatPoint, TDMatPoint) override {}
    void DrawNode(double, double, TDNodeType, bool) override {}
    void DrawFrame(const TDMatRect*) override {}
    long RealToXPos(double x) const override { return MatRound(x); }
    long RealToYPos(double y) const override { return MatRound(y); }
    long RealToXPos(long x) const override { return x; }
    long RealToYPos(long y) const override { return y; }
    long RealToXVal(double x) const override { return MatRound(x); }
    long RealToYVal(double y) const override { return MatRound(y); }
    long RealToXVal(long x) const override { return x; }
    long RealToYVal(long y) const override { return y; }
    double ScreenToXPos(long x) const override { return static_cast<double>(x); }
    double ScreenToYPos(long y) const override { return static_cast<double>(y); }
    double ScreenToXVal(long x) const override { return static_cast<double>(x); }
    double ScreenToYVal(long y) const override { return static_cast<double>(y); }
    void SetDrawColor(TDRgbColor color) override { drawColor = color; }
    TDRgbColor GetDrawColor() const override { return drawColor; }
    int GetNodeSize() const override { return 3; }
    void SetNodeSize(int size) override { nodeSize = size; }
    bool IsRectVisible(TDMatRect) const override { return true; }
    long GetMouseTolerance() const override { return 5; }
    double GetMinimumDistance() const override { return 1.0; }

    int lineDraws = 0;
    int polygonDraws = 0;
    int constructPolygonDraws = 0;
    int lastPolygonPointCount = 0;
    std::vector<TDMatPoint> lastConstructPolygonPoints;
    std::vector<std::vector<TDMatPoint>> constructPolygons;
    int nodeSize = 3;
    TDRgbColor drawColor = 0;
};

class TestViewInterface : public TDVecViewInterfaceBase {
public:
    explicit TestViewInterface(TDGraphicEngine* graphicEngine) {
        SetGraphicEngine(graphicEngine);
    }

    void Send_ViewRefresh_ToView() override {}
    void Send_UseCursor_ToView(TDVecViewCursor) override {}
    double Send_GetZoom_ToView() const override { return 1.0; }
    void Send_SetZoom_ToView(double, const POINT*) override {}
    bool Send_GetGridLock_ToView() const override { return false; }
    bool Send_IsRectVisible_ToView(TDMatRect) const override { return true; }
    long Send_GetMouseTolerance_ToView() const override { return 5; }
    double Send_GetMinimumDistance_ToView() const override { return 1.0; }
    void Send_ZoomInOutEvent_ToView(TDVOPZoomInOutExtVar*) const override {}
    void* GetParent() override { return nullptr; }
};

} // namespace

int main() {
    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_POLYGON_SMARTLINE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_RIGHT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        auto* polygon = dynamic_cast<TDVecPolygon*>(model.GetObject(0));
        if (int result = expect(polygon != nullptr, "polygon smartline did not create TDVecPolygon")) {
            return result;
        }
        if (int result = expect(polygon->GetType() == VECOBJ_POL, "polygon smartline created wrong object type")) {
            return result;
        }
        if (int result = expect(polygon->CountPoints() == 3, "polygon smartline does not have three points")) {
            return result;
        }
        const TDMatPoint* p2 = polygon->GetPoint(1);
        if (int result = expect(p2 && nearlyEqual(p2->x, 100.0) && nearlyEqual(p2->y, 0.0), "polygon second point mismatch")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        RecordingGraphicEngine graphicEngine;
        TestViewInterface view(&graphicEngine);
        editor.RegisterViewInterface(&view);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_BSPLINE_CONTROLPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);

        editor.PaintContentOnView(&view);
        if (int result = expect(graphicEngine.constructPolygonDraws == 1, "bspline first-click preview did not keep a temporary control polygon alive")) {
            return result;
        }
    }

    {
        TDVecModel model;
        auto* bspline = dynamic_cast<TDVecBSPLine*>(model.AppendObject(new TDVecBSPLine()));
        bspline->SetDegree(2);
        bspline->AppendPoint({0.0, 0.0});
        bspline->AppendPoint({100.0, 0.0});
        bspline->AppendPoint({200.0, 0.0});
        bspline->SetShowPolygon(true);
        bspline->SetShowControls(true);
        bspline->SetSelect(true);
        bspline->ComputeCurve();
        bspline->Initialize();

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        RecordingGraphicEngine graphicEngine;
        TestViewInterface view(&graphicEngine);
        editor.RegisterViewInterface(&view);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_INSERT_NODE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 30.0);
        editor.PaintContentOnView(&view);

        const auto previewIt = std::find_if(
            graphicEngine.constructPolygons.begin(),
            graphicEngine.constructPolygons.end(),
            [](const std::vector<TDMatPoint>& points) {
                return points.size() == 4 &&
                       nearlyEqual(points[1].x, 60.0) &&
                       nearlyEqual(points[1].y, 30.0);
            });
        if (int result = expect(previewIt != graphicEngine.constructPolygons.end(),
                                "bspline insert preview moved the wrong node: " + constructSummary(graphicEngine.constructPolygons))) {
            return result;
        }
        const TDMatPoint previewPoint = (*previewIt)[1];

        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 30.0);
        const TDMatPoint* finalPoint = bspline->GetPoint(1);
        if (int result = expect(finalPoint && nearlyEqual(finalPoint->x, previewPoint.x) && nearlyEqual(finalPoint->y, previewPoint.y),
                                "bspline insert preview and final inserted point differ")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        RecordingGraphicEngine graphicEngine;
        TestViewInterface view(&graphicEngine);
        editor.RegisterViewInterface(&view);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_POLYGON_SMARTLINE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        editor.PaintContentOnView(&view);
        if (int result = expect(graphicEngine.polygonDraws == 1, "polygon preview did not draw a temporary polygon")) {
            return result;
        }
        if (int result = expect(graphicEngine.lineDraws == 0, "polygon preview still draws only a temporary line")) {
            return result;
        }
        if (int result = expect(graphicEngine.lastPolygonPointCount == 3, "polygon preview does not include the moving endpoint")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        RecordingGraphicEngine graphicEngine;
        TestViewInterface view(&graphicEngine);
        editor.RegisterViewInterface(&view);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_BEZIERCURVE_CONTROLPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);

        editor.PaintContentOnView(&view);
        if (int result = expect(graphicEngine.polygonDraws == 1, "bezier first-click preview did not keep a temporary polygon alive")) {
            return result;
        }
        if (int result = expect(graphicEngine.lastPolygonPointCount == 1, "bezier first-click preview has wrong point count")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        RecordingGraphicEngine graphicEngine;
        TestViewInterface view(&graphicEngine);
        editor.RegisterViewInterface(&view);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_BEZIERCURVE_CONTROLPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        editor.PaintContentOnView(&view);
        if (int result = expect(graphicEngine.polygonDraws == 1, "bezier moving preview did not draw the control polygon")) {
            return result;
        }
        if (int result = expect(graphicEngine.lineDraws == 0, "bezier moving preview still draws only a temporary line")) {
            return result;
        }
        if (int result = expect(graphicEngine.lastPolygonPointCount == 3, "bezier moving preview does not include fixed points and moving endpoint")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_POLYGON_SMARTLINE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_RIGHT, VKState_KEY_SHIFT, 100.0, 0.0);

        auto* polygon = dynamic_cast<TDVecPolygon*>(model.GetObject(0));
        if (int result = expect(polygon != nullptr, "closed polygon smartline did not create TDVecPolygon")) {
            return result;
        }
        if (int result = expect(polygon->CountPoints() == 3, "closed polygon smartline did not append first point")) {
            return result;
        }
        const TDMatPoint* first = polygon->GetPoint(0);
        const TDMatPoint* last = polygon->GetPoint(2);
        if (int result = expect(first && last && nearlyEqual(first->x, last->x) && nearlyEqual(first->y, last->y),
                                "closed polygon last point is not first point")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_POLYCURVE);
        auto* extVar = dynamic_cast<TDVOCPolyCurveExtVar*>(operationManager.GetActiveVOPExtVariables());
        if (int result = expect(extVar != nullptr, "polycurve operation did not expose TDVOCPolyCurveExtVar")) {
            return result;
        }
        extVar->SetResolution(9);
        extVar->SetShowControls(false);
        extVar->SetShowPolygon(false);
        extVar->SendUpdateToParrentOP();

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_RIGHT, VKState_KEY_UNKNOWN, 100.0, 0.0);

        auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(model.GetObject(0));
        if (int result = expect(polyCurve != nullptr, "polycurve did not create TDVecPolyCurve")) {
            return result;
        }
        if (int result = expect(polyCurve->GetType() == VECOBJ_POLYCURVE, "polycurve created wrong object type")) {
            return result;
        }
        if (int result = expect(polyCurve->CountPoints() == 2, "polycurve does not have two points")) {
            return result;
        }
        if (int result = expect(polyCurve->GetResolution() == 9 && !polyCurve->GetShowControls() && !polyCurve->GetShowPolygon(),
                                "polycurve external variables were not stored in object")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_POLYCURVE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_CTRL, 50.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_CTRL, 50.0, 50.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_RIGHT, VKState_KEY_UNKNOWN, 100.0, 0.0);

        auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(model.GetObject(0));
        if (int result = expect(polyCurve != nullptr, "polycurve qspline did not create TDVecPolyCurve")) {
            return result;
        }
        if (int result = expect(polyCurve->CountPoints() == 3, "polycurve qspline does not have three contour points")) {
            return result;
        }
        const TDMatConturPoint* control = polyCurve->GetPoint(1);
        if (int result = expect(control && control->eType == CPT_PRIME_QSPLINE, "polycurve second point is not QSPLINE control")) {
            return result;
        }
    }

    return 0;
}
