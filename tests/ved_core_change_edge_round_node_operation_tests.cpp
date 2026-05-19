#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"
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

TDMatConturPoint point(double x, double y, TDConturPointType type = CPT_PRIME_LINE) {
    return {x, y, type, true};
}

} // namespace

int main() {
    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polyCurve = new TDVecPolyCurve();
        polyCurve->AppendPoint(point(0.0, 0.0));
        polyCurve->AppendPoint(point(50.0, 50.0));
        polyCurve->AppendPoint(point(100.0, 0.0));
        polyCurve->SetSelect(true);
        model.AppendObject(polyCurve);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_CHANGE_EDGE_ROUND_NODE,
                                "change edge round node operation was not activated")) {
            return result;
        }

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);

        const TDMatConturPoint* changed = polyCurve->GetPoint(1);
        if (int result = expect(changed && changed->eType == CPT_PRIME_QSPLINE,
                                "polycurve middle point was not changed to QSPLINE")) {
            return result;
        }

        operationManager.SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);

        changed = polyCurve->GetPoint(1);
        if (int result = expect(changed && changed->eType == CPT_PRIME_LINE,
                                "polycurve middle point was not changed back to LINE")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polyCurve = new TDVecPolyCurve();
        polyCurve->AppendPoint(point(0.0, 0.0));
        polyCurve->AppendPoint(point(50.0, 50.0));
        polyCurve->AppendPoint(point(100.0, 0.0, CPT_PRIME_QSPLINE));
        polyCurve->AppendPoint(point(150.0, 0.0));
        polyCurve->SetSelect(true);
        model.AppendObject(polyCurve);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);

        const TDMatConturPoint* unchanged = polyCurve->GetPoint(1);
        if (int result = expect(unchanged && unchanged->eType == CPT_PRIME_LINE,
                                "change point type ignored old VED adjacent-QSPLINE rule")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* polyCurveArea = new TDVecPolyCurveArea();
        polyCurveArea->AppendPoint(point(0.0, 0.0));
        polyCurveArea->AppendPoint(point(50.0, 50.0));
        polyCurveArea->AppendPoint(point(100.0, 0.0));
        polyCurveArea->AppendPoint(point(0.0, 0.0));
        polyCurveArea->SetSelect(true);
        model.AppendObject(polyCurveArea);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 50.0);

        const TDMatConturPoint* changed = polyCurveArea->GetPoint(1);
        if (int result = expect(changed && changed->eType == CPT_PRIME_QSPLINE,
                                "polycurve area middle point was not changed to QSPLINE")) {
            return result;
        }
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* line = new TDVecLine(0.0, 0.0, 100.0, 0.0);
        line->SetSelect(true);
        model.AppendObject(line);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT,
                                "change edge round node with unsupported selection did not fall back to select/move")) {
            return result;
        }
    }

    return 0;
}
