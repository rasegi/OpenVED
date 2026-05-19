//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Delete-Node
//---------------------------------------------------------------------------
#define __VOM_DELETE_NODE_CPP

#include "vom_delete_node.h"

#include "vec_bspline.h"
#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon.h"
#include "vec_polygon_area.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"

#include <algorithm>

TDVOMDeleteNode::TDVOMDeleteNode(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      meObjectType(VECOBJ_UNKNOWN),
      mbDeleteOK(false),
      mbObjectTypeOK(false),
      bOnObject(false),
      miNode(-1) {
    if (SelectedObjectTypeOK()) {
        SetState(OSTATE_STARTED);
        mpVecEditCad->UseCursor(VECVIEW_CROSS_MINUS);
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }
}

TDVOMDeleteNode::~TDVOMDeleteNode() {
    IniInteractivePoints();
    Finished();
}

TDVOMDeleteNode::TDVOMDeleteNode(const TDVOMDeleteNode& oldOperation)
    : TDVOModify(oldOperation),
      meObjectType(oldOperation.meObjectType),
      mbDeleteOK(oldOperation.mbDeleteOK),
      mbObjectTypeOK(oldOperation.mbObjectTypeOK),
      bOnObject(oldOperation.bOnObject),
      miNode(oldOperation.miNode) {
}

TDVOMDeleteNode& TDVOMDeleteNode::operator=(const TDVOMDeleteNode& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    meObjectType = oldOperation.meObjectType;
    mbDeleteOK = oldOperation.mbDeleteOK;
    mbObjectTypeOK = oldOperation.mbObjectTypeOK;
    bOnObject = oldOperation.bOnObject;
    miNode = oldOperation.miNode;
    return *this;
}

TDVOMDeleteNode* TDVOMDeleteNode::Clone() const {
    return new TDVOMDeleteNode(*this);
}

void TDVOMDeleteNode::Finished() {
    meObjectType = VECOBJ_UNKNOWN;
    mbDeleteOK = false;
    mbObjectTypeOK = false;
    bOnObject = false;
    miNode = -1;
}

bool TDVOMDeleteNode::SelectedObjectTypeOK() const {
    TDVecObject* object = mpVecModel ? mpVecModel->GetSelectedObject() : nullptr;
    if (!object) {
        return false;
    }
    switch (object->GetType()) {
    case VECOBJ_POL:
    case VECOBJ_POLAREA:
    case VECOBJ_BSPLINE:
    case VECOBJ_POLYCURVE:
    case VECOBJ_POLYCURVEAREA:
        return true;
    default:
        return false;
    }
}

double TDVOMDeleteNode::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad ? mpVecEditCad->GetActiveGraphicEngine() : nullptr;
    if (!graphicEngine) {
        return 1.0;
    }
    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

long TDVOMDeleteNode::PointOnDeletableNode(TDVecObject* object, TDMatPoint point) const {
    if (!object) {
        return -1;
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(object)) {
        return graphicEngine
            ? bspline->PointOnControle(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance())
            : object->PointOnNode(point, MouseToleranceReal());
    }
    return graphicEngine
        ? object->PointOnNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance())
        : object->PointOnNode(point, MouseToleranceReal());
}

bool TDVOMDeleteNode::DeleteSelectedNode() {
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || miNode == -1) {
        return false;
    }

    if (auto* polygon = dynamic_cast<TDVecPolygon*>(selectedObject)) {
        return polygon->CountPoints() > 2 && polygon->RemovePoint(static_cast<int>(miNode));
    }
    if (auto* polygonArea = dynamic_cast<TDVecPolygonArea*>(selectedObject)) {
        return polygonArea->CountPoints() > 3 && polygonArea->RemovePoint(static_cast<int>(miNode));
    }
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(selectedObject)) {
        if (bspline->CountPoints() > static_cast<int>(bspline->GetDegree() + 1) && bspline->CountPoints() > 3) {
            const bool removed = bspline->RemovePoint(static_cast<int>(miNode));
            bspline->GenerateKnot();
            bspline->ComputeCurve();
            return removed;
        }
        return false;
    }
    if (auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(selectedObject)) {
        return polyCurve->RemovePoint(static_cast<int>(miNode));
    }
    if (auto* polyCurveArea = dynamic_cast<TDVecPolyCurveArea*>(selectedObject)) {
        return polyCurveArea->RemovePoint(static_cast<int>(miNode));
    }
    return false;
}

void __fastcall TDVOMDeleteNode::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        miNode = -1;
        mbDeleteOK = false;
        mbObjectTypeOK = false;
        bOnObject = false;
        SetState(OSTATE_RUNNING);
        break;
    default:
        break;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        Finished();
        SetState(OSTATE_ABORTED);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!selectedObject || !SelectedObjectTypeOK()) {
        mbObjectTypeOK = false;
        mbDeleteOK = false;
        bOnObject = false;
        SetState(OSTATE_ABORTED);
        return;
    }

    meObjectType = selectedObject->GetType();
    mbObjectTypeOK = true;
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    const TDMatPoint point{X, Y};
    bOnObject = graphicEngine
        ? selectedObject->HitTest(graphicEngine, point, graphicEngine->GetMouseTolerance()).IsHit()
        : selectedObject->HitTest(point, MouseToleranceReal()).IsHit();
    miNode = PointOnDeletableNode(selectedObject, point);

    if (miNode != -1) {
        mbDeleteOK = true;
        SetState(OSTATE_RUNNING);
        XMouseStart = X;
        YMouseStart = Y;
        XMouseJet = X;
        YMouseJet = Y;
    } else if (bOnObject) {
        mbDeleteOK = false;
        SetState(OSTATE_STARTED);
    } else {
        mbDeleteOK = false;
        SetState(OSTATE_ABORTED);
    }
}

void __fastcall TDVOMDeleteNode::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;
    if (Button != VIRTMOUSEBTM_LEFT || !mpVecModel->GetSelectedObject() || !mbDeleteOK) {
        return;
    }

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!DeleteSelectedNode()) {
        mpVecEditCad->Beep();
    } else {
        mpVecModel->MarkChanged();
    }
    selectedObject->Initialize();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();
    Finished();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
}

void __fastcall TDVOMDeleteNode::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMDeleteNode::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMDeleteNode::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
