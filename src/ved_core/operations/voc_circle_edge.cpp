//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Circle-Edge
//---------------------------------------------------------------------------
#define __VOC_CIRCLE_EDGE_CPP

#include "voc_circle_edge.h"

#include "vec_ellipse.h"
#include "vec_model.h"

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCCircleEdge::TDVOCCircleEdge(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjCircle(std::make_unique<TDVecEllipse>()),
      mbCircleOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

TDVOCCircleEdge* TDVOCCircleEdge::Clone() const {
    return new TDVOCCircleEdge(mpVecModel, mpVecEditCad, mpParentOperationManager);
}

void TDVOCCircleEdge::Reset() {
    mbCircleOK = false;
    IniInteractivePoints();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

bool TDVOCCircleEdge::UpdatePreviewCircle() {
    mbCircleOK = MatCircleKanteReal(mPtBeg, mPtPoint, mPtEnd, &mMatCircle) &&
                 mMatCircle.r > minimumDistance(mpVecEditCad);
    if (mbCircleOK) {
        mpObjCircle->SetCenter(mMatCircle.xCenter, mMatCircle.yCenter);
        mpObjCircle->SetXRadius(mMatCircle.r);
        mpObjCircle->SetYRadius(mMatCircle.r);
        mpObjCircle->SetAngle(0.0);
        mpVecEditCad->TmpAppend(mpObjCircle.get(), true);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_2);
    }
    return mbCircleOK;
}

bool TDVOCCircleEdge::UpdateFinalCircle(TDMatPoint edgePoint) {
    mPtEnd = edgePoint;
    mbCircleOK = MatCircleKanteReal(mPtBeg, mPtEnd, mPtPoint, &mMatCircle) &&
                 mMatCircle.r > minimumDistance(mpVecEditCad);
    return mbCircleOK;
}

void TDVOCCircleEdge::AppendCircle() {
    auto* object = new TDVecEllipse();
    object->SetCenter(mMatCircle.xCenter, mMatCircle.yCenter);
    object->SetXRadius(mMatCircle.r);
    object->SetYRadius(mMatCircle.r);
    object->SetAngle(0.0);
    object->SetColor(mpVecModel->GetDefaultColor());
    object->Initialize();
    mpVecModel->AppendObject(object);
    Reset();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOCCircleEdge::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        SetState(OSTATE_RUNNING);
        IniInteractivePoints();
        mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
        break;
    default:
        break;
    }
    if (Button == VIRTMOUSEBTM_RIGHT) {
        Reset();
        SetState(OSTATE_STARTED);
        return;
    }
    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }
    if (mClick > 2) {
        mClick = 0;
    }
    ++mClick;
    switch (mClick) {
    case 1:
        mPtBeg = {X, Y};
        mpVecEditCad->AppendTempNode(X, Y, NODE_NORMAL, false);
        break;
    case 2:
        mPtPoint = {X, Y};
        mPtEnd = {X, Y};
        mpVecEditCad->AppendTempNode(X, Y, NODE_NORMAL, false);
        break;
    default:
        break;
    }
}

void __fastcall TDVOCCircleEdge::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }
    switch (mClick) {
    case 1:
        return;
    case 2:
        if (mPtEnd.x == mPtPoint.x && mPtEnd.y == mPtPoint.y) {
            return;
        }
        if (UpdateFinalCircle({X, Y})) {
            AppendCircle();
        } else {
            Reset();
            SetState(OSTATE_FINISHED);
        }
        return;
    case 3:
        if (UpdateFinalCircle({X, Y})) {
            AppendCircle();
        } else {
            Reset();
            SetState(OSTATE_FINISHED);
        }
        return;
    default:
        return;
    }
}

void __fastcall TDVOCCircleEdge::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    if (mClick == 2) {
        UpdatePreviewCircle();
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCCircleEdge::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCCircleEdge::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
