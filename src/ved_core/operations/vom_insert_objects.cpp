//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Insert-Objects
//---------------------------------------------------------------------------
#define __VOM_INSERT_OBJECTS_CPP

#include "vom_insert_objects.h"

#include "vec_edit_cad.h"
#include "vec_group.h"
#include "vec_model.h"

TDVOMInsertObjects::TDVOMInsertObjects(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOMInsertObjectsExtVar(nullptr),
      mpTmpVecGroup(nullptr),
      mpTmpObject(nullptr),
      mbGroupMove(false),
      mbDragOK(false) {
    mpVOPExternalVariables = new TDVOMInsertObjectsExtVar(this);
    mpVOMInsertObjectsExtVar = dynamic_cast<TDVOMInsertObjectsExtVar*>(mpVOPExternalVariables);
}

TDVOMInsertObjects::~TDVOMInsertObjects() {
    IniInteractivePoints();
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOMInsertObjectsExtVar = nullptr;
}

TDVOMInsertObjects::TDVOMInsertObjects(const TDVOMInsertObjects& oldOperation)
    : TDVOModify(oldOperation),
      mpVOMInsertObjectsExtVar(nullptr),
      mpTmpVecGroup(oldOperation.mpTmpVecGroup ? std::unique_ptr<TDVecGroup>(dynamic_cast<TDVecGroup*>(oldOperation.mpTmpVecGroup->Clone().release())) : nullptr),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbGroupMove(oldOperation.mbGroupMove),
      mbDragOK(oldOperation.mbDragOK) {
    mpVOPExternalVariables = oldOperation.mpVOMInsertObjectsExtVar
        ? oldOperation.mpVOMInsertObjectsExtVar->Clone()
        : new TDVOMInsertObjectsExtVar(this);
    mpVOMInsertObjectsExtVar = dynamic_cast<TDVOMInsertObjectsExtVar*>(mpVOPExternalVariables);
}

TDVOMInsertObjects& TDVOMInsertObjects::operator=(const TDVOMInsertObjects& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = oldOperation.mpVOMInsertObjectsExtVar
        ? oldOperation.mpVOMInsertObjectsExtVar->Clone()
        : new TDVOMInsertObjectsExtVar(this);
    mpVOMInsertObjectsExtVar = dynamic_cast<TDVOMInsertObjectsExtVar*>(mpVOPExternalVariables);
    mpTmpVecGroup = oldOperation.mpTmpVecGroup
        ? std::unique_ptr<TDVecGroup>(dynamic_cast<TDVecGroup*>(oldOperation.mpTmpVecGroup->Clone().release()))
        : nullptr;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbGroupMove = oldOperation.mbGroupMove;
    mbDragOK = oldOperation.mbDragOK;
    return *this;
}

TDVOMInsertObjects* TDVOMInsertObjects::Clone() const {
    return new TDVOMInsertObjects(*this);
}

void TDVOMInsertObjects::Initialize() {
    SetState(OSTATE_STARTED);
}

void TDVOMInsertObjects::ExtVarIsChanged() {
    if (!mpVOMInsertObjectsExtVar || mpVOMInsertObjectsExtVar->objectsForInserting_.empty()) {
        mpTmpObject.reset();
        mpTmpVecGroup.reset();
        mbDragOK = false;
        SetState(OSTATE_NEEDSELECTED);
        return;
    }

    IniInteractivePoints();
    mbDragOK = true;
    if (mpVOMInsertObjectsExtVar->objectsForInserting_.size() == 1) {
        mpTmpObject = mpVOMInsertObjectsExtVar->objectsForInserting_.front()->Clone();
        mpTmpVecGroup.reset();
        XMouseStart = mpTmpObject->GetMidpoint().x;
        YMouseStart = mpTmpObject->GetMidpoint().y;
        XMouseJet = XMouseStart;
        YMouseJet = YMouseStart;
        mbGroupMove = false;
    } else {
        mpTmpVecGroup = mpVOMInsertObjectsExtVar->MakeTempGroupFromObjects();
        mpTmpObject.reset();
        XMouseStart = mpTmpVecGroup->GetMidpoint().x;
        YMouseStart = mpTmpVecGroup->GetMidpoint().y;
        XMouseJet = XMouseStart;
        YMouseJet = YMouseStart;
        mbGroupMove = true;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_DOCK) {
        mpVecEditCad->UseCursor(VECVIEW_DOCK);
    }
    SetState(OSTATE_RUNNING);
}

void __fastcall TDVOMInsertObjects::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        SetState(OSTATE_NEEDSELECTED);
    }
}

void __fastcall TDVOMInsertObjects::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (Button != VIRTMOUSEBTM_LEFT || !mbDragOK || !mpVOMInsertObjectsExtVar) {
        return;
    }

    if (Shift != VKState_KEY_SHIFT) {
        mpVOMInsertObjectsExtVar->MoveObjects(X - XMouseStart, Y - YMouseStart);
    }

    for (const auto& object : mpVOMInsertObjectsExtVar->objectsForInserting_) {
        if (object) {
            mpVecModel->AppendObject(object->Clone().release());
        }
    }

    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    SetState(OSTATE_NEEDSELECTED);
}

void __fastcall TDVOMInsertObjects::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    if (!mbDragOK) {
        return;
    }
    if (!mbGroupMove && mpTmpObject) {
        mpTmpObject->MoveBy(X - XMouseJet, Y - YMouseJet);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
    } else if (mbGroupMove && mpTmpVecGroup) {
        mpTmpVecGroup->MoveBy(X - XMouseJet, Y - YMouseJet);
        mpVecEditCad->TmpAppend(mpTmpVecGroup.get(), true);
    }
    XMouseJet = X;
    YMouseJet = Y;
}

void __fastcall TDVOMInsertObjects::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMInsertObjects::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOMInsertObjectsExtVar::TDVOMInsertObjectsExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      objectsForInserting_() {
}

TDVOMInsertObjectsExtVar::TDVOMInsertObjectsExtVar(const TDVOMInsertObjectsExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      objectsForInserting_() {
    for (const auto& object : oldExtVar.objectsForInserting_) {
        if (object) {
            objectsForInserting_.push_back(object->Clone());
        }
    }
}

TDVOMInsertObjectsExtVar& TDVOMInsertObjectsExtVar::operator=(const TDVOMInsertObjectsExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    objectsForInserting_.clear();
    for (const auto& object : oldExtVar.objectsForInserting_) {
        if (object) {
            objectsForInserting_.push_back(object->Clone());
        }
    }
    return *this;
}

TDVOMInsertObjectsExtVar* TDVOMInsertObjectsExtVar::Clone() const {
    return new TDVOMInsertObjectsExtVar(*this);
}

bool TDVOMInsertObjectsExtVar::AppendObject(TDVecObject* pObject) {
    if (!pObject) {
        return false;
    }
    objectsForInserting_.emplace_back(pObject);
    return true;
}

std::unique_ptr<TDVecGroup> TDVOMInsertObjectsExtVar::MakeTempGroupFromObjects() const {
    if (objectsForInserting_.size() <= 1) {
        return nullptr;
    }
    auto resultGroup = std::make_unique<TDVecGroup>();
    for (const auto& object : objectsForInserting_) {
        if (object) {
            auto cloneObject = object->Clone();
            cloneObject->SetSelect(false);
            resultGroup->AppendInGroup(cloneObject.release());
        }
    }
    resultGroup->FirstInitialize();
    resultGroup->SetSelect(false);
    return resultGroup;
}

void TDVOMInsertObjectsExtVar::MoveObjects(double X, double Y) {
    for (const auto& object : objectsForInserting_) {
        if (object) {
            object->MoveBy(X, Y);
            object->Initialize();
        }
    }
}
