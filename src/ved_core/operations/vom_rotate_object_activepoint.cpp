//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Rotate-Object-ActivePoint
//---------------------------------------------------------------------------
#define __VOM_ROTATE_OBJECT_ACTIVEPOINT_CPP

#include "vom_rotate_object_activepoint.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vop_manager.h"

TDVOMRotateObjectActivePoint::TDVOMRotateObjectActivePoint(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOMRotateObjectActPtExtVar(nullptr),
      mTmpObjects(),
      mShadowObjects(),
      mbRotateOK(false),
      mbRotated(false),
      mnAngle(0.0),
      mbSelect(false),
      mbCopy(false) {
    mpVOPExternalVariables = new TDVOMRotateObjectActPtExtVar(this);
    mpVOMRotateObjectActPtExtVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(mpVOPExternalVariables);
    mpVecEditCad->UseCursor(VECVIEW_SIMPLE_CROSS);
}

TDVOMRotateObjectActivePoint::~TDVOMRotateObjectActivePoint() {
    Finish();
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOMRotateObjectActPtExtVar = nullptr;
}

TDVOMRotateObjectActivePoint::TDVOMRotateObjectActivePoint(const TDVOMRotateObjectActivePoint& oldOperation)
    : TDVOModify(oldOperation),
      mpVOMRotateObjectActPtExtVar(nullptr),
      mTmpObjects(),
      mShadowObjects(),
      mbRotateOK(oldOperation.mbRotateOK),
      mbRotated(oldOperation.mbRotated),
      mnAngle(oldOperation.mnAngle),
      mbSelect(oldOperation.mbSelect),
      mbCopy(oldOperation.mbCopy) {
    mpVOPExternalVariables = oldOperation.mpVOMRotateObjectActPtExtVar
        ? oldOperation.mpVOMRotateObjectActPtExtVar->Clone()
        : new TDVOMRotateObjectActPtExtVar(this);
    mpVOMRotateObjectActPtExtVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(mpVOPExternalVariables);
    for (const auto& object : oldOperation.mTmpObjects) {
        if (object) {
            mTmpObjects.push_back(object->Clone());
        }
    }
    for (const auto& object : oldOperation.mShadowObjects) {
        if (object) {
            mShadowObjects.push_back(object->Clone());
        }
    }
}

TDVOMRotateObjectActivePoint& TDVOMRotateObjectActivePoint::operator=(const TDVOMRotateObjectActivePoint& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = oldOperation.mpVOMRotateObjectActPtExtVar
        ? oldOperation.mpVOMRotateObjectActPtExtVar->Clone()
        : new TDVOMRotateObjectActPtExtVar(this);
    mpVOMRotateObjectActPtExtVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(mpVOPExternalVariables);
    mTmpObjects.clear();
    for (const auto& object : oldOperation.mTmpObjects) {
        if (object) {
            mTmpObjects.push_back(object->Clone());
        }
    }
    mShadowObjects.clear();
    for (const auto& object : oldOperation.mShadowObjects) {
        if (object) {
            mShadowObjects.push_back(object->Clone());
        }
    }
    mbRotateOK = oldOperation.mbRotateOK;
    mbRotated = oldOperation.mbRotated;
    mnAngle = oldOperation.mnAngle;
    mbSelect = oldOperation.mbSelect;
    mbCopy = oldOperation.mbCopy;
    return *this;
}

TDVOMRotateObjectActivePoint* TDVOMRotateObjectActivePoint::Clone() const {
    return new TDVOMRotateObjectActivePoint(*this);
}

void TDVOMRotateObjectActivePoint::Initialize() {
    mpVOMRotateObjectActPtExtVar->SetInitializedFlag(false);
    if (mpVecModel->CountSelectedObjects() == 0) {
        SetState(OSTATE_NEEDSELECTED);
        return;
    }

    mTmpObjects.clear();
    mShadowObjects.clear();
    for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
        if (object) {
            mTmpObjects.push_back(object->Clone());
        }
    }
    mpVOMRotateObjectActPtExtVar->SetNumberOfObjects(static_cast<unsigned long>(mpVecModel->CountSelectedObjects()));
    SetState(OSTATE_RUNNING);
    mpVOMRotateObjectActPtExtVar->SendUpdateToOPManager();
}

void TDVOMRotateObjectActivePoint::ExtVarIsChanged() {
    mnAngle = mpVOMRotateObjectActPtExtVar->GetAngle();
    mbCopy = mpVOMRotateObjectActPtExtVar->GetCopyFlag();
    mbSelect = mpVOMRotateObjectActPtExtVar->GetSelectFlag();
    mpVOMRotateObjectActPtExtVar->SetInitializedFlag(true);
    mbRotateOK = true;
    mbRotated = false;
}

void TDVOMRotateObjectActivePoint::Finish() {
    mTmpObjects.clear();
    mShadowObjects.clear();
}

void __fastcall TDVOMRotateObjectActivePoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpParentOperationManager->AktivateDefaultOperation();
    }
    if (Button == VIRTMOUSEBTM_LEFT) {
        SetState(OSTATE_RUNNING);
    }
}

void __fastcall TDVOMRotateObjectActivePoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (GetState() != OSTATE_RUNNING || Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    const TDMatPoint rotatePoint{X, Y};
    if (!mbCopy) {
        bool rotatedAny = false;
        for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
            if (object) {
                if (object->Rotate(rotatePoint, mnAngle)) {
                    object->Initialize();
                    rotatedAny = true;
                }
            }
        }
        if (rotatedAny) {
            mpVecModel->MarkChanged();
        }
    } else {
        std::vector<TDVecObject*> originals = mpVecModel->GetSelectedObjects();
        if (mbSelect) {
            mpVecModel->DeselectAll();
        }
        for (TDVecObject* object : originals) {
            if (!object) {
                continue;
            }
            auto clone = object->Clone();
            clone->Rotate(rotatePoint, mnAngle);
            clone->SetSelect(mbSelect);
            clone->Initialize();
            mpVecModel->AppendObject(clone.release());
        }
    }

    Initialize();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    if (ISActiveChangeToDefOPAfterFinish()) {
        bool bDestroyed = false;
        SetState(OSTATE_FINISHED, &bDestroyed);
        if (bDestroyed) {
            return;
        }
    }
}

void __fastcall TDVOMRotateObjectActivePoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    if (GetState() != OSTATE_RUNNING || !mbRotateOK) {
        return;
    }

    const TDMatPoint rotatePoint{X, Y};
    bool first = true;
    mShadowObjects.clear();
    for (const auto& object : mTmpObjects) {
        if (!object) {
            continue;
        }
        auto tmpObject = object->Clone();
        tmpObject->Rotate(rotatePoint, mnAngle);
        if (first) {
            mpVecEditCad->TmpAppend(tmpObject.get(), true);
            first = false;
        } else {
            mpVecEditCad->TmpAppendAdditional(tmpObject.get(), true);
        }
        mShadowObjects.push_back(std::move(tmpObject));
    }
    mbRotated = true;
    mpVOMRotateObjectActPtExtVar->SetPivotPoint(rotatePoint);
    mpVOMRotateObjectActPtExtVar->SendUpdateToOPManager();
}

void __fastcall TDVOMRotateObjectActivePoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMRotateObjectActivePoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOMRotateObjectActPtExtVar::TDVOMRotateObjectActPtExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mnObjects(0),
      mnAngle(0.0),
      mPivotPoint{0.0, 0.0},
      mbCopy(false),
      mbSelect(false),
      mbInitialized(false) {
}

TDVOMRotateObjectActPtExtVar::TDVOMRotateObjectActPtExtVar(const TDVOMRotateObjectActPtExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mnObjects(oldExtVar.mnObjects),
      mnAngle(oldExtVar.mnAngle),
      mPivotPoint(oldExtVar.mPivotPoint),
      mbCopy(oldExtVar.mbCopy),
      mbSelect(oldExtVar.mbSelect),
      mbInitialized(oldExtVar.mbInitialized) {
}

TDVOMRotateObjectActPtExtVar& TDVOMRotateObjectActPtExtVar::operator=(const TDVOMRotateObjectActPtExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mnObjects = oldExtVar.mnObjects;
    mnAngle = oldExtVar.mnAngle;
    mPivotPoint = oldExtVar.mPivotPoint;
    mbCopy = oldExtVar.mbCopy;
    mbSelect = oldExtVar.mbSelect;
    mbInitialized = oldExtVar.mbInitialized;
    return *this;
}

TDVOMRotateObjectActPtExtVar* TDVOMRotateObjectActPtExtVar::Clone() const {
    return new TDVOMRotateObjectActPtExtVar(*this);
}

void TDVOMRotateObjectActPtExtVar::SetAngle(double nAngle) {
    mnAngle = nAngle;
}

double TDVOMRotateObjectActPtExtVar::GetAngle() const {
    return mnAngle;
}

void TDVOMRotateObjectActPtExtVar::SetPivotPoint(TDMatPoint pivotPoint) {
    mPivotPoint = pivotPoint;
}

TDMatPoint TDVOMRotateObjectActPtExtVar::GetPivotPoint() const {
    return mPivotPoint;
}

void TDVOMRotateObjectActPtExtVar::SetCopyFlag(bool bCopy) {
    mbCopy = bCopy;
}

bool TDVOMRotateObjectActPtExtVar::GetCopyFlag() const {
    return mbCopy;
}

void TDVOMRotateObjectActPtExtVar::SetNumberOfObjects(unsigned long nObjects) {
    mnObjects = nObjects;
}

unsigned long TDVOMRotateObjectActPtExtVar::GetNumberOfObjects() const {
    return mnObjects;
}

void TDVOMRotateObjectActPtExtVar::SetSelectFlag(bool bSelect) {
    mbSelect = bSelect;
}

bool TDVOMRotateObjectActPtExtVar::GetSelectFlag() const {
    return mbSelect;
}

void TDVOMRotateObjectActPtExtVar::SetInitializedFlag(bool bInitialized) {
    mbInitialized = bInitialized;
}

bool TDVOMRotateObjectActPtExtVar::GetInitializedFlag() const {
    return mbInitialized;
}
