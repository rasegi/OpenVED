//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Circle-Midpoint
//---------------------------------------------------------------------------
#define __VOC_CIRCLE_MIDPOINT_CPP

#include "voc_circle_midpoint.h"

#include "vec_ellipse.h"
#include "vec_model.h"

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCCircleMidpoint::TDVOCCircleMidpoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjCircle(std::make_unique<TDVecEllipse>()),
      mbCircleOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

TDVOCCircleMidpoint* TDVOCCircleMidpoint::Clone() const {
    return new TDVOCCircleMidpoint(mpVecModel, mpVecEditCad, mpParentOperationManager);
}

bool TDVOCCircleMidpoint::IsCircleValid() const {
    return mMatCircle.r > minimumDistance(mpVecEditCad);
}

void TDVOCCircleMidpoint::Reset() {
    mbCircleOK = false;
    IniInteractivePoints();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

void TDVOCCircleMidpoint::AppendCircle() {
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

void __fastcall TDVOCCircleMidpoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbCircleOK = false;
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
        IniInteractivePoints();
    }
    ++mClick;
    switch (mClick) {
    case 1:
        mPtBeg = {X, Y};
        mPtEnd = {X, Y};
        break;
    case 2:
        if (mbCircleOK) {
            mPtEnd = {X, Y};
        } else {
            mClick = 1;
            mpVecEditCad->Beep();
        }
        break;
    default:
        break;
    }
}

void __fastcall TDVOCCircleMidpoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button == VIRTMOUSEBTM_LEFT && mClick >= 2 && mbCircleOK) {
        AppendCircle();
    }
}

void __fastcall TDVOCCircleMidpoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatCircle = MatCircleMitteReal(mPtBeg, mPtEnd);
        mbCircleOK = IsCircleValid();
        mpVecEditCad->UseCursor(mbCircleOK ? VECVIEW_CREATE_CIRCLE_2 : VECVIEW_CREATE_CIRCLE_1);
        mpObjCircle->SetCenter(mMatCircle.xCenter, mMatCircle.yCenter);
        mpObjCircle->SetXRadius(mMatCircle.r);
        mpObjCircle->SetYRadius(mMatCircle.r);
        mpObjCircle->SetAngle(0.0);
        mpVecEditCad->TmpAppend(mpObjCircle.get(), true);
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCCircleMidpoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCCircleMidpoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
