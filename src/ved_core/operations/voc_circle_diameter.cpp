//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Circle-Diameter
//---------------------------------------------------------------------------
#define __VOC_CIRCLE_DIAMETER_CPP

#include "voc_circle_diameter.h"

#include "vec_ellipse.h"
#include "vec_model.h"

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCCircleDiameter::TDVOCCircleDiameter(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjCircle(std::make_unique<TDVecEllipse>()),
      mbCircleOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

TDVOCCircleDiameter* TDVOCCircleDiameter::Clone() const {
    return new TDVOCCircleDiameter(mpVecModel, mpVecEditCad, mpParentOperationManager);
}

bool TDVOCCircleDiameter::IsCircleValid() const {
    return mMatCircle.r > minimumDistance(mpVecEditCad);
}

void TDVOCCircleDiameter::Reset() {
    mbCircleOK = false;
    IniInteractivePoints();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

void TDVOCCircleDiameter::AppendCircle() {
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

void __fastcall TDVOCCircleDiameter::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
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
    if (mClick == 1) {
        mPtBeg = {X, Y};
        mPtEnd = {X, Y};
    } else if (mClick == 2 && !mbCircleOK) {
        mClick = 1;
        mpVecEditCad->Beep();
    }
}

void __fastcall TDVOCCircleDiameter::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button == VIRTMOUSEBTM_LEFT && mClick == 2 && mbCircleOK) {
        AppendCircle();
    }
}

void __fastcall TDVOCCircleDiameter::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatCircle = MatCircleDurchReal(mPtBeg, mPtEnd);
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

void __fastcall TDVOCCircleDiameter::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCCircleDiameter::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
