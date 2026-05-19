#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon.h"
#include "vec_polycurve.h"
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

TDMatConturPoint conturPoint(double x, double y, TDConturPointType type = CPT_PRIME_LINE) {
    return {x, y, type, true};
}

} // namespace

int main() {
    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);

        operationManager.SetOperation(VOM_MOVE_NODE);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT,
                                "move node without selection did not fall back to select/move")) {
            return result;
        }

        operationManager.SetOperation(VOM_INSERT_NODE);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT,
                                "insert node without selection did not fall back to select/move")) {
            return result;
        }

        operationManager.SetOperation(VOM_DELETE_NODE);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT,
                                "delete node without selection did not fall back to select/move")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polygon = new TDVecPolygon();
        polygon->AppendPoint({0.0, 0.0});
        polygon->AppendPoint({100.0, 0.0});
        polygon->AppendPoint({100.0, 100.0});
        polygon->SetSelect(true);
        model.AppendObject(polygon);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_MOVE_NODE);
        if (int result = expect(editor.GetUsedCursor() == VECVIEW_SIMPLE_CROSS,
                                "move node did not use the simple cross cursor")) {
            return result;
        }
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 120.0, 30.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 120.0, 30.0);

        const TDMatPoint* moved = polygon->GetPoint(1);
        if (int result = expect(moved && nearlyEqual(moved->x, 120.0) && nearlyEqual(moved->y, 30.0),
                                "move node did not move the selected polygon node")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polygon = new TDVecPolygon();
        polygon->AppendPoint({0.0, 0.0});
        polygon->AppendPoint({100.0, 0.0});
        polygon->AppendPoint({100.0, 100.0});
        polygon->SetSelect(true);
        model.AppendObject(polygon);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_INSERT_NODE);
        if (int result = expect(editor.GetUsedCursor() == VECVIEW_CROSS_PLUS,
                                "insert node did not use the cross plus cursor")) {
            return result;
        }
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 0.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 25.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 25.0);

        if (int result = expect(polygon->CountPoints() == 4, "insert node did not insert a polygon point")) {
            return result;
        }
        const TDMatPoint* inserted = polygon->GetPoint(1);
        if (int result = expect(inserted && nearlyEqual(inserted->x, 50.0) && nearlyEqual(inserted->y, 25.0),
                                "insert node inserted the polygon point at the wrong position")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polyCurve = new TDVecPolyCurve();
        polyCurve->AppendPoint(conturPoint(0.0, 0.0));
        polyCurve->AppendPoint(conturPoint(100.0, 0.0));
        polyCurve->AppendPoint(conturPoint(100.0, 100.0));
        polyCurve->SetSelect(true);
        model.AppendObject(polyCurve);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_DELETE_NODE);
        if (int result = expect(editor.GetUsedCursor() == VECVIEW_CROSS_MINUS,
                                "delete node did not use the cross minus cursor")) {
            return result;
        }
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 0.0);

        if (int result = expect(polyCurve->CountPoints() == 2, "delete node did not remove the polycurve point")) {
            return result;
        }
        const TDMatConturPoint* remaining = polyCurve->GetPoint(1);
        if (int result = expect(remaining && nearlyEqual(remaining->x, 100.0) && nearlyEqual(remaining->y, 100.0),
                                "delete node removed the wrong polycurve point")) {
            return result;
        }
    }

    return 0;
}
