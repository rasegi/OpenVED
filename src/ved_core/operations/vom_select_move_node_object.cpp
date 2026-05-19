//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Select-Move-Node-Object
//---------------------------------------------------------------------------
#define __VOM_SELECT_MOVE_NODE_OBJECT_CPP

#include "vom_select_move_node_object.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <algorithm>
#include <cmath>

TDVOMSelectMoveNodeObject::TDVOMSelectMoveNodeObject(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      miObjectIndex(-1),
      mpObject(nullptr),
      mnNodeMove(-1),
      mMouseStart{0.0, 0.0},
      mMouseCurrent{0.0, 0.0},
      mbMoveObject(false),
      mbMoveNode(false),
      mbGroupMove(false),
      mbObjectChanged(false),
      mbToleranceOK(false),
      mbAreaSelect(false),
      mbAreaMoved(false) {
    SetState(OSTATE_RUNNING);
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
}

TDVOMSelectMoveNodeObject::~TDVOMSelectMoveNodeObject() = default;

TDVOMSelectMoveNodeObject::TDVOMSelectMoveNodeObject(const TDVOMSelectMoveNodeObject& oldOperation)
    : TDVOModify(oldOperation),
      miObjectIndex(oldOperation.miObjectIndex),
      mpObject(oldOperation.mpObject),
      mnNodeMove(oldOperation.mnNodeMove),
      mMouseStart(oldOperation.mMouseStart),
      mMouseCurrent(oldOperation.mMouseCurrent),
      mbMoveObject(oldOperation.mbMoveObject),
      mbMoveNode(oldOperation.mbMoveNode),
      mbGroupMove(oldOperation.mbGroupMove),
      mbObjectChanged(oldOperation.mbObjectChanged),
      mbToleranceOK(oldOperation.mbToleranceOK),
      mbAreaSelect(oldOperation.mbAreaSelect),
      mbAreaMoved(oldOperation.mbAreaMoved) {
    if (oldOperation.mpTmpObject) {
        mpTmpObject = oldOperation.mpTmpObject->Clone();
    }
    for (const auto& object : oldOperation.mTmpGroupObjects) {
        if (object) {
            mTmpGroupObjects.push_back(object->Clone());
        }
    }
}

TDVOMSelectMoveNodeObject& TDVOMSelectMoveNodeObject::operator=(const TDVOMSelectMoveNodeObject& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    miObjectIndex = oldOperation.miObjectIndex;
    mpObject = oldOperation.mpObject;
    mnNodeMove = oldOperation.mnNodeMove;
    mMouseStart = oldOperation.mMouseStart;
    mMouseCurrent = oldOperation.mMouseCurrent;
    mbMoveObject = oldOperation.mbMoveObject;
    mbMoveNode = oldOperation.mbMoveNode;
    mbGroupMove = oldOperation.mbGroupMove;
    mbObjectChanged = oldOperation.mbObjectChanged;
    mbToleranceOK = oldOperation.mbToleranceOK;
    mbAreaSelect = oldOperation.mbAreaSelect;
    mbAreaMoved = oldOperation.mbAreaMoved;
    if (oldOperation.mpTmpObject) {
        mpTmpObject = oldOperation.mpTmpObject->Clone();
    } else {
        mpTmpObject.reset();
    }
    mTmpGroupObjects.clear();
    for (const auto& object : oldOperation.mTmpGroupObjects) {
        if (object) {
            mTmpGroupObjects.push_back(object->Clone());
        }
    }
    return *this;
}

TDVOMSelectMoveNodeObject* TDVOMSelectMoveNodeObject::Clone() const {
    return new TDVOMSelectMoveNodeObject(*this);
}

TDVecModelHitResult TDVOMSelectMoveNodeObject::FindHit(TDMatPoint point) const {
    const TDVecModelHitResult selectedNode = FindSelectedNode(point);
    if (selectedNode.IsHit()) {
        return selectedNode;
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (graphicEngine) {
        return mpVecModel->FindObjectAt(graphicEngine, point, graphicEngine->GetMouseTolerance());
    }
    return mpVecModel->FindObjectAt(point, MouseToleranceReal());
}

TDVecModelHitResult TDVOMSelectMoveNodeObject::FindSelectedNode(TDMatPoint point) const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    const double realTolerance = MouseToleranceReal();

    for (int index = mpVecModel->CountObjects() - 1; index >= 0; --index) {
        TDVecObject* object = mpVecModel->GetObject(index);
        if (!object || !object->GetSelect()) {
            continue;
        }

        long node = -1;
        if (graphicEngine) {
            node = object->PointOnNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
        } else {
            node = object->PointOnNode(point, realTolerance);
        }
        if (node > -1) {
            return {index, object, {TDVecLineHitKind::StartNode, 0.0}};
        }
    }

    return {};
}

double TDVOMSelectMoveNodeObject::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return 0.0;
    }

    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

bool TDVOMSelectMoveNodeObject::HasMovedPastTolerance(TDMatPoint point) const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return std::fabs(point.x - mMouseStart.x) > 0.0 || std::fabs(point.y - mMouseStart.y) > 0.0;
    }

    const long xDelta = std::labs(graphicEngine->RealToXVal(point.x - mMouseStart.x));
    const long yDelta = std::labs(graphicEngine->RealToYVal(point.y - mMouseStart.y));
    const long tolerance = graphicEngine->GetMouseTolerance() + 2;
    return xDelta >= tolerance || yDelta >= tolerance;
}

void TDVOMSelectMoveNodeObject::ResetInteractiveState() {
    miObjectIndex = -1;
    mpObject = nullptr;
    mpTmpObject.reset();
    mTmpGroupObjects.clear();
    mnNodeMove = -1;
    mbMoveObject = false;
    mbMoveNode = false;
    mbGroupMove = false;
    mbObjectChanged = false;
    mbToleranceOK = false;
    mbAreaSelect = false;
    mbAreaMoved = false;
}

void TDVOMSelectMoveNodeObject::UpdateMoveShadow(TDMatPoint point) {
    if (!mpObject) {
        return;
    }

    mpTmpObject = mpObject->Clone();
    mpTmpObject->MoveBy(point.x - mMouseStart.x, point.y - mMouseStart.y);
    mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
    mpVecEditCad->UseCursor(VECVIEW_DOCK);
    mbObjectChanged = true;
}

void TDVOMSelectMoveNodeObject::UpdateNodeShadow(TDMatPoint point) {
    if (!mpObject || mnNodeMove == -1) {
        return;
    }

    mpTmpObject = mpObject->Clone();
    if (!mpTmpObject->MoveNode(mnNodeMove, point.x - mMouseStart.x, point.y - mMouseStart.y, point)) {
        return;
    }
    mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
    mpVecEditCad->UseCursor(VECVIEW_DOCK);
    mbObjectChanged = true;
}

void TDVOMSelectMoveNodeObject::UpdateGroupShadow(TDMatPoint point) {
    const double dx = point.x - mMouseStart.x;
    const double dy = point.y - mMouseStart.y;
    mTmpGroupObjects.clear();
    mpVecEditCad->TmpClear();

    for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
        if (!object) {
            continue;
        }

        auto tmpObject = object->Clone();
        tmpObject->MoveBy(dx, dy);
        if (mTmpGroupObjects.empty()) {
            mpVecEditCad->TmpAppend(tmpObject.get(), true);
        } else {
            mpVecEditCad->TmpAppendAdditional(tmpObject.get(), true);
        }
        mTmpGroupObjects.push_back(std::move(tmpObject));
    }

    if (!mTmpGroupObjects.empty()) {
        mpVecEditCad->UseCursor(VECVIEW_DOCK);
        mbObjectChanged = true;
    }
}

void __fastcall TDVOMSelectMoveNodeObject::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_UNKNOWN:
    case OSTATE_ABORTED:
        return;
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        break;
    case OSTATE_RUNNING:
    case OSTATE_NEEDSELECTED:
        break;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    ResetInteractiveState();
    mpVecEditCad->TmpClear();

    const TDMatPoint point{X, Y};
    const TDVecModelHitResult hit = FindHit(point);
    mMouseStart = point;
    mMouseCurrent = point;

    if (!hit.IsHit()) {
        mbAreaSelect = true;
        return;
    }

    miObjectIndex = hit.objectIndex;
    mpObject = hit.object;
    if ((Shift & VKState_KEY_SHIFT) != 0) {
        mpVecModel->SelectObject(miObjectIndex);
        mpVecEditCad->ViewsRefresh();
        return;
    }

    if (mpVecModel->CountSelectedObjects() > 1) {
        if (mpVecModel->IsObjectSelected(miObjectIndex)) {
            mbGroupMove = true;
            return;
        }

        mpVecModel->SelectOnlyObject(miObjectIndex);
        mpVecEditCad->ViewsRefresh();
    } else {
        mpVecModel->SelectOnlyObject(miObjectIndex);
        mpVecEditCad->ViewsRefresh();
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (graphicEngine) {
        mnNodeMove = mpObject->PointOnNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
    } else {
        mnNodeMove = mpObject->PointOnNode(point, MouseToleranceReal());
    }
    mbMoveNode = mnNodeMove != -1;
    mbMoveObject = !mbMoveNode;
}

void __fastcall TDVOMSelectMoveNodeObject::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (Button != VIRTMOUSEBTM_LEFT) {
        ResetInteractiveState();
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        return;
    }

    const TDMatPoint point{X, Y};
    if (mbGroupMove && mbObjectChanged) {
        mpVecModel->MoveSelectedObjects(point.x - mMouseStart.x, point.y - mMouseStart.y);
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        ResetInteractiveState();
        bool destroyed = false;
        SetState(OSTATE_FINISHED, &destroyed);
        return;
    }

    if (mbMoveNode && mbObjectChanged) {
        if (mpObject && mnNodeMove != -1) {
            if (mpObject->MoveNode(mnNodeMove, point.x - mMouseStart.x, point.y - mMouseStart.y, point)) {
                mpVecModel->MarkChanged();
            }
        }
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        ResetInteractiveState();
        bool destroyed = false;
        SetState(OSTATE_FINISHED, &destroyed);
        return;
    }

    if (mbMoveObject && mbObjectChanged) {
        if (mpObject) {
            if (mpObject->MoveBy(point.x - mMouseStart.x, point.y - mMouseStart.y)) {
                mpVecModel->MarkChanged();
            }
        }
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        ResetInteractiveState();
        bool destroyed = false;
        SetState(OSTATE_FINISHED, &destroyed);
        return;
    }

    if (mbAreaSelect) {
        mMouseCurrent = point;
        if (mbAreaMoved) {
            mpVecModel->SelectObjectsInArea(mMouseStart, mMouseCurrent, (Shift & VKState_KEY_SHIFT) != 0);
        } else {
            mpVecModel->DeselectAll();
        }
        mpVecEditCad->ViewsRefresh();
    }

    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
    mpVecEditCad->TmpClear();
    ResetInteractiveState();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOMSelectMoveNodeObject::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    const TDMatPoint point{X, Y};
    mMouseCurrent = point;

    if (mbGroupMove || mbMoveNode || mbMoveObject) {
        if (!mbToleranceOK) {
            mbToleranceOK = HasMovedPastTolerance(point);
        }
        if (mbToleranceOK) {
            if (mbGroupMove) {
                UpdateGroupShadow(point);
            } else if (mbMoveNode) {
                UpdateNodeShadow(point);
            } else {
                UpdateMoveShadow(point);
            }
        }
        return;
    }

    if (mbAreaSelect) {
        const double tolerance = MouseToleranceReal();
        if (std::fabs(point.x - mMouseStart.x) > tolerance ||
            std::fabs(point.y - mMouseStart.y) > tolerance) {
            mbAreaMoved = true;
        }
    }
}

void __fastcall TDVOMSelectMoveNodeObject::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMSelectMoveNodeObject::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
