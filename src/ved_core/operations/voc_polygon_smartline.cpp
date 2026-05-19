//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Polygon-Smartline
//---------------------------------------------------------------------------
#define __VOC_POLYGON_SMARTLINE_CPP

#include "voc_polygon_smartline.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon.h"

TDVOCPolygonSmartline::TDVOCPolygonSmartline(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpTmpObject(nullptr),
      mMatPoints() {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYGON_SMARTLINE_1);
}

TDVOCPolygonSmartline::TDVOCPolygonSmartline(const TDVOCPolygonSmartline& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mMatPoints(oldOperation.mMatPoints) {
}

TDVOCPolygonSmartline& TDVOCPolygonSmartline::operator=(const TDVOCPolygonSmartline& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mMatPoints = oldOperation.mMatPoints;
    return *this;
}

TDVOCPolygonSmartline* TDVOCPolygonSmartline::Clone() const {
    return new TDVOCPolygonSmartline(*this);
}

std::unique_ptr<TDVecPolygon> TDVOCPolygonSmartline::CreatePolygonFromPoints() const {
    auto polygon = std::make_unique<TDVecPolygon>();
    for (const TDMatPoint& point : mMatPoints) {
        polygon->AppendPoint(point);
    }
    return polygon;
}

void TDVOCPolygonSmartline::Finish() {
    IniInteractivePoints();
    mMatPoints.clear();
    mpTmpObject.reset();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
    if (bDestroyed) {
        return;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYGON_SMARTLINE_1) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYGON_SMARTLINE_1);
    }
}

void __fastcall TDVOCPolygonSmartline::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYGON_SMARTLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYGON_SMARTLINE_1);
        }
        break;
    case OSTATE_FINISHED:
        IniInteractivePoints();
        mMatPoints.clear();
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYGON_SMARTLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYGON_SMARTLINE_1);
        }
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_LEFT && !(Shift & VKState_KEY_SHIFT)) {
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
    } else if (Button == VIRTMOUSEBTM_LEFT && (Shift & VKState_KEY_SHIFT) && !mMatPoints.empty()) {
        const TDMatPoint firstPoint = mMatPoints.front();
        mPtBeg = firstPoint;
        mPtEnd = firstPoint;
        mMatPoints.push_back(firstPoint);
    }

    if (mMatPoints.size() > 1 && mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYGON_SMARTLINE_2) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYGON_SMARTLINE_2);
    }
}

void __fastcall TDVOCPolygonSmartline::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && !(Shift & VKState_KEY_SHIFT)) {
        mClick = 1;
        mpTmpObject = CreatePolygonFromPoints();
        mpVecEditCad->TmpAppend(mpTmpObject.get(), false);
        return;
    }

    if (Button == VIRTMOUSEBTM_LEFT && (Shift & VKState_KEY_SHIFT) && !mMatPoints.empty()) {
        mClick = 1;
        mMatPoints.push_back(mMatPoints.front());
        mpTmpObject = CreatePolygonFromPoints();
        mpVecEditCad->TmpAppend(mpTmpObject.get(), false);
        return;
    }

    if (Button == VIRTMOUSEBTM_RIGHT && !(Shift & VKState_KEY_SHIFT)) {
        if (mMatPoints.size() > 1) {
            TDVecPolygon* polygon = CreatePolygonFromPoints().release();
            polygon->SetSelect(false);
            polygon->SetColor(mpVecModel->GetDefaultColor());
            polygon->Initialize();
            mpVecModel->AppendObject(polygon);
            IniInteractivePoints();
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
        return;
    }

    if (Button == VIRTMOUSEBTM_RIGHT && (Shift & VKState_KEY_SHIFT)) {
        if (mMatPoints.size() > 1) {
            TDVecPolygon* polygon = CreatePolygonFromPoints().release();
            polygon->AppendPoint(mMatPoints.front());
            polygon->SetSelect(false);
            polygon->SetColor(mpVecModel->GetDefaultColor());
            polygon->Initialize();
            mpVecModel->AppendObject(polygon);
            IniInteractivePoints();
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
    }
}

void __fastcall TDVOCPolygonSmartline::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        auto tmpPolygon = CreatePolygonFromPoints();
        tmpPolygon->AppendPoint(mPtEnd);
        tmpPolygon->SetSelect(false);
        mpTmpObject = std::move(tmpPolygon);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCPolygonSmartline::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCPolygonSmartline::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
