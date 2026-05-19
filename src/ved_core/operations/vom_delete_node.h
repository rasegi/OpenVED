//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Delete-Node
//---------------------------------------------------------------------------
#ifndef __VOM_DELETE_NODE_H
#define __VOM_DELETE_NODE_H

#include "vop_base.h"

class TDVOMDeleteNode : public TDVOModify {
public:
    TDVOMDeleteNode(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMDeleteNode(const TDVOMDeleteNode&);
    ~TDVOMDeleteNode() override;
    TDVOMDeleteNode& operator=(const TDVOMDeleteNode&);
    TDVOMDeleteNode* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMDeleteNode();
    void Finished();
    bool SelectedObjectTypeOK() const;
    long PointOnDeletableNode(TDVecObject* object, TDMatPoint point) const;
    bool DeleteSelectedNode();
    double MouseToleranceReal() const;

    TDVecObjectType meObjectType;
    bool mbDeleteOK;
    bool mbObjectTypeOK;
    bool bOnObject;
    long miNode;
};

#endif
