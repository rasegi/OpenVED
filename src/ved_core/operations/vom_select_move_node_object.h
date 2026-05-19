//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Select-Move-Node-Object
//---------------------------------------------------------------------------
#ifndef __VOM_SELECT_MOVE_NODE_OBJECT_H
#define __VOM_SELECT_MOVE_NODE_OBJECT_H

#include "vec_model.h"
#include "vop_base.h"

#include <memory>
#include <vector>

class TDVOMSelectMoveNodeObject : public TDVOModify {
public:
    TDVOMSelectMoveNodeObject(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMSelectMoveNodeObject(const TDVOMSelectMoveNodeObject&);
    ~TDVOMSelectMoveNodeObject() override;
    TDVOMSelectMoveNodeObject& operator=(const TDVOMSelectMoveNodeObject&);
    TDVOMSelectMoveNodeObject* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMSelectMoveNodeObject();

    TDVecModelHitResult FindHit(TDMatPoint point) const;
    TDVecModelHitResult FindSelectedNode(TDMatPoint point) const;
    double MouseToleranceReal() const;
    bool HasMovedPastTolerance(TDMatPoint point) const;
    void ResetInteractiveState();
    void UpdateMoveShadow(TDMatPoint point);
    void UpdateNodeShadow(TDMatPoint point);
    void UpdateGroupShadow(TDMatPoint point);

    long miObjectIndex;
    TDVecObject* mpObject;
    std::unique_ptr<TDVecObject> mpTmpObject;
    std::vector<std::unique_ptr<TDVecObject>> mTmpGroupObjects;
    long mnNodeMove;
    TDMatPoint mMouseStart;
    TDMatPoint mMouseCurrent;
    bool mbMoveObject;
    bool mbMoveNode;
    bool mbGroupMove;
    bool mbObjectChanged;
    bool mbToleranceOK;
    bool mbAreaSelect;
    bool mbAreaMoved;
};

#endif
