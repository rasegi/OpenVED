//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Move-Object
//---------------------------------------------------------------------------
#define __VOM_MOVE_OBJECT_CPP

#include "vom_move_object.h"

#include "vec_edit_cad.h"

#include <algorithm>

namespace {

void includePoint(TDMatPoint point, double& left, double& top, double& right, double& bottom) {
    left = std::min(left, point.x);
    top = std::min(top, point.y);
    right = std::max(right, point.x);
    bottom = std::max(bottom, point.y);
}

} // namespace

TDVOMMoveObject::TDVOMMoveObject(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mMouseStart{0.0, 0.0},
      mbGroupMove(false),
      mbDragOK(false) {
    if (!InitializeFromSelection()) {
        SetState(OSTATE_NEEDSELECTED);
        return;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_DOCK) {
        mpVecEditCad->UseCursor(VECVIEW_DOCK);
    }
    SetState(OSTATE_RUNNING);
}

TDVOMMoveObject::~TDVOMMoveObject() = default;

TDVOMMoveObject::TDVOMMoveObject(const TDVOMMoveObject& oldOperation)
    : TDVOModify(oldOperation),
      mMouseStart(oldOperation.mMouseStart),
      mbGroupMove(oldOperation.mbGroupMove),
      mbDragOK(oldOperation.mbDragOK) {
    for (const auto& tmpObject : oldOperation.mTmpObjects) {
        if (tmpObject) {
            mTmpObjects.push_back(tmpObject->Clone());
        }
    }
}

TDVOMMoveObject& TDVOMMoveObject::operator=(const TDVOMMoveObject& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    mMouseStart = oldOperation.mMouseStart;
    mbGroupMove = oldOperation.mbGroupMove;
    mbDragOK = oldOperation.mbDragOK;
    mTmpObjects.clear();
    for (const auto& tmpObject : oldOperation.mTmpObjects) {
        if (tmpObject) {
            mTmpObjects.push_back(tmpObject->Clone());
        }
    }
    return *this;
}

TDVOMMoveObject* TDVOMMoveObject::Clone() const {
    return new TDVOMMoveObject(*this);
}

bool TDVOMMoveObject::InitializeFromSelection() {
    const int selectedCount = mpVecModel->CountSelectedObjects();
    if (selectedCount < 1) {
        mbDragOK = false;
        mbGroupMove = false;
        mTmpObjects.clear();
        return false;
    }

    if (selectedCount == 1) {
        TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
        mMouseStart = selectedObject ? selectedObject->GetMidpoint() : TDMatPoint{0.0, 0.0};
    } else {
        mMouseStart = SelectionMidpoint();
    }
    mbGroupMove = selectedCount > 1;
    mbDragOK = true;
    return true;
}

TDMatPoint TDVOMMoveObject::SelectionMidpoint() const {
    const std::vector<TDVecObject*> selectedObjects = mpVecModel->GetSelectedObjects();
    bool hasFrame = false;
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;

    for (const TDVecObject* object : selectedObjects) {
        if (!object) {
            continue;
        }

        const TDMatRect frame = object->GetFrame();
        if (!hasFrame) {
            left = right = frame.P1.x;
            top = bottom = frame.P1.y;
            hasFrame = true;
        }
        includePoint(frame.P1, left, top, right, bottom);
        includePoint(frame.P2, left, top, right, bottom);
        includePoint(frame.P3, left, top, right, bottom);
        includePoint(frame.P4, left, top, right, bottom);
    }

    if (!hasFrame) {
        return {0.0, 0.0};
    }
    return {(left + right) / 2.0, (top + bottom) / 2.0};
}

void TDVOMMoveObject::UpdateMoveShadow(TDMatPoint point) {
    const double dx = point.x - mMouseStart.x;
    const double dy = point.y - mMouseStart.y;
    mTmpObjects.clear();
    mpVecEditCad->TmpClear();
    bool appendedAny = false;

    for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
        if (!object) {
            continue;
        }

        std::unique_ptr<TDVecObject> tmpObject = object->Clone();
        if (!tmpObject || !tmpObject->MoveBy(dx, dy)) {
            continue;
        }

        if (!appendedAny) {
            mpVecEditCad->TmpAppend(tmpObject.get(), true);
            appendedAny = true;
        } else {
            mpVecEditCad->TmpAppendAdditional(tmpObject.get(), true);
        }
        mTmpObjects.push_back(std::move(tmpObject));
    }
}

void TDVOMMoveObject::ClearShadow() {
    mTmpObjects.clear();
    mpVecEditCad->TmpClear();
}

void __fastcall TDVOMMoveObject::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mbDragOK = false;
        ClearShadow();
        mpVecEditCad->ViewsRefresh();
        SetState(OSTATE_NEEDSELECTED);
    }
}

void __fastcall TDVOMMoveObject::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (Button != VIRTMOUSEBTM_LEFT || !mbDragOK) {
        return;
    }

    mpVecModel->MoveSelectedObjects(X - mMouseStart.x, Y - mMouseStart.y);
    mbDragOK = false;
    ClearShadow();
    mpVecEditCad->ViewsRefresh();
    SetState(OSTATE_NEEDSELECTED);
}

void __fastcall TDVOMMoveObject::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;

    if (!mbDragOK) {
        return;
    }

    UpdateMoveShadow({X, Y});
    if (mpVecEditCad->GetUsedCursor() != VECVIEW_DOCK) {
        mpVecEditCad->UseCursor(VECVIEW_DOCK);
    }
}

void __fastcall TDVOMMoveObject::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMMoveObject::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
