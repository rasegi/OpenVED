#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
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

} // namespace

int main() {
    TDVecModel model;
    auto* firstLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 10.0, 100.0, 10.0)));

    TDVecEditCad editor;
    editor.SetVecModel(&model);
    TDViewOperationManager operationManager(&model, &editor);
    operationManager.SetOperation(VOM_SELECT_OBJECT);

    if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_OBJECT, "select operation was not activated")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 0.0);
    if (int result = expect(firstLine->GetSelect(), "first line was not selected")) {
        return result;
    }
    if (int result = expect(!secondLine->GetSelect(), "second line unexpectedly selected")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 1, "selected count after first click failed")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_SHIFT, 25.0, 10.0);
    if (int result = expect(firstLine->GetSelect() && secondLine->GetSelect(), "shift select failed")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 2, "selected count after shift click failed")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 30.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 30.0);
    if (int result = expect(model.CountSelectedObjects() == 0, "miss did not deselect all")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -1.0, -1.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 101.0, 1.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 101.0, 1.0);
    if (int result = expect(firstLine->GetSelect() && !secondLine->GetSelect(), "area select failed")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_SHIFT, -1.0, 9.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_SHIFT, 101.0, 11.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_SHIFT, 101.0, 11.0);
    if (int result = expect(firstLine->GetSelect() && secondLine->GetSelect(), "shift area select failed")) {
        return result;
    }

    return 0;
}
