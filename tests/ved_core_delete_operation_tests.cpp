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
    {
        TDVecModel model;
        auto* firstLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
        model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0));
        firstLine->SetSelect(true);

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_DELETE_OBJECT);

        if (int result = expect(model.CountObjects() == 2, "delete operation activation removed selected object")) {
            return result;
        }
        operationManager.KeyDown(VIRTKEY_DELETE, VKState_KEY_UNKNOWN);
        if (int result = expect(model.CountObjects() == 1, "delete key did not remove selected object")) {
            return result;
        }
        if (int result = expect(model.CountSelectedObjects() == 0, "deleted selection remained selected")) {
            return result;
        }
    }

    {
        TDVecModel model;
        model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0));
        model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0));

        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_DELETE_OBJECT);

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 0.0);
        if (int result = expect(model.CountObjects() == 2, "first delete click removed object")) {
            return result;
        }
        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 0.0);
        if (int result = expect(model.CountObjects() == 1, "second delete click did not remove object")) {
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
        operationManager.SetOperation(VOM_DELETE_OBJECT);

        if (int result = expect(model.CountObjects() == 2, "delete operation activation removed multi-selected objects")) {
            return result;
        }
        operationManager.KeyDown(VIRTKEY_DELETE, VKState_KEY_UNKNOWN);
        if (int result = expect(model.CountObjects() == 0, "delete key did not remove all selected objects")) {
            return result;
        }
    }

    return 0;
}
