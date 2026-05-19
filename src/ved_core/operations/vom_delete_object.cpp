//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Delete-Object
//---------------------------------------------------------------------------
#define __VOM_DELETE_OBJECT_CPP

#include "vom_delete_object.h"

#include "vec_edit_cad.h"

#include <algorithm>

TDVOMDeleteObject::TDVOMDeleteObject(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mClick(0),
      miIndexObject(-1) {
    SetState(OSTATE_RUNNING);
    mpVecEditCad->UseCursor(VECVIEW_DELETE_OBJECT);
}

TDVOMDeleteObject::~TDVOMDeleteObject() = default;

TDVOMDeleteObject::TDVOMDeleteObject(const TDVOMDeleteObject& oldOperation)
    : TDVOModify(oldOperation),
      mClick(oldOperation.mClick),
      miIndexObject(oldOperation.miIndexObject) {
}

TDVOMDeleteObject& TDVOMDeleteObject::operator=(const TDVOMDeleteObject& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    mClick = oldOperation.mClick;
    miIndexObject = oldOperation.miIndexObject;
    return *this;
}

TDVOMDeleteObject* TDVOMDeleteObject::Clone() const {
    return new TDVOMDeleteObject(*this);
}

TDVecModelHitResult TDVOMDeleteObject::FindHit(TDMatPoint point) const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (graphicEngine) {
        return mpVecModel->FindObjectAt(graphicEngine, point, graphicEngine->GetMouseTolerance());
    }
    return mpVecModel->FindObjectAt(point, MouseToleranceReal());
}

double TDVOMDeleteObject::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return 0.0;
    }

    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

void TDVOMDeleteObject::DeleteSelectedAndFinish() {
    if (mpVecModel->DeleteSelectedObjects() > 0) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }

    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void TDVOMDeleteObject::DeleteObjectAndFinish(int objectIndex) {
    if (mpVecModel->DeleteObject(objectIndex)) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }

    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOMDeleteObject::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    const TDVecModelHitResult hit = FindHit({X, Y});
    if (!hit.IsHit()) {
        mClick = 0;
        miIndexObject = -1;
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        return;
    }

    mpVecEditCad->TmpAppend(hit.object, true);
    if (miIndexObject == hit.objectIndex && mClick > 0) {
        DeleteObjectAndFinish(hit.objectIndex);
        return;
    }

    miIndexObject = hit.objectIndex;
    ++mClick;
}

void __fastcall TDVOMDeleteObject::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMDeleteObject::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMDeleteObject::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)StateKey;
    if (eVirtualKey == VIRTKEY_DELETE) {
        DeleteSelectedAndFinish();
    }
}

void __fastcall TDVOMDeleteObject::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
