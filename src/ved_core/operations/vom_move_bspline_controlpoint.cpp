//---------------------------------------------------------------------------
// MODULE: View-Operationen-Modify-Move-BSPLine-ControlPoint
//---------------------------------------------------------------------------
#define __VOM_MOVE_BSPLINE_CONTROLEPOINT_CPP

#include "vom_move_bspline_controlpoint.h"

#include "vec_bezier_curve.h"
#include "vec_bspline.h"
#include "vec_edit_cad.h"
#include "vec_model.h"

TDVOMMoveBSPLineControlPoint::TDVOMMoveBSPLineControlPoint(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpTmpObject(nullptr),
      mbDragOK(false),
      miControle(-1) {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (selectedObject && (selectedObject->GetType() == VECOBJ_BZC || selectedObject->GetType() == VECOBJ_BSPLINE)) {
        SetState(OSTATE_STARTED);
        mpVecEditCad->UseCursor(VECVIEW_SIMPLE_CROSS);
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }
}

TDVOMMoveBSPLineControlPoint::TDVOMMoveBSPLineControlPoint(const TDVOMMoveBSPLineControlPoint& oldOperation)
    : TDVOModify(oldOperation),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbDragOK(oldOperation.mbDragOK),
      miControle(oldOperation.miControle) {
}

TDVOMMoveBSPLineControlPoint& TDVOMMoveBSPLineControlPoint::operator=(const TDVOMMoveBSPLineControlPoint& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbDragOK = oldOperation.mbDragOK;
    miControle = oldOperation.miControle;
    return *this;
}

TDVOMMoveBSPLineControlPoint* TDVOMMoveBSPLineControlPoint::Clone() const {
    return new TDVOMMoveBSPLineControlPoint(*this);
}

void __fastcall TDVOMMoveBSPLineControlPoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mbDragOK = false;
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_STARTED);
        mpTmpObject.reset();
        return;
    }
    if (Button != VIRTMOUSEBTM_LEFT || !graphicEngine) {
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || mpVecModel->GetSelectedObjectIndex() == -1) {
        SetState(OSTATE_ABORTED);
        mbDragOK = false;
        return;
    }

    bool clickOnObject = false;
    if (auto* bezier = dynamic_cast<TDVecBezierCurve*>(selectedObject)) {
        clickOnObject = bezier->HitTest(graphicEngine, {X, Y}, graphicEngine->GetMouseTolerance()).IsHit();
        miControle = bezier->PointOnControle(graphicEngine, X, Y, graphicEngine->GetMouseTolerance());
    } else if (auto* bspline = dynamic_cast<TDVecBSPLine*>(selectedObject)) {
        clickOnObject = bspline->HitTest(graphicEngine, {X, Y}, graphicEngine->GetMouseTolerance()).IsHit();
        miControle = bspline->PointOnControle(graphicEngine, X, Y, graphicEngine->GetMouseTolerance());
    } else {
        SetState(OSTATE_ABORTED);
        mbDragOK = false;
        return;
    }

    if (miControle != -1) {
        mbDragOK = true;
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
        mpTmpObject.reset();
    }
}

void __fastcall TDVOMMoveBSPLineControlPoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button == VIRTMOUSEBTM_RIGHT) {
        mbDragOK = false;
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_STARTED);
        mpTmpObject.reset();
        return;
    }
    if (Button != VIRTMOUSEBTM_LEFT || !mbDragOK) {
        return;
    }

    bool moved = false;
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(selectedObject)) {
        bspline->MoveControle(miControle, X - XMouseStart, Y - YMouseStart);
        bspline->Initialize();
        moved = true;
    } else if (auto* bezier = dynamic_cast<TDVecBezierCurve*>(selectedObject)) {
        bezier->MoveControle(miControle, X - XMouseStart, Y - YMouseStart);
        bezier->Initialize();
        moved = true;
    }
    if (moved) {
        mpVecModel->MarkChanged();
    }

    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
}

void __fastcall TDVOMMoveBSPLineControlPoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT || !mbDragOK || miControle == -1) {
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject) {
        return;
    }

    mpTmpObject = selectedObject->Clone();
    if (auto* bezier = dynamic_cast<TDVecBezierCurve*>(mpTmpObject.get())) {
        bezier->MoveControle(miControle, X - XMouseStart, Y - YMouseStart);
        bezier->Initialize();
        mpVecEditCad->TmpAppend(bezier, true);
    } else if (auto* bspline = dynamic_cast<TDVecBSPLine*>(mpTmpObject.get())) {
        bspline->MoveControle(miControle, X - XMouseStart, Y - YMouseStart);
        bspline->Initialize();
        mpVecEditCad->TmpAppend(bspline, true);
    }
    XMouseJet = X;
    YMouseJet = Y;
}

void __fastcall TDVOMMoveBSPLineControlPoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMMoveBSPLineControlPoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
