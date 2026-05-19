#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vom_rotate_object_activepoint.h"
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
    auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(10.0, 0.0, 20.0, 0.0)));
    line->SetSelect(true);

    TDVecEditCad editor;
    editor.SetVecModel(&model);
    TDViewOperationManager operationManager(&model, &editor);
    operationManager.SetOperation(VOM_ROTATE_OBJECT_ACTIVEPOINT);

    if (int result = expect(operationManager.GetAktiveOperationType() == VOM_ROTATE_OBJECT_ACTIVEPOINT, "rotate operation was not activated")) {
        return result;
    }

    auto* extVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(operationManager.GetActiveVOPExtVariables());
    if (int result = expect(extVar != nullptr, "rotate operation ext vars missing")) {
        return result;
    }
    extVar->SetAngle(-45.0);
    extVar->SetCopyFlag(true);
    extVar->SetSelectFlag(true);
    extVar->SendUpdateToParrentOP();

    operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 0.0, 0.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 0.0, 0.0);

    if (int result = expect(operationManager.GetAktiveOperationType() == VOM_ROTATE_OBJECT_ACTIVEPOINT, "rotate operation changed after first copy rotate")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 2, "first copy rotate did not append one clone")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 1, "select copy did not leave exactly one selected object after first rotate")) {
        return result;
    }

    operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 5.0, 5.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 5.0, 5.0);
    if (int result = expect(model.CountObjects() == 3, "second copy rotate did not append another clone")) {
        return result;
    }

    return 0;
}
