//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Change-EdgeRound-Node
//---------------------------------------------------------------------------
#define __VOM_CHANGE_EDGE_ROUND_NODE_CPP

#include "vom_change_edge_round_node.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"

#include <algorithm>

TDVOMChangeEdgeRoundNode::TDVOMChangeEdgeRoundNode(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      meObjectType(VECOBJ_UNKNOWN),
      mbChangeOK(false),
      mbObjectTypeOK(false),
      mbOnObject(false),
      miNode(-1) {
    if (SelectedObjectTypeOK()) {
        SetState(OSTATE_STARTED);
        mpVecEditCad->UseCursor(VECVIEW_SIMPLE_CROSS);
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }
}

TDVOMChangeEdgeRoundNode::~TDVOMChangeEdgeRoundNode() {
    IniInteractivePoints();
    Finished();
}

TDVOMChangeEdgeRoundNode::TDVOMChangeEdgeRoundNode(const TDVOMChangeEdgeRoundNode& oldOperation)
    : TDVOModify(oldOperation),
      meObjectType(oldOperation.meObjectType),
      mbChangeOK(oldOperation.mbChangeOK),
      mbObjectTypeOK(oldOperation.mbObjectTypeOK),
      mbOnObject(oldOperation.mbOnObject),
      miNode(oldOperation.miNode) {
}

TDVOMChangeEdgeRoundNode& TDVOMChangeEdgeRoundNode::operator=(const TDVOMChangeEdgeRoundNode& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    meObjectType = oldOperation.meObjectType;
    mbChangeOK = oldOperation.mbChangeOK;
    mbObjectTypeOK = oldOperation.mbObjectTypeOK;
    mbOnObject = oldOperation.mbOnObject;
    miNode = oldOperation.miNode;
    return *this;
}

TDVOMChangeEdgeRoundNode* TDVOMChangeEdgeRoundNode::Clone() const {
    return new TDVOMChangeEdgeRoundNode(*this);
}

void TDVOMChangeEdgeRoundNode::Finished() {
    meObjectType = VECOBJ_UNKNOWN;
    mbChangeOK = false;
    mbObjectTypeOK = false;
    mbOnObject = false;
    miNode = -1;
}

bool TDVOMChangeEdgeRoundNode::SelectedObjectTypeOK() const {
    if (!mpVecModel || mpVecModel->CountSelectedObjects() != 1) {
        return false;
    }

    const TDVecObject* object = mpVecModel->GetSelectedObject();
    return object && (object->GetType() == VECOBJ_POLYCURVE || object->GetType() == VECOBJ_POLYCURVEAREA);
}

double TDVOMChangeEdgeRoundNode::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad ? mpVecEditCad->GetActiveGraphicEngine() : nullptr;
    if (!graphicEngine) {
        return 1.0;
    }

    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

long TDVOMChangeEdgeRoundNode::FindNodeOnSelectedObject(TDMatPoint point) const {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject) {
        return -1;
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (graphicEngine) {
        return selectedObject->PointOnNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
    }
    return selectedObject->PointOnNode(point, MouseToleranceReal());
}

bool TDVOMChangeEdgeRoundNode::ChangeSelectedNodeType() {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || miNode == -1) {
        return false;
    }

    switch (selectedObject->GetType()) {
    case VECOBJ_POLYCURVE: {
        auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(selectedObject);
        return polyCurve && polyCurve->ChangePointType(static_cast<int>(miNode));
    }
    case VECOBJ_POLYCURVEAREA: {
        auto* polyCurveArea = dynamic_cast<TDVecPolyCurveArea*>(selectedObject);
        return polyCurveArea && polyCurveArea->ChangePointType(static_cast<int>(miNode));
    }
    default:
        return false;
    }
}

void __fastcall TDVOMChangeEdgeRoundNode::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        miNode = -1;
        mbChangeOK = false;
        mbObjectTypeOK = false;
        mbOnObject = false;
        SetState(OSTATE_RUNNING);
        break;
    default:
        break;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        Finished();
        SetState(OSTATE_ABORTED);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || !SelectedObjectTypeOK()) {
        mbObjectTypeOK = false;
        mbChangeOK = false;
        mbOnObject = false;
        SetState(OSTATE_ABORTED);
        return;
    }

    meObjectType = selectedObject->GetType();
    mbObjectTypeOK = true;

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    const TDMatPoint point{X, Y};
    if (graphicEngine) {
        mbOnObject = selectedObject->HitTest(graphicEngine, point, graphicEngine->GetMouseTolerance()).IsHit();
    } else {
        mbOnObject = selectedObject->HitTest(point, MouseToleranceReal()).IsHit();
    }
    miNode = FindNodeOnSelectedObject(point);

    if (miNode != -1) {
        mbChangeOK = true;
        SetState(OSTATE_RUNNING);
        XMouseStart = X;
        YMouseStart = Y;
        XMouseJet = X;
        YMouseJet = Y;
    } else if (mbOnObject) {
        mbChangeOK = false;
        SetState(OSTATE_STARTED);
    } else {
        mbChangeOK = false;
        SetState(OSTATE_ABORTED);
    }
}

void __fastcall TDVOMChangeEdgeRoundNode::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button != VIRTMOUSEBTM_LEFT || !mpVecModel->GetSelectedObject() || !mbChangeOK) {
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (ChangeSelectedNodeType()) {
        mpVecModel->MarkChanged();
    } else {
        mpVecEditCad->Beep();
    }

    selectedObject->Initialize();
    Finished();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();

    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOMChangeEdgeRoundNode::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMChangeEdgeRoundNode::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMChangeEdgeRoundNode::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
