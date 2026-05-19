//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Move-Object
//---------------------------------------------------------------------------
#ifndef __VOM_MOVE_OBJECT_H
#define __VOM_MOVE_OBJECT_H

#include "vec_model.h"
#include "vop_base.h"

#include <memory>
#include <vector>

class TDVOMMoveObject : public TDVOModify {
public:
    TDVOMMoveObject(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMMoveObject(const TDVOMMoveObject&);
    ~TDVOMMoveObject() override;
    TDVOMMoveObject& operator=(const TDVOMMoveObject&);
    TDVOMMoveObject* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMMoveObject();

    bool InitializeFromSelection();
    TDMatPoint SelectionMidpoint() const;
    void UpdateMoveShadow(TDMatPoint point);
    void ClearShadow();

    std::vector<std::unique_ptr<TDVecObject>> mTmpObjects;
    TDMatPoint mMouseStart;
    bool mbGroupMove;
    bool mbDragOK;
};

#endif
