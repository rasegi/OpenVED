//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Insert-Node
//---------------------------------------------------------------------------
#ifndef __VOM_INSERT_NODE_H
#define __VOM_INSERT_NODE_H

#include "vop_base.h"

#include <memory>

class TDVOMInsertNode : public TDVOModify {
public:
    TDVOMInsertNode(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMInsertNode(const TDVOMInsertNode&);
    ~TDVOMInsertNode() override;
    TDVOMInsertNode& operator=(const TDVOMInsertNode&);
    TDVOMInsertNode* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMInsertNode();
    void Finished();
    bool SelectedObjectTypeOK() const;
    long PointAfterNode(TDVecObject* object, TDMatPoint point) const;
    long PointOnNode(TDVecObject* object, TDMatPoint point) const;
    bool InsertPointInto(TDVecObject* object, long afterNode, TDMatPoint point, TDOPVirtKeyState shift);
    double MouseToleranceReal() const;

    TDVecObjectType meObjectType;
    std::unique_ptr<TDVecObject> mpCloneObject;
    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbInsertOK;
    bool mbObjectTypeOK;
    bool bOnObject;
    long miNode;
    long miVorNode;
};

#endif
