//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Move-Node
//---------------------------------------------------------------------------
#ifndef __VOM_MOVE_NODE_H
#define __VOM_MOVE_NODE_H

#include "vop_base.h"

#include <memory>

class TDVOMMoveNode : public TDVOModify {
public:
    TDVOMMoveNode(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMMoveNode(const TDVOMMoveNode&);
    ~TDVOMMoveNode() override = default;
    TDVOMMoveNode& operator=(const TDVOMMoveNode&);
    TDVOMMoveNode* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMMoveNode();
    double MouseToleranceReal() const;

    std::unique_ptr<TDVecObject> mpObject;
    bool mbDragOK;
    long miNode;
};

#endif
