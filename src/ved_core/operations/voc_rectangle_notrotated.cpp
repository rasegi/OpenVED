//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Rectangle-NotRotated
//---------------------------------------------------------------------------
#define __VOC_RECTANGLE_NOTROTATED_CPP

#include "voc_rectangle_notrotated.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon_area.h"

#include <memory>

namespace {
TDMatRect createOrthogonalRect(TDMatPoint p1, TDMatPoint p2) {
    return {
        {p1.x, p1.y},
        {p2.x, p1.y},
        {p2.x, p2.y},
        {p1.x, p2.y}
    };
}

TDMatPoint rectEdge(const TDMatRect& rect, unsigned int edge) {
    switch (edge) {
    case 1:
        return rect.P1;
    case 2:
        return rect.P2;
    case 3:
        return rect.P3;
    case 4:
        return rect.P4;
    default:
        return {};
    }
}
}

TDVOCRectangleNotRotated::TDVOCRectangleNotRotated(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mMatRect{},
      mpTmpObject(nullptr),
      mbRectangleOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
}

TDVOCRectangleNotRotated::TDVOCRectangleNotRotated(const TDVOCRectangleNotRotated& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mMatRect(oldOperation.mMatRect),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbRectangleOK(oldOperation.mbRectangleOK) {
}

TDVOCRectangleNotRotated& TDVOCRectangleNotRotated::operator=(const TDVOCRectangleNotRotated& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mMatRect = oldOperation.mMatRect;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbRectangleOK = oldOperation.mbRectangleOK;
    return *this;
}

TDVOCRectangleNotRotated* TDVOCRectangleNotRotated::Clone() const {
    return new TDVOCRectangleNotRotated(*this);
}

void __fastcall TDVOCRectangleNotRotated::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbRectangleOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_FINISHED);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
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
        mPtEnd = {X, Y};
        if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
            mbRectangleOK = false;
            mClick = 1;
            mpVecEditCad->Beep();
        } else {
            mbRectangleOK = true;
        }
        break;
    default:
        break;
    }
}

void __fastcall TDVOCRectangleNotRotated::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && mbRectangleOK && mClick == 2 && (mPtBeg.x != mPtEnd.x || mPtBeg.y != mPtEnd.y)) {
        mMatRect = createOrthogonalRect(mPtBeg, mPtEnd);

        auto* polygonArea = new TDVecPolygonArea();
        for (unsigned int i = 0; i < 4; ++i) {
            polygonArea->AppendPoint(rectEdge(mMatRect, i + 1));
        }
        polygonArea->SetSelect(false);
        polygonArea->SetRectangularLock(true);
        polygonArea->SetColor(mpVecModel->GetDefaultColor());
        polygonArea->Initialize();
        mpVecModel->AppendObject(polygonArea);

        IniInteractivePoints();
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        SetState(OSTATE_FINISHED);
    } else if (mClick == 2 && !mbRectangleOK) {
        mClick = 1;
        mpVecEditCad->Beep();
    }
}

void __fastcall TDVOCRectangleNotRotated::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatRect = createOrthogonalRect(mPtBeg, mPtEnd);
        auto tmpPolygonArea = std::make_unique<TDVecPolygonArea>();
        for (unsigned int i = 0; i < 4; ++i) {
            tmpPolygonArea->AppendPoint(rectEdge(mMatRect, i + 1));
        }
        tmpPolygonArea->SetSelect(false);
        mpTmpObject = std::move(tmpPolygonArea);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);

        mPtEnd = {X, Y};
        if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
            mbRectangleOK = false;
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        } else {
            mbRectangleOK = true;
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_2);
        }
    }
}

void __fastcall TDVOCRectangleNotRotated::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCRectangleNotRotated::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
