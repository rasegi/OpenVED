//---------------------------------------------------------------------------
// Qt port note: this is the first reduced, compilable operation manager slice.
// It currently owns and dispatches only the operation types already moved into
// ved_core/operations.
//---------------------------------------------------------------------------
#ifndef __VOP_MANAGER_H
#define __VOP_MANAGER_H

#include "vec_model.h"
#include "vop_base.h"

#include <memory>
#include <string>
#include <vector>

class TDVecEditCad;
class TDVecObject;
enum class TDVecAlignMode;

struct TDCommandHistoryEntry {
    std::string name;
    TDVecModelSnapshot before;
    TDVecModelSnapshot after;
};

class TDViewOperationManager {
public:
    TDViewOperationManager();
    TDViewOperationManager(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad);
    ~TDViewOperationManager();

    void MouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void MouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void MouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);

    void KeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey);
    void KeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey);

    void SetOperation(TDViewOperationEnum ViewOperationName);
    void SetDefaultOperation(TDViewOperationEnum eDefaultOperation);
    TDViewOperationEnum GetDefaultOperation();
    void AktivateDefaultOperation();
    void SetActiveChangeToDefOPAfterFinish(bool bValue);
    bool GetActiveChangeToDefOPAfterFinish();
    void SetActiveChangeToDefOPAfterAbort(bool bValue);
    bool GetActiveChangeToDefOPAfterAbort();

    TDVecModel* GetVecModel();
    TDVecEditCad* GetVecEditCad();
    const TDViewOperation* GetAktiveOperation() const;
    const TDViewOperationEnum GetAktiveOperationType() const;
    bool MakeGroup();
    bool ResolveAllSelectedGroups();
    bool AlignSelectedObjects(TDVecAlignMode mode);
    bool CopySelectedObjects();
    bool CutSelectedObjects();
    bool PasteClipboardObjects();
    bool HasClipboardObjects() const;
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void ApplyExternalVariablesChanged(TDViewOperation* operation);

    bool SetActiveVOPExtVariables(TDVOPExternalVariables* pVariables);
    TDVOPExternalVariables* GetActiveVOPExtVariables();

    bool IsVOPExternalVariablesChanged();
    void VOPExternalVariablesIsChanged();

    void SendUpdateExtVarToParent();
    void Send_CanceledOPMessage_ToParent();
    void Send_StartedOPMessage_ToParent();
    void Send_OpenDialogMessage_ToParent();
    void Send_BeepMessage_ToParent();

    bool SetVecModel(TDVecModel* pVecModel);
    bool SetVecEditCad(TDVecEditCad* pVecEditCad);

private:
    void RecordHistoryIfChanged(const std::string& name, TDVecModelSnapshot before, std::uint64_t beforeRevision);
    void PushHistoryEntry(TDCommandHistoryEntry entry);
    void ActivateSelectMoveAfterMoveFinished();
    void ActivateSelectMoveForNeedSelected();

    TDViewOperation* mpAktiveOperation;
    TDVecModel* mpVecModel;
    TDVecEditCad* mpVecEditCad;
    bool mbChangedVOPExternalVariables;
    TDViewOperationEnum meDefaultOperation;
    bool mbActiveChangeToDefOPAfterFinish;
    bool mbActiveChangeToDefOPAfterAbort;
    std::vector<std::unique_ptr<TDVecObject>> clipboardObjects_;
    int clipboardPasteCount_;
    std::vector<TDCommandHistoryEntry> undoStack_;
    std::vector<TDCommandHistoryEntry> redoStack_;
};

#endif
