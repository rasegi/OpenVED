//---------------------------------------------------------------------------
// Qt port note: reduced to the currently ported operation set.
//---------------------------------------------------------------------------
#include "vop_manager.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_object.h"
#include "voc_beziercurve_controlpoint.h"
#include "voc_bspline_controlpoint.h"
#include "voc_circle_diameter.h"
#include "voc_circle_diagonal.h"
#include "voc_circle_edge.h"
#include "voc_circle_midpoint.h"
#include "voc_ellipse_midpoint.h"
#include "voc_ellipse_orthogonal.h"
#include "voc_line.h"
#include "voc_polygon_smartline.h"
#include "voc_polycurve.h"
#include "voc_rectangle_notrotated.h"
#include "voc_rectangle_rotated.h"
#include "voc_roundrectangle_notrotated.h"
#include "voc_vecframetext_empty.h"
#include "voc_vectext.h"
#include "vom_change_edge_round_node.h"
#include "vom_delete_node.h"
#include "vom_delete_object.h"
#include "vom_insert_node.h"
#include "vom_insert_objects.h"
#include "vom_modify_curve_attribute.h"
#include "vom_move_bspline_controlpoint.h"
#include "vom_move_node.h"
#include "vom_move_object.h"
#include "vom_rotate_object_activepoint.h"
#include "vom_scale_3point.h"
#include "vom_select_object.h"
#include "vom_selectmove_object_scale_frame.h"
#include "vom_select_move_node_object.h"
#include "vom_vectext.h"

namespace {
constexpr std::size_t kUndoLimit = 50;
}

TDViewOperationManager::TDViewOperationManager()
    : mpAktiveOperation(nullptr),
      mpVecModel(nullptr),
      mpVecEditCad(nullptr),
      mbChangedVOPExternalVariables(false),
      meDefaultOperation(VO_UNKNOWN),
      mbActiveChangeToDefOPAfterFinish(false),
      mbActiveChangeToDefOPAfterAbort(false),
      clipboardObjects_(),
      clipboardPasteCount_(1),
      undoStack_(),
      redoStack_() {
}

TDViewOperationManager::TDViewOperationManager(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad)
    : TDViewOperationManager() {
    SetVecModel(pVecModel);
    SetVecEditCad(pVecEditCad);
}

TDViewOperationManager::~TDViewOperationManager() {
    delete mpAktiveOperation;
}

bool TDViewOperationManager::SetVecModel(TDVecModel* pVecModel) {
    if (!pVecModel) {
        return false;
    }
    mpVecModel = pVecModel;
    return true;
}

TDVecModel* TDViewOperationManager::GetVecModel() {
    return mpVecModel;
}

bool TDViewOperationManager::SetVecEditCad(TDVecEditCad* pVecEditCad) {
    if (!pVecEditCad) {
        return false;
    }
    mpVecEditCad = pVecEditCad;
    mpVecEditCad->SetViewOperationManager(this);
    return true;
}

TDVecEditCad* TDViewOperationManager::GetVecEditCad() {
    return mpVecEditCad;
}

void TDViewOperationManager::SetDefaultOperation(TDViewOperationEnum eDefaultOperation) {
    meDefaultOperation = eDefaultOperation;
}

TDViewOperationEnum TDViewOperationManager::GetDefaultOperation() {
    return meDefaultOperation;
}

void TDViewOperationManager::AktivateDefaultOperation() {
    if (GetAktiveOperationType() != meDefaultOperation) {
        SetOperation(meDefaultOperation);
    }
}

void TDViewOperationManager::SetActiveChangeToDefOPAfterFinish(bool bValue) {
    mbActiveChangeToDefOPAfterFinish = bValue;
}

bool TDViewOperationManager::GetActiveChangeToDefOPAfterFinish() {
    return mbActiveChangeToDefOPAfterFinish;
}

void TDViewOperationManager::SetActiveChangeToDefOPAfterAbort(bool bValue) {
    mbActiveChangeToDefOPAfterAbort = bValue;
}

bool TDViewOperationManager::GetActiveChangeToDefOPAfterAbort() {
    return mbActiveChangeToDefOPAfterAbort;
}

void TDViewOperationManager::SetOperation(TDViewOperationEnum ViewOperationName) {
    if (!mpVecModel || !mpVecEditCad) {
        return;
    }

    if (mpAktiveOperation) {
        if (mpAktiveOperation->GetState() == OSTATE_RUNNING) {
            mpVecEditCad->ViewsRefresh();
            mpVecEditCad->TmpClear();
        }
        delete mpAktiveOperation;
        mpAktiveOperation = nullptr;
        Send_CanceledOPMessage_ToParent();
    }

    switch (ViewOperationName) {
    case VOC_CIRCLE_EDGE:
        mpAktiveOperation = new TDVOCCircleEdge(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_CIRCLE_MIDPOINT:
        mpAktiveOperation = new TDVOCCircleMidpoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_CIRCLE_DIAMETER:
        mpAktiveOperation = new TDVOCCircleDiameter(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_CIRCLE_DIAGONAL:
        mpAktiveOperation = new TDVOCCircleDiagonal(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_ELLIPSE_MIDPOINT:
        mpAktiveOperation = new TDVOCEllipseMidpoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_ELLIPSE_ORTHOGONAL:
        mpAktiveOperation = new TDVOCEllipseOrthogonal(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_LINE:
        mpAktiveOperation = new TDVOCLine(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_POLYGON_SMARTLINE:
        mpAktiveOperation = new TDVOCPolygonSmartline(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_POLYCURVE:
        mpAktiveOperation = new TDVOCPolyCurve(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_VECTEXT:
        mpAktiveOperation = new TDVOCVecText(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_VECFRAMETEXT_EMPTY:
        mpAktiveOperation = new TDVOCVecFrameTextEmpty(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_BEZIERCURVE_CONTROLPOINT:
        mpAktiveOperation = new TDVOCBezierCurveControlPoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_BSPLINE_CONTROLPOINT:
        mpAktiveOperation = new TDVOCBSPLineControlPoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_RECTANGLE_NOTROTATED:
        mpAktiveOperation = new TDVOCRectangleNotRotated(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_RECTANGLE_ROTATED:
        mpAktiveOperation = new TDVOCRectangleRotated(mpVecModel, mpVecEditCad, this);
        break;
    case VOC_ROUND_RECTANGLE_NOTROTATED:
        mpAktiveOperation = new TDVOCRoundRectangleNotRotated(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_SELECT_OBJECT:
        mpAktiveOperation = new TDVOMSelectObject(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_DELETE_OBJECT:
        mpAktiveOperation = new TDVOMDeleteObject(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_MOVE_OBJECT:
        mpAktiveOperation = new TDVOMMoveObject(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_MOVE_NODE:
        mpAktiveOperation = new TDVOMMoveNode(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_MOVE_BSPLINE_CONTROLPOINT:
        mpAktiveOperation = new TDVOMMoveBSPLineControlPoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_MODIFY_CURVE_ATTRIBUTE:
        mpAktiveOperation = new TDVOMModifyCurveAttribute(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_INSERT_NODE:
        mpAktiveOperation = new TDVOMInsertNode(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_DELETE_NODE:
        mpAktiveOperation = new TDVOMDeleteNode(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_ROTATE_OBJECT_ACTIVEPOINT:
        mpAktiveOperation = new TDVOMRotateObjectActivePoint(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_SCALE_OBJECT_3POINT:
        mpAktiveOperation = new TDVOMScaleObject3Point(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_INSERT_OBJECTS:
        mpAktiveOperation = new TDVOMInsertObjects(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_SELECT_MOVE_NODE_OBJECT:
        mpAktiveOperation = new TDVOMSelectMoveNodeObject(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_SELECTMOVE_OBJ_SCALE_FRAME:
        mpAktiveOperation = new TDVOMSelectMoveObjectScaleFrame(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_CHANGE_EDGE_ROUND_NODE:
        mpAktiveOperation = new TDVOMChangeEdgeRoundNode(mpVecModel, mpVecEditCad, this);
        break;
    case VOM_VECTEXT:
        mpAktiveOperation = new TDVOMVecText(mpVecModel, mpVecEditCad, this);
        break;
    default:
        break;
    }

    if (mpAktiveOperation) {
        mpAktiveOperation->Initialize();
        Send_StartedOPMessage_ToParent();
        if (mpAktiveOperation->GetState() == OSTATE_NEEDSELECTED) {
            ActivateSelectMoveForNeedSelected();
        }
    }
}

void TDViewOperationManager::MouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (mpAktiveOperation) {
        auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
        const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
        switch (mpAktiveOperation->GetState()) {
        case OSTATE_RUNNING:
        case OSTATE_STARTED:
        case OSTATE_FINISHED:
        case OSTATE_ABORTED:
            mpAktiveOperation->OPMouseDown(Button, Shift, X, Y);
            break;
        case OSTATE_NEEDSELECTED:
            ActivateSelectMoveForNeedSelected();
            break;
        case OSTATE_UNKNOWN:
            break;
        }
        RecordHistoryIfChanged("MouseDown", std::move(before), beforeRevision);
        ActivateSelectMoveAfterMoveFinished();
    }
}

void TDViewOperationManager::MouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (mpAktiveOperation) {
        auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
        const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
        switch (mpAktiveOperation->GetState()) {
        case OSTATE_RUNNING:
            mpAktiveOperation->OPMouseUp(Button, Shift, X, Y);
            break;
        case OSTATE_NEEDSELECTED:
            ActivateSelectMoveForNeedSelected();
            break;
        case OSTATE_UNKNOWN:
        case OSTATE_STARTED:
        case OSTATE_FINISHED:
        case OSTATE_ABORTED:
            break;
        }
        RecordHistoryIfChanged("MouseUp", std::move(before), beforeRevision);
        ActivateSelectMoveAfterMoveFinished();
    }
}

void TDViewOperationManager::MouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (mpAktiveOperation) {
        switch (mpAktiveOperation->GetState()) {
        case OSTATE_RUNNING:
            mpAktiveOperation->OPMouseMove(Button, Shift, X, Y);
            break;
        case OSTATE_NEEDSELECTED:
            ActivateSelectMoveForNeedSelected();
            break;
        case OSTATE_UNKNOWN:
        case OSTATE_STARTED:
        case OSTATE_FINISHED:
        case OSTATE_ABORTED:
            break;
        }
    }
}

void TDViewOperationManager::KeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    if (mpAktiveOperation) {
        auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
        const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
        mpAktiveOperation->OPKeyDown(eVirtualKey, StateKey);
        RecordHistoryIfChanged("KeyDown", std::move(before), beforeRevision);
    }
}

void TDViewOperationManager::KeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    if (mpAktiveOperation) {
        auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
        const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
        mpAktiveOperation->OPKeyUp(eVirtualKey, StateKey);
        RecordHistoryIfChanged("KeyUp", std::move(before), beforeRevision);
    }
}

const TDViewOperation* TDViewOperationManager::GetAktiveOperation() const {
    return mpAktiveOperation;
}

const TDViewOperationEnum TDViewOperationManager::GetAktiveOperationType() const {
    if (!mpAktiveOperation) {
        return VO_UNKNOWN;
    }
    if (dynamic_cast<const TDVOCLine*>(mpAktiveOperation)) {
        return VOC_LINE;
    }
    if (dynamic_cast<const TDVOCPolygonSmartline*>(mpAktiveOperation)) {
        return VOC_POLYGON_SMARTLINE;
    }
    if (dynamic_cast<const TDVOCPolyCurve*>(mpAktiveOperation)) {
        return VOC_POLYCURVE;
    }
    if (dynamic_cast<const TDVOCVecText*>(mpAktiveOperation)) {
        return VOC_VECTEXT;
    }
    if (dynamic_cast<const TDVOCVecFrameTextEmpty*>(mpAktiveOperation)) {
        return VOC_VECFRAMETEXT_EMPTY;
    }
    if (dynamic_cast<const TDVOCBezierCurveControlPoint*>(mpAktiveOperation)) {
        return VOC_BEZIERCURVE_CONTROLPOINT;
    }
    if (dynamic_cast<const TDVOCBSPLineControlPoint*>(mpAktiveOperation)) {
        return VOC_BSPLINE_CONTROLPOINT;
    }
    if (dynamic_cast<const TDVOCRectangleNotRotated*>(mpAktiveOperation)) {
        return VOC_RECTANGLE_NOTROTATED;
    }
    if (dynamic_cast<const TDVOCRectangleRotated*>(mpAktiveOperation)) {
        return VOC_RECTANGLE_ROTATED;
    }
    if (dynamic_cast<const TDVOCRoundRectangleNotRotated*>(mpAktiveOperation)) {
        return VOC_ROUND_RECTANGLE_NOTROTATED;
    }
    if (dynamic_cast<const TDVOCCircleEdge*>(mpAktiveOperation)) {
        return VOC_CIRCLE_EDGE;
    }
    if (dynamic_cast<const TDVOCCircleMidpoint*>(mpAktiveOperation)) {
        return VOC_CIRCLE_MIDPOINT;
    }
    if (dynamic_cast<const TDVOCCircleDiameter*>(mpAktiveOperation)) {
        return VOC_CIRCLE_DIAMETER;
    }
    if (dynamic_cast<const TDVOCCircleDiagonal*>(mpAktiveOperation)) {
        return VOC_CIRCLE_DIAGONAL;
    }
    if (dynamic_cast<const TDVOCEllipseMidpoint*>(mpAktiveOperation)) {
        return VOC_ELLIPSE_MIDPOINT;
    }
    if (dynamic_cast<const TDVOCEllipseOrthogonal*>(mpAktiveOperation)) {
        return VOC_ELLIPSE_ORTHOGONAL;
    }
    if (dynamic_cast<const TDVOMSelectObject*>(mpAktiveOperation)) {
        return VOM_SELECT_OBJECT;
    }
    if (dynamic_cast<const TDVOMDeleteObject*>(mpAktiveOperation)) {
        return VOM_DELETE_OBJECT;
    }
    if (dynamic_cast<const TDVOMMoveObject*>(mpAktiveOperation)) {
        return VOM_MOVE_OBJECT;
    }
    if (dynamic_cast<const TDVOMMoveNode*>(mpAktiveOperation)) {
        return VOM_MOVE_NODE;
    }
    if (dynamic_cast<const TDVOMMoveBSPLineControlPoint*>(mpAktiveOperation)) {
        return VOM_MOVE_BSPLINE_CONTROLPOINT;
    }
    if (dynamic_cast<const TDVOMModifyCurveAttribute*>(mpAktiveOperation)) {
        return VOM_MODIFY_CURVE_ATTRIBUTE;
    }
    if (dynamic_cast<const TDVOMInsertNode*>(mpAktiveOperation)) {
        return VOM_INSERT_NODE;
    }
    if (dynamic_cast<const TDVOMDeleteNode*>(mpAktiveOperation)) {
        return VOM_DELETE_NODE;
    }
    if (dynamic_cast<const TDVOMRotateObjectActivePoint*>(mpAktiveOperation)) {
        return VOM_ROTATE_OBJECT_ACTIVEPOINT;
    }
    if (dynamic_cast<const TDVOMScaleObject3Point*>(mpAktiveOperation)) {
        return VOM_SCALE_OBJECT_3POINT;
    }
    if (dynamic_cast<const TDVOMInsertObjects*>(mpAktiveOperation)) {
        return VOM_INSERT_OBJECTS;
    }
    if (dynamic_cast<const TDVOMSelectMoveNodeObject*>(mpAktiveOperation)) {
        return VOM_SELECT_MOVE_NODE_OBJECT;
    }
    if (dynamic_cast<const TDVOMSelectMoveObjectScaleFrame*>(mpAktiveOperation)) {
        return VOM_SELECTMOVE_OBJ_SCALE_FRAME;
    }
    if (dynamic_cast<const TDVOMChangeEdgeRoundNode*>(mpAktiveOperation)) {
        return VOM_CHANGE_EDGE_ROUND_NODE;
    }
    if (dynamic_cast<const TDVOMVecText*>(mpAktiveOperation)) {
        return VOM_VECTEXT;
    }
    return VO_UNKNOWN;
}

bool TDViewOperationManager::MakeGroup() {
    if (!mpVecModel || !mpVecEditCad) {
        return false;
    }

    auto before = mpVecModel->CreateSnapshot();
    const std::uint64_t beforeRevision = mpVecModel->Revision();
    if (mpVecModel->MakeGroup()) {
        RecordHistoryIfChanged("Make Group", std::move(before), beforeRevision);
        mpVecEditCad->ViewsRefresh();
        return true;
    }
    return false;
}

bool TDViewOperationManager::ResolveAllSelectedGroups() {
    if (!mpVecModel || !mpVecEditCad) {
        return false;
    }

    auto before = mpVecModel->CreateSnapshot();
    const std::uint64_t beforeRevision = mpVecModel->Revision();
    if (mpVecModel->ResolveAllSelectedGroups()) {
        RecordHistoryIfChanged("Resolve Group", std::move(before), beforeRevision);
        mpVecEditCad->ViewsRefresh();
        return true;
    }
    return false;
}

bool TDViewOperationManager::AlignSelectedObjects(TDVecAlignMode mode) {
    if (!mpVecModel || !mpVecEditCad) {
        return false;
    }

    auto before = mpVecModel->CreateSnapshot();
    const std::uint64_t beforeRevision = mpVecModel->Revision();
    if (mpVecModel->AlignSelectedObjects(mode) > 0) {
        RecordHistoryIfChanged("Align", std::move(before), beforeRevision);
        mpVecEditCad->ViewsRefresh();
        return true;
    }
    return false;
}

bool TDViewOperationManager::CopySelectedObjects() {
    if (!mpVecModel) {
        return false;
    }

    std::vector<std::unique_ptr<TDVecObject>> copiedObjects;
    for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
        if (!object) {
            continue;
        }

        auto clone = object->Clone();
        if (!clone) {
            continue;
        }
        clone->SetSelect(false);
        copiedObjects.push_back(std::move(clone));
    }

    if (copiedObjects.empty()) {
        return false;
    }

    clipboardObjects_ = std::move(copiedObjects);
    clipboardPasteCount_ = 1;
    return true;
}

bool TDViewOperationManager::CutSelectedObjects() {
    if (!CopySelectedObjects()) {
        return false;
    }

    auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
    const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
    const int deletedCount = mpVecModel ? mpVecModel->DeleteSelectedObjects() : 0;
    if (deletedCount > 0 && mpVecEditCad) {
        RecordHistoryIfChanged("Cut", std::move(before), beforeRevision);
        mpVecEditCad->ViewsRefresh();
    }
    return deletedCount > 0;
}

bool TDViewOperationManager::PasteClipboardObjects() {
    if (!mpVecModel || clipboardObjects_.empty()) {
        return false;
    }

    auto before = mpVecModel->CreateSnapshot();
    const std::uint64_t beforeRevision = mpVecModel->Revision();
    constexpr double kPasteOffset = 5000.0;
    const double offset = kPasteOffset * static_cast<double>(clipboardPasteCount_);
    int pastedCount = 0;

    mpVecModel->DeselectAll();
    for (const auto& clipboardObject : clipboardObjects_) {
        if (!clipboardObject) {
            continue;
        }

        auto clone = clipboardObject->Clone();
        if (!clone) {
            continue;
        }

        const bool lockPosition = clone->GetLockPosition();
        if (lockPosition) {
            clone->SetLockPosition(false);
        }
        clone->MoveBy(offset, offset);
        if (lockPosition) {
            clone->SetLockPosition(true);
        }
        clone->Initialize();
        clone->SetSelect(true);
        mpVecModel->AppendObject(clone.release());
        ++pastedCount;
    }

    if (pastedCount == 0) {
        return false;
    }

    RecordHistoryIfChanged("Paste", std::move(before), beforeRevision);
    ++clipboardPasteCount_;
    if (mpVecEditCad) {
        mpVecEditCad->ViewsRefresh();
    }
    return true;
}

bool TDViewOperationManager::HasClipboardObjects() const {
    return !clipboardObjects_.empty();
}

bool TDViewOperationManager::Undo() {
    if (!mpVecModel || undoStack_.empty()) {
        return false;
    }

    TDCommandHistoryEntry entry = std::move(undoStack_.back());
    undoStack_.pop_back();
    mpVecModel->RestoreSnapshot(entry.before, true);
    redoStack_.push_back(std::move(entry));
    if (mpVecEditCad) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }
    return true;
}

bool TDViewOperationManager::Redo() {
    if (!mpVecModel || redoStack_.empty()) {
        return false;
    }

    TDCommandHistoryEntry entry = std::move(redoStack_.back());
    redoStack_.pop_back();
    mpVecModel->RestoreSnapshot(entry.after, true);
    undoStack_.push_back(std::move(entry));
    if (undoStack_.size() > kUndoLimit) {
        undoStack_.erase(undoStack_.begin());
    }
    if (mpVecEditCad) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }
    return true;
}

bool TDViewOperationManager::CanUndo() const {
    return !undoStack_.empty();
}

bool TDViewOperationManager::CanRedo() const {
    return !redoStack_.empty();
}

void TDViewOperationManager::ApplyExternalVariablesChanged(TDViewOperation* operation) {
    if (!operation) {
        return;
    }

    auto before = mpVecModel ? mpVecModel->CreateSnapshot() : TDVecModelSnapshot{};
    const std::uint64_t beforeRevision = mpVecModel ? mpVecModel->Revision() : 0;
    operation->ExtVarIsChanged();
    RecordHistoryIfChanged("External Variables", std::move(before), beforeRevision);
}

void TDViewOperationManager::RecordHistoryIfChanged(const std::string& name, TDVecModelSnapshot before, std::uint64_t beforeRevision) {
    if (!mpVecModel || mpVecModel->Revision() == beforeRevision) {
        return;
    }

    PushHistoryEntry({name, std::move(before), mpVecModel->CreateSnapshot()});
}

void TDViewOperationManager::PushHistoryEntry(TDCommandHistoryEntry entry) {
    undoStack_.push_back(std::move(entry));
    if (undoStack_.size() > kUndoLimit) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void TDViewOperationManager::ActivateSelectMoveAfterMoveFinished() {
    if (!mpAktiveOperation || GetAktiveOperationType() != VOM_MOVE_OBJECT) {
        return;
    }

    const TDViewOperationState state = mpAktiveOperation->GetState();
    if (state == OSTATE_NEEDSELECTED || state == OSTATE_FINISHED || state == OSTATE_ABORTED) {
        SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
    }
}

void TDViewOperationManager::ActivateSelectMoveForNeedSelected() {
    if (!mpVecModel || !mpVecEditCad) {
        return;
    }
    if (mpAktiveOperation) {
        delete mpAktiveOperation;
        mpAktiveOperation = nullptr;
        Send_CanceledOPMessage_ToParent();
    }

    mpAktiveOperation = new TDVOMSelectMoveNodeObject(mpVecModel, mpVecEditCad, this);
    mpAktiveOperation->Initialize();
    Send_StartedOPMessage_ToParent();
}

bool TDViewOperationManager::SetActiveVOPExtVariables(TDVOPExternalVariables* pVariables) {
    if (!mpAktiveOperation || !pVariables) {
        return false;
    }
    return mpAktiveOperation->SetVOPExternalVariables(pVariables);
}

TDVOPExternalVariables* TDViewOperationManager::GetActiveVOPExtVariables() {
    return mpAktiveOperation ? mpAktiveOperation->GetVOPExternalVariables() : nullptr;
}

bool TDViewOperationManager::IsVOPExternalVariablesChanged() {
    return mbChangedVOPExternalVariables;
}

void TDViewOperationManager::VOPExternalVariablesIsChanged() {
    mbChangedVOPExternalVariables = true;
}

void TDViewOperationManager::SendUpdateExtVarToParent() {
    VOPExternalVariablesIsChanged();
}

void TDViewOperationManager::Send_CanceledOPMessage_ToParent() {
}

void TDViewOperationManager::Send_StartedOPMessage_ToParent() {
}

void TDViewOperationManager::Send_OpenDialogMessage_ToParent() {
}

void TDViewOperationManager::Send_BeepMessage_ToParent() {
    if (mpVecEditCad) {
        mpVecEditCad->Beep();
    }
}
