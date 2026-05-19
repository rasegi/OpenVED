//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-BezierCurve
//---------------------------------------------------------------------------
#define __VOC_BEZIERCURVE_CONTROLPOINT_CPP

#include "voc_beziercurve_controlpoint.h"

#include "vec_bezier_curve.h"
#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_polygon.h"

TDVOCBezierCurveControlPoint::TDVOCBezierCurveControlPoint(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjBezierCurve(std::make_unique<TDVecBezierCurve>()),
      mpTmpLine(std::make_unique<TDVecLine>()),
      mpTmpPolygon(nullptr),
      MatLine{},
      mMatPoints() {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
}

TDVOCBezierCurveControlPoint::TDVOCBezierCurveControlPoint(const TDVOCBezierCurveControlPoint& oldOperation)
    : TDVOCreate(oldOperation),
      mpObjBezierCurve(oldOperation.mpObjBezierCurve ? std::make_unique<TDVecBezierCurve>(*oldOperation.mpObjBezierCurve) : nullptr),
      mpTmpLine(oldOperation.mpTmpLine ? std::make_unique<TDVecLine>(*oldOperation.mpTmpLine) : nullptr),
      mpTmpPolygon(oldOperation.mpTmpPolygon ? std::make_unique<TDVecPolygon>(*oldOperation.mpTmpPolygon) : nullptr),
      MatLine(oldOperation.MatLine),
      mMatPoints(oldOperation.mMatPoints) {
}

TDVOCBezierCurveControlPoint& TDVOCBezierCurveControlPoint::operator=(const TDVOCBezierCurveControlPoint& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOCreate::operator=(oldOperation);
    mpObjBezierCurve = oldOperation.mpObjBezierCurve ? std::make_unique<TDVecBezierCurve>(*oldOperation.mpObjBezierCurve) : nullptr;
    mpTmpLine = oldOperation.mpTmpLine ? std::make_unique<TDVecLine>(*oldOperation.mpTmpLine) : nullptr;
    mpTmpPolygon = oldOperation.mpTmpPolygon ? std::make_unique<TDVecPolygon>(*oldOperation.mpTmpPolygon) : nullptr;
    MatLine = oldOperation.MatLine;
    mMatPoints = oldOperation.mMatPoints;
    return *this;
}

TDVOCBezierCurveControlPoint* TDVOCBezierCurveControlPoint::Clone() const {
    return new TDVOCBezierCurveControlPoint(*this);
}

void TDVOCBezierCurveControlPoint::Finish() {
    IniInteractivePoints();
    mpObjBezierCurve.reset();
    mpTmpLine.reset();
    mpTmpPolygon.reset();
    mMatPoints.clear();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
    if (bDestroyed) {
        return;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_1) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
    }
}

void __fastcall TDVOCBezierCurveControlPoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
        if (mMatPoints.empty()) {
            IniInteractivePoints();
        }
        SetState(OSTATE_RUNNING);
        break;
    case OSTATE_FINISHED:
        mpObjBezierCurve = std::make_unique<TDVecBezierCurve>();
        mpTmpLine = std::make_unique<TDVecLine>();
        mpTmpPolygon.reset();
        mMatPoints.clear();
        SetState(OSTATE_RUNNING);
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_LEFT && Shift != VKState_KEY_SHIFT) {
        if (mClick == 0) {
            mPtBeg = {X, Y};
            mPtEnd = {X, Y};
            mMatPoints.push_back(mPtBeg);
        } else {
            mPtBeg = mPtEnd;
            mPtEnd = {X, Y};
            mMatPoints.push_back(mPtEnd);
        }
        ++mClick;
    } else if (Button == VIRTMOUSEBTM_LEFT && Shift == VKState_KEY_SHIFT && !mMatPoints.empty()) {
        const TDMatPoint firstPoint = mMatPoints.front();
        mPtBeg = firstPoint;
        mPtEnd = firstPoint;
        mMatPoints.push_back(firstPoint);
    }

    if (mMatPoints.size() > 2 && mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_2) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_2);
    }
}

void __fastcall TDVOCBezierCurveControlPoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && Shift != VKState_KEY_SHIFT) {
        mClick = 1;
        mpTmpPolygon = std::make_unique<TDVecPolygon>();
        for (const TDMatPoint& point : mMatPoints) {
            mpTmpPolygon->AppendPoint(point);
        }
        mpVecEditCad->TmpAppend(mpTmpPolygon.get(), false);
    }

    if (Button == VIRTMOUSEBTM_RIGHT && Shift != VKState_KEY_SHIFT) {
        if (mMatPoints.size() > 2) {
            auto* pObjBezierCurve = new TDVecBezierCurve();
            for (const TDMatPoint& point : mMatPoints) {
                pObjBezierCurve->AppendPoint(point);
            }
            pObjBezierCurve->SetSelect(false);
            pObjBezierCurve->SetShowControls(true);
            pObjBezierCurve->SetShowPolygon(true);
            IniInteractivePoints();

            pObjBezierCurve->ComputeCurve();
            pObjBezierCurve->SetColor(mpVecModel->GetDefaultColor());
            pObjBezierCurve->Initialize();
            mpVecModel->AppendObject(pObjBezierCurve);
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
    }
}

void __fastcall TDVOCBezierCurveControlPoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mpTmpPolygon = std::make_unique<TDVecPolygon>();
        for (const TDMatPoint& point : mMatPoints) {
            mpTmpPolygon->AppendPoint(point);
        }
        mpTmpPolygon->AppendPoint(mPtEnd);
        mpTmpPolygon->SetSelect(false);
        mpVecEditCad->TmpAppend(mpTmpPolygon.get(), true);
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCBezierCurveControlPoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCBezierCurveControlPoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
