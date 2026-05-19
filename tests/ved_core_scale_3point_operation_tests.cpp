#include "vec_edit_cad.h"
#include "vec_graphic_engine.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_view_interface.h"
#include "vop_manager.h"

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

class RecordingGraphicEngine : public TDGraphicEngine {
public:
    void DrawEllipse(const TDMatEllipse*, bool) override {}
    void DrawLine(const TDMatLine*, bool) override {}
    void DrawRectangle(const TDMatRect*, bool) override {}
    void DrawPolygon(const TDMatPointsArray*, bool) override {}
    void DrawLineOutLine(const TDMatLine*) override {}
    void DrawConstructPolygon(const TDMatPointsArray*) override {}
    void DrawBoxOutLine(TDMatPoint, TDMatPoint) override {}
    void DrawNode(double, double, TDNodeType, bool) override {
        ++nodeDraws;
    }
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

    int nodeDraws = 0;
    int nodeSize = 3;
    TDRgbColor drawColor = 0;
};

class TestViewInterface : public TDVecViewInterfaceBase {
public:
    explicit TestViewInterface(TDGraphicEngine* graphicEngine) {
        SetGraphicEngine(graphicEngine);
    }

    void Send_ViewRefresh_ToView() override {
        ++refreshCount;
    }
    void Send_UseCursor_ToView(TDVecViewCursor) override {}
    double Send_GetZoom_ToView() const override { return 1.0; }
    void Send_SetZoom_ToView(double, const POINT*) override {}
    bool Send_GetGridLock_ToView() const override { return false; }
    bool Send_IsRectVisible_ToView(TDMatRect) const override { return true; }
    long Send_GetMouseTolerance_ToView() const override { return 5; }
    double Send_GetMinimumDistance_ToView() const override { return 1.0; }
    void Send_ZoomInOutEvent_ToView(TDVOPZoomInOutExtVar*) const override {}
    void* GetParent() override { return nullptr; }

    int refreshCount = 0;
};

} // namespace

int main() {
    TDVecModel model;
    auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    line->SetSelect(true);

    TDVecEditCad editor;
    editor.SetVecModel(&model);
    editor.SetVisualNodes(false);
    editor.SetVisualFrame(false);
    RecordingGraphicEngine graphicEngine;
    TestViewInterface view(&graphicEngine);
    editor.RegisterViewInterface(&view);

    TDViewOperationManager operationManager(&model, &editor);
    operationManager.SetOperation(VOM_SCALE_OBJECT_3POINT);
    editor.SetVisualNodes(false);
    editor.SetVisualFrame(false);
    if (int result = expect(model.GetSelectedObjectIndex() == 0, "scale 3 point test object is not selected")) {
        return result;
    }
    if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SCALE_OBJECT_3POINT,
                            "scale 3 point operation was not activated")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
    graphicEngine.nodeDraws = 0;
    editor.PaintContentOnView(&view);
    if (int result = expect(graphicEngine.nodeDraws == 1,
                            "scale 3 point did not keep first interactive node: " + std::to_string(graphicEngine.nodeDraws))) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
    graphicEngine.nodeDraws = 0;
    editor.PaintContentOnView(&view);
    if (int result = expect(graphicEngine.nodeDraws == 2,
                            "scale 3 point did not keep first and second interactive nodes: " + std::to_string(graphicEngine.nodeDraws))) {
        return result;
    }

    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 150.0, 0.0);
    graphicEngine.nodeDraws = 0;
    editor.PaintContentOnView(&view);
    if (int result = expect(graphicEngine.nodeDraws == 2,
                            "scale 3 point preview lost interactive nodes: " + std::to_string(graphicEngine.nodeDraws))) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 150.0, 0.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 150.0, 0.0);
    graphicEngine.nodeDraws = 0;
    editor.PaintContentOnView(&view);
    if (int result = expect(graphicEngine.nodeDraws == 0,
                            "scale 3 point did not clear temporary nodes after finish: " + std::to_string(graphicEngine.nodeDraws))) {
        return result;
    }

    return 0;
}
