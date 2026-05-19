#include "vec_edit_cad.h"
#include "vec_ellipse.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vop_manager.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool nearlyEqual(double a, double b) {
    return std::fabs(a - b) < 0.000001;
}

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expect(bool condition, const std::string& message) {
    return condition ? 0 : fail(message);
}

int expectLine(const TDVecLine& line, double x1, double y1, double x2, double y2, const std::string& label) {
    const TDMatLine actual = line.GetLine();
    if (nearlyEqual(actual.x1, x1) &&
        nearlyEqual(actual.y1, y1) &&
        nearlyEqual(actual.x2, x2) &&
        nearlyEqual(actual.y2, y2)) {
        return 0;
    }

    std::cerr << label << ": got (" << actual.x1 << ", " << actual.y1 << ") -> ("
              << actual.x2 << ", " << actual.y2 << "), expected (" << x1 << ", "
              << y1 << ") -> (" << x2 << ", " << y2 << ")\n";
    return 1;
}

int expectEllipse(const TDVecEllipse& ellipse, double xCenter, double yCenter, double xRadius, double yRadius, double angle, const std::string& label) {
    const TDMatEllipse actual = ellipse.GetEllipse();
    if (nearlyEqual(actual.xCenter, xCenter) &&
        nearlyEqual(actual.yCenter, yCenter) &&
        nearlyEqual(actual.xRadius, xRadius) &&
        nearlyEqual(actual.yRadius, yRadius) &&
        nearlyEqual(actual.nAngle, angle)) {
        return 0;
    }

    std::cerr << label << ": got center (" << actual.xCenter << ", " << actual.yCenter
              << "), radii (" << actual.xRadius << ", " << actual.yRadius << "), angle "
              << actual.nAngle << '\n';
    return 1;
}

} // namespace

int main() {
    {
        TDVecModel model;
        auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 100.0)));

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_SELECTMOVE_OBJ_SCALE_FRAME);

        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECTMOVE_OBJ_SCALE_FRAME, "select-scale operation was not activated")) {
            return result;
        }
        if (int result = expect(!editor.GetVisualNodes() && editor.GetVisualFrame(), "select-scale did not switch to frame visualization")) {
            return result;
        }

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 100.0);
        if (int result = expect(line->GetSelect(), "line was not selected before frame scale")) {
            return result;
        }
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 200.0);
        if (int result = expectLine(*line, 0.0, 0.0, 100.0, 100.0, "real line scaled before mouse up")) {
            return result;
        }
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 200.0);
        if (int result = expectLine(*line, 0.0, 0.0, 200.0, 200.0, "line was not frame-scaled from node 4")) {
            return result;
        }
    }

    {
        TDVecEllipse ellipse(0.0, 0.0, 20.0, 10.0, 45.0);
        const TDMatRect objectFrame = ellipse.GetFrame();
        const TDMatRect scaleFrame = ellipse.GetScaleFrame();
        const double extent = std::sqrt(250.0);
        if (int result = expect(!nearlyEqual(objectFrame.P1.y, objectFrame.P2.y), "rotated ellipse object frame was not oriented")) {
            return result;
        }
        if (int result = expect(
                nearlyEqual(scaleFrame.P1.x, -extent) &&
                nearlyEqual(scaleFrame.P1.y, -extent) &&
                nearlyEqual(scaleFrame.P3.x, extent) &&
                nearlyEqual(scaleFrame.P3.y, extent),
                "rotated ellipse scale frame was not horizontal bounding frame")) {
            return result;
        }

        ellipse.ToScale({0.0, 0.0}, 2.0, 1.0);
        if (int result = expect(ellipse.GetXRadius() > 0.0 && ellipse.GetYRadius() > 0.0, "ellipse scale produced invalid radii")) {
            return result;
        }
        if (int result = expect(!nearlyEqual(ellipse.GetAngle(), 45.0), "rotated ellipse scale did not update angle")) {
            return result;
        }
    }

    {
        TDVecModel model;
        auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
        auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
        line->SetSelect(true);
        secondLine->SetSelect(true);

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_SELECTMOVE_OBJ_SCALE_FRAME);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 10.0);
        if (int result = expectLine(*line, 10.0, 10.0, 110.0, 10.0, "first selected line was not moved by select-scale group move")) {
            return result;
        }
        if (int result = expectLine(*secondLine, 10.0, 30.0, 110.0, 30.0, "second selected line was not moved by select-scale group move")) {
            return result;
        }
    }

    {
        TDVecModel model;
        auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
        auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
        line->SetSelect(true);
        secondLine->SetSelect(true);
        if (int result = expect(model.MakeGroup(), "test group creation for frame scale failed")) {
            return result;
        }

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_SELECTMOVE_OBJ_SCALE_FRAME);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 20.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 40.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 40.0);
        if (int result = expect(model.ResolveAllSelectedGroups(), "test group resolve after frame scale failed")) {
            return result;
        }
        auto* scaledFirst = dynamic_cast<TDVecLine*>(model.GetObject(0));
        auto* scaledSecond = dynamic_cast<TDVecLine*>(model.GetObject(1));
        if (int result = expect(scaledFirst != nullptr && scaledSecond != nullptr, "resolved scaled group lines missing")) {
            return result;
        }
        if (int result = expectLine(*scaledFirst, 0.0, 0.0, 200.0, 0.0, "group first line was not frame-scaled")) {
            return result;
        }
        if (int result = expectLine(*scaledSecond, 0.0, 40.0, 200.0, 40.0, "group second line was not frame-scaled")) {
            return result;
        }
    }

    {
        TDVecEllipse ellipse(10.0, 20.0, 5.0, 7.0, 0.0);
        ellipse.ToScale({0.0, 0.0}, 2.0, 3.0);
        if (int result = expectEllipse(ellipse, 20.0, 60.0, 21.0, 10.0, -90.0, "axis-aligned ellipse scale mismatch")) {
            return result;
        }
    }

    {
        TDVecEllipse circle(0.0, 0.0, 10.0, 10.0, 0.0);
        circle.Initialize();
        if (int result = expect(circle.GetResizeProportional(), "circle initialize did not enable proportional resize")) {
            return result;
        }
        if (int result = expect(circle.MoveScaleFrameNode(3, 10.0, 0.0, {20.0, 0.0}), "circle frame node move failed")) {
            return result;
        }
        if (int result = expectEllipse(circle, 5.0, 0.0, 15.0, 15.0, 0.0, "circle did not stay circle after select-frame move")) {
            return result;
        }
    }

    return 0;
}
