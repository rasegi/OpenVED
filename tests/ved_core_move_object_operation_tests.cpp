#include "vec_edit_cad.h"
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
    return expect(
        nearlyEqual(actual.x1, x1) &&
        nearlyEqual(actual.y1, y1) &&
        nearlyEqual(actual.x2, x2) &&
        nearlyEqual(actual.y2, y2),
        label);
}

} // namespace

int main() {
    {
        TDVecModel model;
        auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
        auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
        line->SetSelect(true);

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_MOVE_OBJECT);

        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_MOVE_OBJECT, "move operation was not activated")) {
            return result;
        }
        if (int result = expect(editor.GetUsedCursor() == VECVIEW_DOCK, "move operation did not use dock cursor")) {
            return result;
        }

        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 60.0, 5.0);
        if (int result = expectLine(*line, 0.0, 0.0, 100.0, 0.0, "real line moved before mouse up")) {
            return result;
        }
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 5.0);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT, "move operation did not return to select-move after mouse up")) {
            return result;
        }
        if (int result = expectLine(*line, 10.0, 5.0, 110.0, 5.0, "selected line was not moved from midpoint to release point")) {
            return result;
        }
        if (int result = expectLine(*secondLine, 0.0, 20.0, 100.0, 20.0, "unselected line was moved")) {
            return result;
        }
    }

    {
        TDVecModel model;
        auto* firstLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
        auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
        firstLine->SetSelect(true);
        secondLine->SetSelect(true);

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_MOVE_OBJECT);

        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 60.0, 20.0);
        if (int result = expectLine(*firstLine, 0.0, 0.0, 100.0, 0.0, "first grouped line moved before mouse up")) {
            return result;
        }
        if (int result = expectLine(*secondLine, 0.0, 20.0, 100.0, 20.0, "second grouped line moved before mouse up")) {
            return result;
        }
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 20.0);
        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT, "group move operation did not return to select-move after mouse up")) {
            return result;
        }
        if (int result = expectLine(*firstLine, 10.0, 10.0, 110.0, 10.0, "first selected line was not group-moved")) {
            return result;
        }
        if (int result = expectLine(*secondLine, 10.0, 30.0, 110.0, 30.0, "second selected line was not group-moved")) {
            return result;
        }
    }

    {
        TDVecModel model;
        auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_MOVE_OBJECT);

        if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT,
                                "move operation without selection did not fall back to select/move")) {
            return result;
        }
        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 60.0, 5.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 5.0);
        if (int result = expectLine(*line, 0.0, 0.0, 100.0, 0.0, "unselected line was moved")) {
            return result;
        }
    }

    return 0;
}
