#include "vec_edit_cad.h"
#include "vec_group.h"
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
    TDVecModel model;
    auto* line = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));

    TDVecEditCad editor;
    editor.SetVecModel(&model);
    TDViewOperationManager operationManager(&model, &editor);
    operationManager.SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);

    if (int result = expect(operationManager.GetAktiveOperationType() == VOM_SELECT_MOVE_NODE_OBJECT, "select-move operation was not activated")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 0.0);
    if (int result = expect(line->GetSelect(), "line was not selected on mouse down")) {
        return result;
    }

    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 35.0, 5.0);
    if (int result = expectLine(*line, 0.0, 0.0, 100.0, 0.0, "real line moved before mouse up")) {
        return result;
    }

    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 35.0, 5.0);
    if (int result = expectLine(*line, 10.0, 5.0, 110.0, 5.0, "line was not moved on mouse up")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 5.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 5.0);
    if (int result = expectLine(*line, 10.0, 5.0, 110.0, 5.0, "click without drag changed line")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 5.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 20.0, 15.0);
    if (int result = expectLine(*line, 10.0, 5.0, 110.0, 5.0, "real line start node moved before mouse up")) {
        return result;
    }
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 20.0, 15.0);
    if (int result = expectLine(*line, 20.0, 15.0, 110.0, 5.0, "start node was not moved on mouse up")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 110.0, 5.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 115.0, 25.0);
    if (int result = expectLine(*line, 20.0, 15.0, 110.0, 5.0, "real line end node moved before mouse up")) {
        return result;
    }
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 115.0, 25.0);
    if (int result = expectLine(*line, 20.0, 15.0, 115.0, 25.0, "end node was not moved on mouse up")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -1.0, 19.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 101.0, 21.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 101.0, 21.0);
    if (int result = expect(!line->GetSelect() && secondLine->GetSelect(), "area select did not select only second line")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, -1.0, 14.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 116.0, 26.0);
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 116.0, 26.0);
    if (int result = expect(line->GetSelect() && secondLine->GetSelect(), "area select did not select both lines")) {
        return result;
    }

    operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 50.0, 20.0);
    operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 30.0);
    if (int result = expectLine(*line, 20.0, 15.0, 115.0, 25.0, "first grouped line moved before mouse up")) {
        return result;
    }
    if (int result = expectLine(*secondLine, 0.0, 20.0, 100.0, 20.0, "second grouped line moved before mouse up")) {
        return result;
    }
    operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 60.0, 30.0);
    if (int result = expectLine(*line, 30.0, 25.0, 125.0, 35.0, "first grouped line was not moved on mouse up")) {
        return result;
    }
    if (int result = expectLine(*secondLine, 10.0, 30.0, 110.0, 30.0, "second grouped line was not moved on mouse up")) {
        return result;
    }

    TDVecModel groupModel;
    auto* groupLine = dynamic_cast<TDVecLine*>(groupModel.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    auto* groupSecondLine = dynamic_cast<TDVecLine*>(groupModel.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
    groupLine->SetSelect(true);
    groupSecondLine->SetSelect(true);
    if (int result = expect(groupModel.MakeGroup(), "test group creation failed")) {
        return result;
    }
    auto* group = dynamic_cast<TDVecGroup*>(groupModel.GetObject(0));
    if (int result = expect(group != nullptr && group->GetSelect(), "created group was not selected")) {
        return result;
    }

    TDVecEditCad groupEditor;
    groupEditor.SetVecModel(&groupModel);
    TDViewOperationManager groupOperationManager(&groupModel, &groupEditor);
    groupOperationManager.SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
    groupOperationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 25.0, 0.0);
    groupOperationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 35.0, 10.0);
    groupOperationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 35.0, 10.0);
    if (int result = expect(groupModel.ResolveAllSelectedGroups(), "test group resolve failed")) {
        return result;
    }
    auto* movedFirst = dynamic_cast<TDVecLine*>(groupModel.GetObject(0));
    auto* movedSecond = dynamic_cast<TDVecLine*>(groupModel.GetObject(1));
    if (int result = expect(movedFirst != nullptr && movedSecond != nullptr, "resolved moved group lines missing")) {
        return result;
    }
    if (int result = expectLine(*movedFirst, 10.0, 10.0, 110.0, 10.0, "group first line was not moved through select operation")) {
        return result;
    }
    if (int result = expectLine(*movedSecond, 10.0, 30.0, 110.0, 30.0, "group second line was not moved through select operation")) {
        return result;
    }

    TDVecModel groupNodeModel;
    auto* groupNodeLine = dynamic_cast<TDVecLine*>(groupNodeModel.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    auto* groupNodeSecondLine = dynamic_cast<TDVecLine*>(groupNodeModel.AppendObject(new TDVecLine(0.0, 20.0, 100.0, 20.0)));
    groupNodeLine->SetSelect(true);
    groupNodeSecondLine->SetSelect(true);
    if (int result = expect(groupNodeModel.MakeGroup(), "test group node creation failed")) {
        return result;
    }

    TDVecEditCad groupNodeEditor;
    groupNodeEditor.SetVecModel(&groupNodeModel);
    TDViewOperationManager groupNodeOperationManager(&groupNodeModel, &groupNodeEditor);
    groupNodeOperationManager.SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
    groupNodeOperationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 20.0);
    groupNodeOperationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 40.0);
    groupNodeOperationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 200.0, 40.0);
    if (int result = expect(groupNodeModel.ResolveAllSelectedGroups(), "test group node resolve failed")) {
        return result;
    }
    auto* scaledFirst = dynamic_cast<TDVecLine*>(groupNodeModel.GetObject(0));
    auto* scaledSecond = dynamic_cast<TDVecLine*>(groupNodeModel.GetObject(1));
    if (int result = expect(scaledFirst != nullptr && scaledSecond != nullptr, "resolved scaled group lines missing")) {
        return result;
    }
    if (int result = expectLine(*scaledFirst, 0.0, 0.0, 200.0, 0.0, "group first line was not scaled through node move")) {
        return result;
    }
    if (int result = expectLine(*scaledSecond, 0.0, 40.0, 200.0, 40.0, "group second line was not scaled through node move")) {
        return result;
    }

    return 0;
}
