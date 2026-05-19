#include "vec_edit_cad.h"
#include "vec_ellipse.h"
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

int expectEllipse(const TDVecEllipse& ellipse, double xCenter, double yCenter, double xRadius, double yRadius, double angle, const std::string& label) {
    const TDMatEllipse actual = ellipse.GetEllipse();
    return expect(
        nearlyEqual(actual.xCenter, xCenter) &&
        nearlyEqual(actual.yCenter, yCenter) &&
        nearlyEqual(actual.xRadius, xRadius) &&
        nearlyEqual(actual.yRadius, yRadius) &&
        nearlyEqual(actual.nAngle, angle),
        label);
}

int expectMatEllipse(const TDMatEllipse& ellipse, double xCenter, double yCenter, double xRadius, double yRadius, double angle, const std::string& label) {
    return expect(
        nearlyEqual(ellipse.xCenter, xCenter) &&
        nearlyEqual(ellipse.yCenter, yCenter) &&
        nearlyEqual(ellipse.xRadius, xRadius) &&
        nearlyEqual(ellipse.yRadius, yRadius) &&
        nearlyEqual(ellipse.nAngle, angle),
        label);
}

TDVecEllipse* createdEllipse(TDVecModel& model) {
    return dynamic_cast<TDVecEllipse*>(model.GetObject(model.CountObjects() - 1));
}

} // namespace

int main() {
    {
        const TDMatEllipse ellipse = MatEllipseMitte3PointReal({0.0, 0.0}, {10.0, 10.0}, {-5.0, 5.0});
        if (int result = expectMatEllipse(
                ellipse,
                0.0,
                0.0,
                std::sqrt(200.0),
                std::sqrt(50.0),
                -45.0,
                "rotated midpoint ellipse math mismatch")) {
            return result;
        }
    }

    {
        TDVecEllipse ellipse(0.0, 0.0, std::sqrt(200.0), std::sqrt(50.0), -45.0);
        const TDMatRect frame = ellipse.GetFrame();
        const TDMatPoint movedNode{frame.P3.x + 2.0, frame.P3.y + 1.0};
        if (int result = expect(ellipse.MoveNode(4, 2.0, 1.0, movedNode), "rotated ellipse node move failed")) {
            return result;
        }
        if (int result = expect(nearlyEqual(ellipse.GetAngle(), -45.0), "rotated ellipse node move changed angle")) {
            return result;
        }
        if (int result = expect(ellipse.GetXRadius() > 0.0 && ellipse.GetYRadius() > 0.0, "rotated ellipse node move produced invalid radii")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_CIRCLE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        if (int result = expect(model.CountObjects() == 1, "circle midpoint did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 0.0, 0.0, 10.0, 10.0, 0.0, "circle midpoint geometry mismatch")) {
            return result;
        }
        if (int result = expect(createdEllipse(model)->GetResizeProportional(), "circle midpoint was not marked proportional")) {
            return result;
        }
        createdEllipse(model)->MoveNode(3, 5.0, 0.0, {15.0, 0.0});
        if (int result = expectEllipse(*createdEllipse(model), 0.0, 0.0, 15.0, 15.0, 0.0, "circle midpoint did not stay circle after node move")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, -5.0, 5.0);
        if (int result = expect(editor.GetUsedCursor() == VECVIEW_CREATE_ELLIPSE_ROTATED_2, "rotated ellipse did not enter second cursor after one preview move")) {
            return result;
        }
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        if (int result = expect(model.CountObjects() == 1, "rotated ellipse second-click without preview did not create object")) {
            return result;
        }
        if (int result = expectEllipse(
                *createdEllipse(model),
                0.0,
                0.0,
                std::sqrt(200.0),
                std::sqrt(50.0),
                -45.0,
                "rotated ellipse second-click without preview geometry mismatch")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        if (int result = expect(model.CountObjects() == 1, "rotated ellipse direct third-click did not create object")) {
            return result;
        }
        if (int result = expectEllipse(
                *createdEllipse(model),
                0.0,
                0.0,
                std::sqrt(200.0),
                std::sqrt(50.0),
                -45.0,
                "rotated ellipse direct third-click geometry mismatch")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_CIRCLE_DIAMETER);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        if (int result = expect(model.CountObjects() == 1, "circle diameter did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 5.0, 0.0, 5.0, 5.0, 0.0, "circle diameter geometry mismatch")) {
            return result;
        }
        if (int result = expect(createdEllipse(model)->GetResizeProportional(), "circle diameter was not marked proportional")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_CIRCLE_EDGE);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 10.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -10.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -10.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -10.0, 0.0);
        if (int result = expect(model.CountObjects() == 1, "circle edge did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 0.0, 0.0, 10.0, 10.0, 0.0, "circle edge geometry mismatch")) {
            return result;
        }
        if (int result = expect(createdEllipse(model)->GetResizeProportional(), "circle edge was not marked proportional")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_ORTHOGONAL);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 20.0, 10.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 20.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 20.0, 10.0);
        if (int result = expect(model.CountObjects() == 1, "orthogonal ellipse did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 10.0, 5.0, 10.0, 5.0, 0.0, "orthogonal ellipse geometry mismatch")) {
            return result;
        }
        const TDVecModelHitResult hit = model.FindObjectAt({20.0, 5.0}, 1.0);
        if (int result = expect(hit.IsHit(), "created ellipse was not hit-testable")) {
            return result;
        }
        model.SelectOnlyObject(0);
        model.MoveSelectedObjects(5.0, 5.0);
        if (int result = expectEllipse(*createdEllipse(model), 15.0, 10.0, 10.0, 5.0, 0.0, "selected ellipse was not movable through model")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 5.0);
        if (int result = expect(model.CountObjects() == 1, "rotated ellipse did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 0.0, 0.0, 10.0, 5.0, 0.0, "rotated ellipse geometry mismatch")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 4.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 5.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 5.0);
        if (int result = expect(model.CountObjects() == 1, "rotated ellipse third-click sequence did not create object")) {
            return result;
        }
        if (int result = expectEllipse(*createdEllipse(model), 0.0, 0.0, 10.0, 5.0, 0.0, "rotated ellipse third-click geometry mismatch")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ELLIPSE_MIDPOINT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 10.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, -5.0, 5.0);
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, -5.0, 5.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -5.0, 5.0);
        if (int result = expect(model.CountObjects() == 1, "rotated 45 degree ellipse did not create object")) {
            return result;
        }
        if (int result = expectEllipse(
                *createdEllipse(model),
                0.0,
                0.0,
                std::sqrt(200.0),
                std::sqrt(50.0),
                -45.0,
                "rotated 45 degree ellipse geometry mismatch")) {
            return result;
        }
    }

    return 0;
}
