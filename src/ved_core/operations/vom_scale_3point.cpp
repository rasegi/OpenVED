//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Scale-Object-3Point
//---------------------------------------------------------------------------
#define __VOM_SCALE_OBJECT_3POINT_CPP

#include "vom_scale_3point.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <cmath>

TDVOMScaleObject3Point::TDVOMScaleObject3Point(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpCloneObject(nullptr),
      mpTmpObject(nullptr),
      mnXScale(1.0),
      mnYScale(1.0),
      mnXBasis(0.0),
      mnYBasis(0.0),
      MatPoint1{},
      MatPoint2{},
      mnClick(0),
      mbScaleOK(false) {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject) {
        SetState(OSTATE_NEEDSELECTED);
    } else if (selectedObject->GetLockResize() || selectedObject->GetLockPosition()) {
        mpVecEditCad->Beep();
        SetState(OSTATE_NEEDSELECTED);
    } else {
        SetState(OSTATE_STARTED);
        mpVecEditCad->UseCursor(VECVIEW_SIMPLE_CROSS);
    }
}

TDVOMScaleObject3Point::TDVOMScaleObject3Point(const TDVOMScaleObject3Point& oldOperation)
    : TDVOModify(oldOperation),
      mpCloneObject(oldOperation.mpCloneObject ? oldOperation.mpCloneObject->Clone() : nullptr),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mnXScale(oldOperation.mnXScale),
      mnYScale(oldOperation.mnYScale),
      mnXBasis(oldOperation.mnXBasis),
      mnYBasis(oldOperation.mnYBasis),
      MatPoint1(oldOperation.MatPoint1),
      MatPoint2(oldOperation.MatPoint2),
      mnClick(oldOperation.mnClick),
      mbScaleOK(oldOperation.mbScaleOK) {
}

TDVOMScaleObject3Point& TDVOMScaleObject3Point::operator=(const TDVOMScaleObject3Point& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    mpCloneObject = oldOperation.mpCloneObject ? oldOperation.mpCloneObject->Clone() : nullptr;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mnXScale = oldOperation.mnXScale;
    mnYScale = oldOperation.mnYScale;
    mnXBasis = oldOperation.mnXBasis;
    mnYBasis = oldOperation.mnYBasis;
    MatPoint1 = oldOperation.MatPoint1;
    MatPoint2 = oldOperation.MatPoint2;
    mnClick = oldOperation.mnClick;
    mbScaleOK = oldOperation.mbScaleOK;
    return *this;
}

TDVOMScaleObject3Point* TDVOMScaleObject3Point::Clone() const {
    return new TDVOMScaleObject3Point(*this);
}

void __fastcall TDVOMScaleObject3Point::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
        SetState(OSTATE_RUNNING);
        break;
    case OSTATE_FINISHED:
        mbScaleOK = false;
        mnClick = 0;
        mnXScale = 1.0;
        mnYScale = 1.0;
        mpCloneObject.reset();
        mpTmpObject.reset();
        SetState(OSTATE_RUNNING);
        break;
    default:
        break;
    }

    if (mpVecModel->GetSelectedObject() && mpVecModel->GetSelectedObjectIndex() != -1) {
        if (Button == VIRTMOUSEBTM_LEFT && GetState() == OSTATE_RUNNING) {
            switch (mnClick) {
            case 0:
                MatPoint1 = {X, Y};
                mpVecEditCad->AppendTempNode(X, Y, NODE_NORMAL, false);
                ++mnClick;
                break;
            case 1:
                mpCloneObject = mpVecModel->GetSelectedObject()->Clone();
                MatPoint2 = {X, Y};
                mnXBasis = MatPoint1.x - MatPoint2.x;
                mnYBasis = MatPoint1.y - MatPoint2.y;
                mpVecEditCad->AppendTempNode(X, Y, NODE_NORMAL, false);
                mbScaleOK = true;
                ++mnClick;
                break;
            case 2:
                ++mnClick;
                break;
            default:
                break;
            }
        } else {
            mpVecEditCad->TmpClear();
            mpVecEditCad->ViewsRefresh();
            mbScaleOK = false;
            mnClick = 0;
            bool bDestroyed = false;
            SetState(OSTATE_FINISHED, &bDestroyed);
        }
    }
}

void __fastcall TDVOMScaleObject3Point::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)Y;
    if (mbScaleOK && mpCloneObject && mnClick == 3 && GetState() == OSTATE_RUNNING) {
        TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
        if (!selectedObject) {
            return;
        }
        mnXScale = mnXBasis != 0.0 ? (MatPoint1.x - X) / mnXBasis : 1.0;
        mnYScale = mnYBasis != 0.0 ? (MatPoint1.y - Y) / mnYBasis : 1.0;

        if (selectedObject->ToScale(MatPoint1, mnXScale, mnYScale)) {
            selectedObject->Initialize();
            mpVecModel->MarkChanged();
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        mbScaleOK = false;
        mnClick = 0;
        SetState(OSTATE_FINISHED);
    }
}

void __fastcall TDVOMScaleObject3Point::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    if (mbScaleOK && mpCloneObject && mnClick == 2 && GetState() == OSTATE_RUNNING) {
        mpTmpObject = mpCloneObject->Clone();
        mnXScale = mnXBasis != 0.0 ? (MatPoint1.x - X) / mnXBasis : 1.0;
        mnYScale = mnYBasis != 0.0 ? (MatPoint1.y - Y) / mnYBasis : 1.0;
        mpTmpObject->ToScale(MatPoint1, mnXScale, mnYScale);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
    }
}

void __fastcall TDVOMScaleObject3Point::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMScaleObject3Point::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
