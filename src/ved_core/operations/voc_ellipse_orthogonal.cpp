//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Ellipse-Orthogonal
//---------------------------------------------------------------------------
#define __VOC_ELLIPSE_ORTHOGONAL_CPP

#include "voc_ellipse_orthogonal.h"

#include "vec_ellipse.h"
#include "vec_model.h"

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCEllipseOrthogonal::TDVOCEllipseOrthogonal(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjEllipse(std::make_unique<TDVecEllipse>()),
      mbEllipseOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_NOTROTATED_1);
}

TDVOCEllipseOrthogonal* TDVOCEllipseOrthogonal::Clone() const {
    return new TDVOCEllipseOrthogonal(mpVecModel, mpVecEditCad, mpParentOperationManager);
}

bool TDVOCEllipseOrthogonal::IsEllipseValid() const {
    const double tolerance = minimumDistance(mpVecEditCad);
    return mMatEllipse.xRadius > tolerance && mMatEllipse.yRadius > tolerance;
}

void TDVOCEllipseOrthogonal::Reset() {
    mbEllipseOK = false;
    IniInteractivePoints();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_NOTROTATED_1);
}

void TDVOCEllipseOrthogonal::AppendEllipse() {
    auto* object = new TDVecEllipse();
    object->Import(mMatEllipse);
    object->SetSelect(false);
    object->SetColor(mpVecModel->GetDefaultColor());
    object->Initialize();
    mpVecModel->AppendObject(object);
    Reset();
    SetState(OSTATE_FINISHED);
}

void __fastcall TDVOCEllipseOrthogonal::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbEllipseOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_NOTROTATED_1);
        break;
    default:
        break;
    }
    if (Button == VIRTMOUSEBTM_RIGHT) {
        Reset();
        SetState(OSTATE_FINISHED);
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
    } else if (mClick == 2 && !mbEllipseOK) {
        mClick = 1;
        mpVecEditCad->Beep();
    }
}

void __fastcall TDVOCEllipseOrthogonal::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button == VIRTMOUSEBTM_LEFT && mClick == 2 && mbEllipseOK) {
        AppendEllipse();
    }
}

void __fastcall TDVOCEllipseOrthogonal::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatEllipse.CreateDiagonal(mPtBeg, mPtEnd);
        mbEllipseOK = IsEllipseValid();
        mpVecEditCad->UseCursor(mbEllipseOK ? VECVIEW_CREATE_ELLIPSE_NOTROTATED_2 : VECVIEW_CREATE_ELLIPSE_NOTROTATED_1);
        mpObjEllipse->Import(mMatEllipse);
        mpVecEditCad->TmpAppend(mpObjEllipse.get(), true);
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCEllipseOrthogonal::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCEllipseOrthogonal::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
