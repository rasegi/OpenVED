#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon_area.h"
#include "vec_polycurve_area.h"
#include "vop_manager.h"

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

bool nearlyEqual(double a, double b) {
    return std::fabs(a - b) < 0.000001;
}

} // namespace

int main() {
    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_RECTANGLE_NOTROTATED);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        auto* rectangle = dynamic_cast<TDVecPolygonArea*>(model.GetObject(0));
        if (int result = expect(rectangle != nullptr, "orthogonal rectangle did not create TDVecPolygonArea")) {
            return result;
        }
        if (int result = expect(rectangle->GetRectangularLock(), "orthogonal rectangle did not set rectangular lock")) {
            return result;
        }
        if (int result = expect(rectangle->CountPoints() == 4, "orthogonal rectangle does not have four points")) {
            return result;
        }
        const TDMatRect frame = rectangle->GetFrame();
        if (int result = expect(nearlyEqual(frame.P1.x, 0.0) && nearlyEqual(frame.P1.y, 0.0) &&
                                nearlyEqual(frame.P3.x, 100.0) && nearlyEqual(frame.P3.y, 50.0),
                                "orthogonal rectangle frame is wrong")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_RECTANGLE_ROTATED);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        auto* rectangle = dynamic_cast<TDVecPolygonArea*>(model.GetObject(0));
        if (int result = expect(rectangle != nullptr, "rotated rectangle did not create TDVecPolygonArea")) {
            return result;
        }
        if (int result = expect(rectangle->GetRectangularLock(), "rotated rectangle did not set rectangular lock")) {
            return result;
        }
        if (int result = expect(rectangle->CountPoints() == 4, "rotated rectangle does not have four points")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_ROUND_RECTANGLE_NOTROTATED);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 50.0);

        auto* roundRectangle = dynamic_cast<TDVecPolyCurveArea*>(model.GetObject(0));
        if (int result = expect(roundRectangle != nullptr, "round rectangle did not create TDVecPolyCurveArea")) {
            return result;
        }
        if (int result = expect(roundRectangle->GetRectangularLock(), "round rectangle did not set rectangular lock")) {
            return result;
        }
        if (int result = expect(roundRectangle->CountPoints() == 13, "round rectangle does not have thirteen contour points")) {
            return result;
        }
    }

    return 0;
}
