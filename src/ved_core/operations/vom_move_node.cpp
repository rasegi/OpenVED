//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Move-Node
//---------------------------------------------------------------------------
#define __VOM_MOVE_NODE_CPP

#include "vom_move_node.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <algorithm>

TDVOMMoveNode::TDVOMMoveNode(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpObject(nullptr),
      mbDragOK(false),
      miNode(-1) {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject) {
        SetState(OSTATE_NEEDSELECTED);
        return;
    }
    mpObject = selectedObject->Clone();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_SIMPLE_CROSS);
}

TDVOMMoveNode::TDVOMMoveNode(const TDVOMMoveNode& oldOperation)
    : TDVOModify(oldOperation),
      mpObject(oldOperation.mpObject ? oldOperation.mpObject->Clone() : nullptr),
      mbDragOK(oldOperation.mbDragOK),
      miNode(oldOperation.miNode) {
}

TDVOMMoveNode& TDVOMMoveNode::operator=(const TDVOMMoveNode& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    mpObject = oldOperation.mpObject ? oldOperation.mpObject->Clone() : nullptr;
    mbDragOK = oldOperation.mbDragOK;
    miNode = oldOperation.miNode;
    return *this;
}

TDVOMMoveNode* TDVOMMoveNode::Clone() const {
    return new TDVOMMoveNode(*this);
}

double TDVOMMoveNode::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad ? mpVecEditCad->GetActiveGraphicEngine() : nullptr;
    if (!graphicEngine) {
        return 1.0;
    }
    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

void __fastcall TDVOMMoveNode::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT) {
        SetState(OSTATE_ABORTED);
        mbDragOK = false;
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || mpVecModel->GetSelectedObjectIndex() == -1) {
        SetState(OSTATE_ABORTED);
        mbDragOK = false;
        return;
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    const TDMatPoint point{X, Y};
    const bool clickOnObject = graphicEngine
        ? selectedObject->HitTest(graphicEngine, point, graphicEngine->GetMouseTolerance()).IsHit()
        : selectedObject->HitTest(point, MouseToleranceReal()).IsHit();
    miNode = graphicEngine
        ? selectedObject->PointOnNode(graphicEngine, X, Y, graphicEngine->GetMouseTolerance())
        : selectedObject->PointOnNode(point, MouseToleranceReal());

    if (miNode != -1) {
        mbDragOK = true;
        mpObject = selectedObject->Clone();
        SetState(OSTATE_RUNNING);
        XMouseStart = X;
        YMouseStart = Y;
        XMouseJet = X;
        YMouseJet = Y;
    } else if (clickOnObject) {
        mbDragOK = false;
        SetState(OSTATE_STARTED);
    } else {
        mbDragOK = false;
        SetState(OSTATE_ABORTED);
    }
}

void __fastcall TDVOMMoveNode::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT || !mbDragOK) {
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (selectedObject && miNode != -1) {
        const TDMatPoint point{X, Y};
        if (selectedObject->MoveNode(miNode, X - XMouseStart, Y - YMouseStart, point)) {
            selectedObject->Initialize();
            mpVecModel->MarkChanged();
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        bool bDestroyed = false;
        SetState(OSTATE_FINISHED, &bDestroyed);
    }
}

void __fastcall TDVOMMoveNode::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button == VIRTMOUSEBTM_LEFT && mbDragOK && mpObject && miNode != -1) {
        const TDMatPoint point{X, Y};
        mpObject->MoveNode(miNode, X - XMouseJet, Y - YMouseJet, point);
        mpVecEditCad->TmpAppend(mpObject.get(), true);
        XMouseJet = X;
        YMouseJet = Y;
    }
}

void __fastcall TDVOMMoveNode::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMMoveNode::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
