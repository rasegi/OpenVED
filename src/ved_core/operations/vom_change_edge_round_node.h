//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Change-EdgeRound-Node
//---------------------------------------------------------------------------
#ifndef __VOM_CHANGE_EDGE_ROUND_NODE_H
#define __VOM_CHANGE_EDGE_ROUND_NODE_H

#include "vop_base.h"

class TDVOMChangeEdgeRoundNode : public TDVOModify {
public:
    TDVOMChangeEdgeRoundNode(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMChangeEdgeRoundNode(const TDVOMChangeEdgeRoundNode&);
    ~TDVOMChangeEdgeRoundNode() override;
    TDVOMChangeEdgeRoundNode& operator=(const TDVOMChangeEdgeRoundNode&);
    TDVOMChangeEdgeRoundNode* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMChangeEdgeRoundNode();
    void Finished();
    bool SelectedObjectTypeOK() const;
    long FindNodeOnSelectedObject(TDMatPoint point) const;
    bool ChangeSelectedNodeType();
    double MouseToleranceReal() const;

    TDVecObjectType meObjectType;
    bool mbChangeOK;
    bool mbObjectTypeOK;
    bool mbOnObject;
    long miNode;
};

#endif
