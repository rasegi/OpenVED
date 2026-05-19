//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-SelectMove-Object-Scale-Frame
//---------------------------------------------------------------------------
#define __VOM_SELECTMOVE_OBJECT_SCALE_FRAME__CPP

#include "vom_selectmove_object_scale_frame.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <algorithm>
#include <cmath>

TDVOMSelectMoveObjectScaleFrame::TDVOMSelectMoveObjectScaleFrame(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOMSelectMoveObjScaleFrameExtVar(nullptr),
      mbMoveObject(false),
      mbMoveNode(false),
      mbObjectChanged(false),
      mbMustViewsRefresh(false),
      mbDrawLassoOK(false),
      mbGroupMove(false),
      mnNodeMove(-1),
      miObjIndex(-1),
      mbToleranzOK(false),
      mbDialogOpen(true) {
    SetState(OSTATE_RUNNING);
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
    mpVOPExternalVariables = new TDVOMSelectMoveObjScaleFrameExtVar(this);
    mpVOMSelectMoveObjScaleFrameExtVar = dynamic_cast<TDVOMSelectMoveObjScaleFrameExtVar*>(mpVOPExternalVariables);
    mpVecEditCad->SetVisualNodes(false);
    mpVecEditCad->SetVisualFrame(true);
    mpVecEditCad->ViewsRefresh();
}

TDVOMSelectMoveObjectScaleFrame::~TDVOMSelectMoveObjectScaleFrame() {
    IniInteractivePoints();
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
}

TDVOMSelectMoveObjectScaleFrame::TDVOMSelectMoveObjectScaleFrame(const TDVOMSelectMoveObjectScaleFrame& oldOperation)
    : TDVOModify(oldOperation),
      mpVOMSelectMoveObjScaleFrameExtVar(dynamic_cast<TDVOMSelectMoveObjScaleFrameExtVar*>(mpVOPExternalVariables)),
      mbMoveObject(oldOperation.mbMoveObject),
      mbMoveNode(oldOperation.mbMoveNode),
      mbObjectChanged(oldOperation.mbObjectChanged),
      mbMustViewsRefresh(oldOperation.mbMustViewsRefresh),
      mbDrawLassoOK(oldOperation.mbDrawLassoOK),
      mbGroupMove(oldOperation.mbGroupMove),
      mnNodeMove(oldOperation.mnNodeMove),
      miObjIndex(oldOperation.miObjIndex),
      mbToleranzOK(oldOperation.mbToleranzOK),
      mbDialogOpen(oldOperation.mbDialogOpen) {
    for (const auto& object : oldOperation.mTmpObjects) {
        if (object) {
            mTmpObjects.push_back(object->Clone());
        }
    }
}

TDVOMSelectMoveObjectScaleFrame& TDVOMSelectMoveObjectScaleFrame::operator=(const TDVOMSelectMoveObjectScaleFrame& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    mpVOMSelectMoveObjScaleFrameExtVar = dynamic_cast<TDVOMSelectMoveObjScaleFrameExtVar*>(mpVOPExternalVariables);
    mbMoveObject = oldOperation.mbMoveObject;
    mbMoveNode = oldOperation.mbMoveNode;
    mbObjectChanged = oldOperation.mbObjectChanged;
    mbMustViewsRefresh = oldOperation.mbMustViewsRefresh;
    mbDrawLassoOK = oldOperation.mbDrawLassoOK;
    mbGroupMove = oldOperation.mbGroupMove;
    mnNodeMove = oldOperation.mnNodeMove;
    miObjIndex = oldOperation.miObjIndex;
    mbToleranzOK = oldOperation.mbToleranzOK;
    mbDialogOpen = oldOperation.mbDialogOpen;
    mTmpObjects.clear();
    for (const auto& object : oldOperation.mTmpObjects) {
        if (object) {
            mTmpObjects.push_back(object->Clone());
        }
    }
    return *this;
}

TDVOMSelectMoveObjectScaleFrame* TDVOMSelectMoveObjectScaleFrame::Clone() const {
    return new TDVOMSelectMoveObjectScaleFrame(*this);
}

double TDVOMSelectMoveObjectScaleFrame::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return 3.0;
    }
    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

bool TDVOMSelectMoveObjectScaleFrame::HasMovedPastTolerance(TDMatPoint point) const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return std::fabs(point.x - XMouseStart) > MouseToleranceReal() ||
               std::fabs(point.y - YMouseStart) > MouseToleranceReal();
    }

    const long xDelta = std::labs(graphicEngine->RealToXVal(point.x - XMouseStart));
    const long yDelta = std::labs(graphicEngine->RealToYVal(point.y - YMouseStart));
    const long tolerance = graphicEngine->GetMouseTolerance() + 2;
    return xDelta >= tolerance || yDelta >= tolerance;
}

long TDVOMSelectMoveObjectScaleFrame::FindObjectFrame(TDMatPoint point) const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    const double tolerance = MouseToleranceReal();
    long selectedElement = -1;
    for (int index = 0; index < mpVecModel->CountObjects(); ++index) {
        TDVecObject* object = mpVecModel->GetObject(index);
        if (!object) {
            continue;
        }

        if (graphicEngine) {
            if (object->HitTest(graphicEngine, point, graphicEngine->GetMouseTolerance()).IsHit()) {
                selectedElement = index;
            } else if (object->GetSelect() && object->PointOnScaleFrameNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance()) > -1) {
                selectedElement = index;
            }
        } else {
            if (object->HitTest(point, tolerance).IsHit()) {
                selectedElement = index;
            } else if (object->GetSelect() && object->PointOnScaleFrameNode(point, tolerance) > -1) {
                selectedElement = index;
            }
        }
    }
    return selectedElement;
}

long TDVOMSelectMoveObjectScaleFrame::PointOnSelectedFrameNode(TDVecObject* object, TDMatPoint point) const {
    if (!object) {
        return -1;
    }
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (graphicEngine) {
        return object->PointOnScaleFrameNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
    }
    return object->PointOnScaleFrameNode(point, MouseToleranceReal());
}

void TDVOMSelectMoveObjectScaleFrame::ResetInteractiveState() {
    mTmpObjects.clear();
    mbMoveObject = false;
    mbMoveNode = false;
    mbObjectChanged = false;
    mbMustViewsRefresh = false;
    mbDrawLassoOK = false;
    mbGroupMove = false;
    mnNodeMove = -1;
    miObjIndex = -1;
    mbToleranzOK = false;
}

void TDVOMSelectMoveObjectScaleFrame::FinishInteraction() {
    mbDialogOpen = true;
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
    mpVecEditCad->TmpClear();
    mTmpObjects.clear();
    mpVecEditCad->ViewsRefresh();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void TDVOMSelectMoveObjectScaleFrame::UpdateExternalVariables(TDVecObject* object) {
    if (!mpVOMSelectMoveObjScaleFrameExtVar || !object) {
        return;
    }

    const TDMatRect frame = object->GetScaleFrame();
    mpVOMSelectMoveObjScaleFrameExtVar->SetFrame(frame);
    mpVOMSelectMoveObjScaleFrameExtVar->SetHeight(MatDistance2Point(frame.P1, frame.P2));
    mpVOMSelectMoveObjScaleFrameExtVar->SetWidth(MatDistance2Point(frame.P2, frame.P3));
    mpVOMSelectMoveObjScaleFrameExtVar->SetObjectsCount(1);
    mpVOMSelectMoveObjScaleFrameExtVar->SendUpdateToOPManager();
}

void TDVOMSelectMoveObjectScaleFrame::UpdateExternalVariablesForSelection() {
    if (!mpVOMSelectMoveObjScaleFrameExtVar) {
        return;
    }

    const long objectsCount = mpVecModel->CountSelectedObjects();
    if (objectsCount == 1) {
        UpdateExternalVariables(mpVecModel->GetSelectedObject());
    } else {
        mpVOMSelectMoveObjScaleFrameExtVar->SetObjectsCount(objectsCount);
        mpVOMSelectMoveObjScaleFrameExtVar->SendUpdateToOPManager();
    }
}

void TDVOMSelectMoveObjectScaleFrame::AppendTmpObject(TDVecObject* object, bool first) {
    if (first) {
        mpVecEditCad->TmpAppend(object, true);
    } else {
        mpVecEditCad->TmpAppendAdditional(object, true);
    }
}

void __fastcall TDVOMSelectMoveObjectScaleFrame::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    const TDMatPoint point{X, Y};

    if (Button == VIRTMOUSEBTM_RIGHT) {
        if (mbDialogOpen) {
            Send_OpenDialogMessage_ToManager();
            ResetInteractiveState();
            FinishInteraction();
        }
        return;
    }

    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbDialogOpen = true;
        SetState(OSTATE_RUNNING);
        IniInteractivePoints();
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        break;
    case OSTATE_UNKNOWN:
    case OSTATE_RUNNING:
    case OSTATE_ABORTED:
    case OSTATE_NEEDSELECTED:
        break;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    ResetInteractiveState();
    mpVecEditCad->TmpClear();

    const long objectIndex = FindObjectFrame(point);
    if ((Shift & VKState_KEY_SHIFT) == 0) {
        if (objectIndex != -1) {
            if (mpVecModel->CountSelectedObjects() > 1 && mpVecModel->IsObjectSelected(static_cast<int>(objectIndex))) {
                XMouseStart = X;
                YMouseStart = Y;
                XMouseJet = X;
                YMouseJet = Y;
                mbGroupMove = true;
                miObjIndex = objectIndex;
            } else {
                mbMustViewsRefresh = mpVecModel->SelectOnlyObject(static_cast<int>(objectIndex));
            }
        } else {
            mpVecModel->DeselectAll();
            mbMustViewsRefresh = true;
            XMouseStart = X;
            YMouseStart = Y;
            XMouseJet = X;
            YMouseJet = Y;
            mbDrawLassoOK = true;
        }
    } else {
        if (objectIndex != -1) {
            TDVecObject* object = mpVecModel->GetObject(static_cast<int>(objectIndex));
            if (object) {
                object->SetSelect(!object->GetSelect());
                mbMustViewsRefresh = true;
            }
        } else {
            XMouseStart = X;
            YMouseStart = Y;
            XMouseJet = X;
            YMouseJet = Y;
            mbDrawLassoOK = true;
        }
    }

    UpdateExternalVariablesForSelection();

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!mbGroupMove && selectedObject && mpVecModel->GetSelectedObjectIndex() != -1 && (Shift & VKState_KEY_SHIFT) == 0) {
        const bool clickObject = selectedObject->HitTest(point, MouseToleranceReal()).IsHit();
        const long clickNode = PointOnSelectedFrameNode(selectedObject, point);

        if (clickObject) {
            XMouseStart = X;
            YMouseStart = Y;
            XMouseJet = X;
            YMouseJet = Y;
            mbMoveObject = true;
            if (clickNode != -1) {
                mbMoveNode = true;
                mbMoveObject = false;
            }
        } else {
            mbMoveObject = false;
            if (clickNode != -1) {
                XMouseStart = X;
                YMouseStart = Y;
                XMouseJet = X;
                YMouseJet = Y;
                mbMoveNode = true;
            }
        }
        mnNodeMove = clickNode;
    }

    if (mbMustViewsRefresh) {
        mpVecEditCad->ViewsRefresh();
    }
}

void __fastcall TDVOMSelectMoveObjectScaleFrame::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (mpVecEditCad->GetUsedCursor() == VECVIEW_NO) {
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        ResetInteractiveState();
        FinishInteraction();
        return;
    }

    const TDMatPoint point{X, Y};
    if (mbMoveNode && !mbMoveObject && mbObjectChanged) {
        TDVecObject* object = mpVecModel->GetSelectedObject();
        if (object && mnNodeMove != -1) {
            if (object->MoveScaleFrameNode(mnNodeMove, X - XMouseStart, Y - YMouseStart, point)) {
                object->Initialize();
                mpVecModel->MarkChanged();
            }
        }
        FinishInteraction();
        return;
    }

    if (!mbMoveNode && mbMoveObject && mbObjectChanged) {
        TDVecObject* object = mpVecModel->GetSelectedObject();
        if (object) {
            if (object->MoveBy(X - XMouseStart, Y - YMouseStart)) {
                object->Initialize();
                mpVecModel->MarkChanged();
            }
        }
        FinishInteraction();
        return;
    }

    if (mbGroupMove) {
        if (mbObjectChanged) {
            mpVecModel->MoveSelectedObjects(X - XMouseStart, Y - YMouseStart);
        } else {
            mpVecModel->SelectOnlyObject(static_cast<int>(miObjIndex));
        }
        FinishInteraction();
        return;
    }

    if (!mbMoveNode && !mbMoveObject && !mbObjectChanged) {
        if (mbDrawLassoOK) {
            mpVecModel->SelectObjectsInArea({XMouseStart, YMouseStart}, {XMouseJet, YMouseJet}, (Shift & VKState_KEY_SHIFT) != 0);
        }
        FinishInteraction();
        return;
    }

    if (mbMustViewsRefresh) {
        mpVecEditCad->ViewsRefresh();
    }
}

void __fastcall TDVOMSelectMoveObjectScaleFrame::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    const TDMatPoint point{X, Y};
    if (mbMoveNode || mbMoveObject || mbGroupMove) {
        if (!mbToleranzOK) {
            mbToleranzOK = HasMovedPastTolerance(point);
        }
    }

    if (mbMoveNode && !mbMoveObject) {
        TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
        if (!selectedObject) {
            return;
        }
        if (selectedObject->GetLockResize()) {
            mpVecEditCad->UseCursor(VECVIEW_NO);
            return;
        }

        auto clonedObject = selectedObject->Clone();
        clonedObject->MoveScaleFrameNode(mnNodeMove, X - XMouseStart, Y - YMouseStart, point);
        XMouseJet = X;
        YMouseJet = Y;
        if (mbToleranzOK) {
            mpVecEditCad->UseCursor(VECVIEW_DOCK);
            mTmpObjects.clear();
            mTmpObjects.push_back(std::move(clonedObject));
            mpVecEditCad->TmpAppend(mTmpObjects.back().get(), true);
            mbObjectChanged = true;
            mbDialogOpen = false;
            UpdateExternalVariables(mTmpObjects.back().get());
        }
        return;
    }

    if (!mbMoveNode && mbMoveObject) {
        TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
        if (!selectedObject) {
            return;
        }
        if (selectedObject->GetLockPosition()) {
            mpVecEditCad->UseCursor(VECVIEW_NO);
            return;
        }

        auto clonedObject = selectedObject->Clone();
        clonedObject->MoveBy(X - XMouseStart, Y - YMouseStart);
        XMouseJet = X;
        YMouseJet = Y;
        if (mbToleranzOK) {
            mpVecEditCad->UseCursor(VECVIEW_DOCK);
            mTmpObjects.clear();
            mTmpObjects.push_back(std::move(clonedObject));
            mpVecEditCad->TmpAppend(mTmpObjects.back().get(), true);
            mbObjectChanged = true;
            mbDialogOpen = false;
            UpdateExternalVariables(mTmpObjects.back().get());
        }
        return;
    }

    if (mbGroupMove) {
        if ((X - XMouseJet) != 0.0 || (Y - YMouseJet) != 0.0) {
            mTmpObjects.clear();
            bool first = true;
            for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
                if (!object) {
                    continue;
                }
                auto clone = object->Clone();
                clone->MoveBy(X - XMouseStart, Y - YMouseStart);
                AppendTmpObject(clone.get(), first);
                first = false;
                mTmpObjects.push_back(std::move(clone));
            }
            XMouseJet = X;
            YMouseJet = Y;
            if (mbToleranzOK && !mTmpObjects.empty()) {
                mpVecEditCad->UseCursor(VECVIEW_DOCK);
                mbObjectChanged = true;
                mbDialogOpen = false;
            }
        }
        return;
    }

    if (!mbMoveNode && !mbMoveObject && mbDrawLassoOK) {
        mpVecEditCad->UseCursor(VECVIEW_PLUS);
        XMouseJet = X;
        YMouseJet = Y;
        mbObjectChanged = false;
        mbDialogOpen = false;
    }
}

void __fastcall TDVOMSelectMoveObjectScaleFrame::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMSelectMoveObjectScaleFrame::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOMSelectMoveObjScaleFrameExtVar::TDVOMSelectMoveObjScaleFrameExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mFrame{},
      mnHeight(0.0),
      mnWidth(0.0),
      mnObjectsCount(0) {
}

TDVOMSelectMoveObjScaleFrameExtVar::TDVOMSelectMoveObjScaleFrameExtVar(const TDVOMSelectMoveObjScaleFrameExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mFrame(oldExtVar.mFrame),
      mnHeight(oldExtVar.mnHeight),
      mnWidth(oldExtVar.mnWidth),
      mnObjectsCount(oldExtVar.mnObjectsCount) {
}

TDVOMSelectMoveObjScaleFrameExtVar& TDVOMSelectMoveObjScaleFrameExtVar::operator=(const TDVOMSelectMoveObjScaleFrameExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mFrame = oldExtVar.mFrame;
    mnHeight = oldExtVar.mnHeight;
    mnWidth = oldExtVar.mnWidth;
    mnObjectsCount = oldExtVar.mnObjectsCount;
    return *this;
}

TDVOMSelectMoveObjScaleFrameExtVar* TDVOMSelectMoveObjScaleFrameExtVar::Clone() const {
    return new TDVOMSelectMoveObjScaleFrameExtVar(*this);
}

void TDVOMSelectMoveObjScaleFrameExtVar::SetFrame(TDMatRect Frame) {
    mFrame = Frame;
}

TDMatRect TDVOMSelectMoveObjScaleFrameExtVar::GetFrame() const {
    return mFrame;
}

void TDVOMSelectMoveObjScaleFrameExtVar::SetHeight(double nHeight) {
    mnHeight = nHeight;
}

double TDVOMSelectMoveObjScaleFrameExtVar::GetHeight() const {
    return mnHeight;
}

void TDVOMSelectMoveObjScaleFrameExtVar::SetWidth(double nWidth) {
    mnWidth = nWidth;
}

double TDVOMSelectMoveObjScaleFrameExtVar::GetWidth() const {
    return mnWidth;
}

void TDVOMSelectMoveObjScaleFrameExtVar::SetObjectsCount(long nObjectsCount) {
    mnObjectsCount = nObjectsCount;
}

long TDVOMSelectMoveObjScaleFrameExtVar::GetObjectsCount() const {
    return mnObjectsCount;
}
