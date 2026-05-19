//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Ellipse-Midpoint
//---------------------------------------------------------------------------
#define __VOC_ELLIPSE_MIDPOINT_CPP

#include "voc_ellipse_midpoint.h"

#include "vec_ellipse.h"
#include "vec_line.h"
#include "vec_model.h"

#include <utility>

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCEllipseMidpoint::TDVOCEllipseMidpoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjEllipse(std::make_unique<TDVecEllipse>()),
      mpTmpLine(std::make_unique<TDVecLine>()),
      mbAxeOK(false),
      mbEllipseOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_ROTATED_1);
}

TDVOCEllipseMidpoint* TDVOCEllipseMidpoint::Clone() const {
    return new TDVOCEllipseMidpoint(mpVecModel, mpVecEditCad, mpParentOperationManager);
}

bool TDVOCEllipseMidpoint::IsAxisValid() const {
    return mMatLine.Length() > minimumDistance(mpVecEditCad);
}

bool TDVOCEllipseMidpoint::IsEllipseValid() const {
    const double tolerance = minimumDistance(mpVecEditCad);
    return mMatEllipse.xRadius > tolerance && mMatEllipse.yRadius > tolerance;
}

void TDVOCEllipseMidpoint::Reset() {
    mbAxeOK = false;
    mbEllipseOK = false;
    IniInteractivePoints();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_ROTATED_1);
}

void TDVOCEllipseMidpoint::AppendEllipse() {
    TDMatEllipse ellipse = mMatEllipse;
    if (ellipse.nAngle == 90.0 || ellipse.nAngle == 270.0) {
        std::swap(ellipse.xRadius, ellipse.yRadius);
        ellipse.nAngle -= 90.0;
    }

    auto* object = new TDVecEllipse();
    object->SetCenter(ellipse.xCenter, ellipse.yCenter);
    object->SetXRadius(ellipse.xRadius);
    object->SetYRadius(ellipse.yRadius);
    object->SetAngle(ellipse.nAngle);
    object->SetColor(mpVecModel->GetDefaultColor());
    object->Initialize();
    mpVecModel->AppendObject(object);
    Reset();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOCEllipseMidpoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbAxeOK = false;
        mbEllipseOK = false;
        SetState(OSTATE_RUNNING);
        IniInteractivePoints();
        mpVecEditCad->UseCursor(VECVIEW_CREATE_ELLIPSE_ROTATED_1);
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
    if (mClick > 3) {
        mClick = 0;
    }
    ++mClick;
    switch (mClick) {
    case 1:
        mPtBeg = {X, Y};
        mPtPoint = {X, Y};
        break;
    case 2:
    {
        const TDMatPoint axisEnd{X, Y};
        mMatLine.Create(mPtBeg, axisEnd);
        mbAxeOK = IsAxisValid();
        if (mbAxeOK) {
            mPtEnd = axisEnd;
            mPtPoint = axisEnd;
        } else {
            mClick = 1;
            mpVecEditCad->Beep();
        }
        break;
    }
    case 3:
        mMatEllipse = MatEllipseMitte3PointReal(mPtBeg, mPtEnd, {X, Y});
        mbEllipseOK = IsEllipseValid();
        if (!mbEllipseOK) {
            mClick = 2;
            mpVecEditCad->Beep();
        }
        break;
    default:
        break;
    }
}

void __fastcall TDVOCEllipseMidpoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    switch (mClick) {
    case 1:
    case 2:
        return;
    case 3:
        mMatEllipse = MatEllipseMitte3PointReal(mPtBeg, mPtEnd, {X, Y});
        mbEllipseOK = IsEllipseValid();
        if (mbEllipseOK && mbAxeOK) {
            AppendEllipse();
        } else {
            mpVecEditCad->Beep();
            Reset();
            SetState(OSTATE_FINISHED);
        }
        return;
    default:
        return;
    }
}

void __fastcall TDVOCEllipseMidpoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (mClick == 3 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    switch (mClick) {
    case 1:
        mPtPoint = {X, Y};
        mMatLine.Create(mPtBeg, mPtPoint);
        mbAxeOK = IsAxisValid();
        mpTmpLine->SetLine(&mMatLine);
        mpVecEditCad->TmpAppend(mpTmpLine.get(), true);
        break;
    case 2:
        mPtPoint = {X, Y};
        mMatEllipse = MatEllipseMitte3PointReal(mPtBeg, mPtEnd, mPtPoint);
        mbEllipseOK = IsEllipseValid();
        mpVecEditCad->UseCursor(mbEllipseOK ? VECVIEW_CREATE_ELLIPSE_ROTATED_2 : VECVIEW_CREATE_ELLIPSE_ROTATED_1);
        mpObjEllipse->Import(mMatEllipse);
        mpVecEditCad->TmpAppend(mpObjEllipse.get(), true);
        if (Button == VIRTMOUSEBTM_LEFT) {
            mClick = 3;
        }
        break;
    default:
        break;
    }
}

void __fastcall TDVOCEllipseMidpoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCEllipseMidpoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
