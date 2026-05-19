//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Insert-Node
//---------------------------------------------------------------------------
#define __VOM_INSERT_NODE_CPP

#include "vom_insert_node.h"

#include "vec_bspline.h"
#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon.h"
#include "vec_polygon_area.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
double distanceToSegment(TDMatPoint point, TDMatPoint start, TDMatPoint end) {
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    if (dx == 0.0 && dy == 0.0) {
        return MatDistance2Point(point, start);
    }
    const double t = std::max(0.0, std::min(1.0, ((point.x - start.x) * dx + (point.y - start.y) * dy) / (dx * dx + dy * dy)));
    return MatDistance2Point(point, {start.x + t * dx, start.y + t * dy});
}

template <typename TPointGetter>
long pointAfterFromPoints(int count, TPointGetter getPoint, TDMatPoint point, double tolerance) {
    if (count < 2) {
        return -1;
    }
    double best = std::numeric_limits<double>::infinity();
    long bestIndex = -1;
    for (int i = 1; i < count; ++i) {
        const TDMatPoint* previous = getPoint(i - 1);
        const TDMatPoint* current = getPoint(i);
        if (!previous || !current) {
            continue;
        }
        const double d = distanceToSegment(point, *previous, *current);
        if (d <= tolerance && d < best) {
            best = d;
            bestIndex = i - 1;
        }
    }
    return bestIndex;
}
}

TDVOMInsertNode::TDVOMInsertNode(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      meObjectType(VECOBJ_UNKNOWN),
      mpCloneObject(nullptr),
      mpTmpObject(nullptr),
      mbInsertOK(false),
      mbObjectTypeOK(false),
      bOnObject(false),
      miNode(-1),
      miVorNode(-1) {
    if (SelectedObjectTypeOK()) {
        SetState(OSTATE_STARTED);
        mpVecEditCad->UseCursor(VECVIEW_CROSS_PLUS);
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }
}

TDVOMInsertNode::~TDVOMInsertNode() {
    IniInteractivePoints();
    Finished();
}

TDVOMInsertNode::TDVOMInsertNode(const TDVOMInsertNode& oldOperation)
    : TDVOModify(oldOperation),
      meObjectType(oldOperation.meObjectType),
      mpCloneObject(oldOperation.mpCloneObject ? oldOperation.mpCloneObject->Clone() : nullptr),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbInsertOK(oldOperation.mbInsertOK),
      mbObjectTypeOK(oldOperation.mbObjectTypeOK),
      bOnObject(oldOperation.bOnObject),
      miNode(oldOperation.miNode),
      miVorNode(oldOperation.miVorNode) {
}

TDVOMInsertNode& TDVOMInsertNode::operator=(const TDVOMInsertNode& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    meObjectType = oldOperation.meObjectType;
    mpCloneObject = oldOperation.mpCloneObject ? oldOperation.mpCloneObject->Clone() : nullptr;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbInsertOK = oldOperation.mbInsertOK;
    mbObjectTypeOK = oldOperation.mbObjectTypeOK;
    bOnObject = oldOperation.bOnObject;
    miNode = oldOperation.miNode;
    miVorNode = oldOperation.miVorNode;
    return *this;
}

TDVOMInsertNode* TDVOMInsertNode::Clone() const {
    return new TDVOMInsertNode(*this);
}

void TDVOMInsertNode::Finished() {
    mpCloneObject.reset();
    mpTmpObject.reset();
    meObjectType = VECOBJ_UNKNOWN;
    mbInsertOK = false;
    mbObjectTypeOK = false;
    bOnObject = false;
    miNode = -1;
    miVorNode = -1;
}

bool TDVOMInsertNode::SelectedObjectTypeOK() const {
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

double TDVOMInsertNode::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad ? mpVecEditCad->GetActiveGraphicEngine() : nullptr;
    if (!graphicEngine) {
        return 1.0;
    }
    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

long TDVOMInsertNode::PointOnNode(TDVecObject* object, TDMatPoint point) const {
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

long TDVOMInsertNode::PointAfterNode(TDVecObject* object, TDMatPoint point) const {
    if (!object) {
        return -1;
    }
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(object)) {
        return graphicEngine
            ? bspline->PointAfterControle(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance())
            : pointAfterFromPoints(bspline->CountPoints(), [bspline](int i) { return bspline->GetPoint(i); }, point, MouseToleranceReal());
    }
    if (auto* polygon = dynamic_cast<TDVecPolygon*>(object)) {
        return graphicEngine
            ? polygon->PointAfterNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance())
            : pointAfterFromPoints(polygon->CountPoints(), [polygon](int i) { return polygon->GetPoint(i); }, point, MouseToleranceReal());
    }
    if (auto* polygonArea = dynamic_cast<TDVecPolygonArea*>(object)) {
        return graphicEngine
            ? polygonArea->PointAfterNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance())
            : pointAfterFromPoints(polygonArea->CountPoints(), [polygonArea](int i) { return polygonArea->GetPoint(i); }, point, MouseToleranceReal());
    }
    if (auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(object)) {
        if (graphicEngine) {
            return polyCurve->PointAfterNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
        }
        double best = std::numeric_limits<double>::infinity();
        long bestIndex = -1;
        for (int i = 1; i < polyCurve->CountPoints(); ++i) {
            const TDMatConturPoint* previous = polyCurve->GetPoint(i - 1);
            const TDMatConturPoint* current = polyCurve->GetPoint(i);
            if (!previous || !current) {
                continue;
            }
            const double d = distanceToSegment(point, {previous->x, previous->y}, {current->x, current->y});
            if (d <= MouseToleranceReal() && d < best) {
                best = d;
                bestIndex = i - 1;
            }
        }
        return bestIndex;
    }
    if (auto* polyCurveArea = dynamic_cast<TDVecPolyCurveArea*>(object)) {
        if (graphicEngine) {
            return polyCurveArea->PointAfterNode(graphicEngine, point.x, point.y, graphicEngine->GetMouseTolerance());
        }
        double best = std::numeric_limits<double>::infinity();
        long bestIndex = -1;
        for (int i = 1; i < polyCurveArea->CountPoints(); ++i) {
            const TDMatConturPoint* previous = polyCurveArea->GetPoint(i - 1);
            const TDMatConturPoint* current = polyCurveArea->GetPoint(i);
            if (!previous || !current) {
                continue;
            }
            const double d = distanceToSegment(point, {previous->x, previous->y}, {current->x, current->y});
            if (d <= MouseToleranceReal() && d < best) {
                best = d;
                bestIndex = i - 1;
            }
        }
        return bestIndex;
    }
    return -1;
}

bool TDVOMInsertNode::InsertPointInto(TDVecObject* object, long afterNode, TDMatPoint point, TDOPVirtKeyState shift) {
    if (!object || afterNode == -1) {
        return false;
    }
    const int insertIndex = static_cast<int>(afterNode + 1);
    if (auto* polygon = dynamic_cast<TDVecPolygon*>(object)) {
        return polygon->InsertPoint(insertIndex, point);
    }
    if (auto* polygonArea = dynamic_cast<TDVecPolygonArea*>(object)) {
        return polygonArea->InsertPoint(insertIndex, point);
    }
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(object)) {
        const bool inserted = bspline->InsertPoint(insertIndex, point);
        bspline->GenerateKnot();
        bspline->ComputeCurve();
        return inserted;
    }
    const TDMatConturPoint conturPoint{point.x, point.y, (shift == VKState_KEY_CTRL ? CPT_PRIME_QSPLINE : CPT_PRIME_LINE), true};
    if (auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(object)) {
        return polyCurve->InsertPoint(insertIndex, conturPoint);
    }
    if (auto* polyCurveArea = dynamic_cast<TDVecPolyCurveArea*>(object)) {
        return polyCurveArea->InsertPoint(insertIndex, conturPoint);
    }
    return false;
}

void __fastcall TDVOMInsertNode::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        miNode = -1;
        miVorNode = -1;
        mbInsertOK = false;
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
        SetState(OSTATE_ABORTED);
        mbInsertOK = false;
        return;
    }

    meObjectType = selectedObject->GetType();
    mbObjectTypeOK = true;
    mpCloneObject = selectedObject->Clone();
    const TDMatPoint point{X, Y};
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    bOnObject = graphicEngine
        ? selectedObject->HitTest(graphicEngine, point, graphicEngine->GetMouseTolerance()).IsHit()
        : selectedObject->HitTest(point, MouseToleranceReal()).IsHit();
    miVorNode = PointAfterNode(selectedObject, point);
    miNode = PointOnNode(selectedObject, point);
    if (miNode != -1) {
        miVorNode = -1;
    }

    if (miVorNode != -1) {
        mbInsertOK = InsertPointInto(mpCloneObject.get(), miVorNode, point, Shift);
        if (!mbInsertOK) {
            SetState(OSTATE_ABORTED);
            mpVecEditCad->Beep();
            return;
        }
        SetState(OSTATE_RUNNING);
        XMouseStart = X;
        YMouseStart = Y;
        XMouseJet = X;
        YMouseJet = Y;
    } else if (bOnObject && miNode != -1) {
        mbInsertOK = false;
        SetState(OSTATE_STARTED);
    } else {
        mbInsertOK = false;
        SetState(OSTATE_ABORTED);
    }
}

void __fastcall TDVOMInsertNode::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (Button != VIRTMOUSEBTM_LEFT || !mpVecModel->GetSelectedObject() || !mbInsertOK) {
        return;
    }
    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (!InsertPointInto(selectedObject, miVorNode, {X, Y}, Shift)) {
        mpVecEditCad->Beep();
        return;
    }
    selectedObject->Initialize();
    mpVecModel->MarkChanged();
    Finished();
    mpVecEditCad->TmpClear();
    mpVecEditCad->ViewsRefresh();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
}

void __fastcall TDVOMInsertNode::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (Button == VIRTMOUSEBTM_LEFT && mbInsertOK && mpCloneObject && miVorNode != -1) {
        mpTmpObject = mpCloneObject->Clone();
        if (auto* bspline = dynamic_cast<TDVecBSPLine*>(mpTmpObject.get())) {
            bspline->MoveControle(miVorNode + 1, X - XMouseStart, Y - YMouseStart);
        } else {
            mpTmpObject->MoveNode(miVorNode + 1, X - XMouseStart, Y - YMouseStart, {X, Y});
        }
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
        XMouseJet = X;
        YMouseJet = Y;
    }
}

void __fastcall TDVOMInsertNode::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMInsertNode::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
