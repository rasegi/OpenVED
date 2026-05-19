//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Select-Object
//---------------------------------------------------------------------------
#ifndef __VOM_SELECT_OBJECT_H
#define __VOM_SELECT_OBJECT_H

#include "vop_base.h"

class TDVOMSelectObject : public TDVOModify {
public:
    TDVOMSelectObject(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMSelectObject(const TDVOMSelectObject&);
    ~TDVOMSelectObject() override;
    TDVOMSelectObject& operator=(const TDVOMSelectObject&);
    TDVOMSelectObject* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMSelectObject();
    double MouseToleranceReal() const;

    long miSelectObject;
    TDVecObject* mpSelectObject;
    TDMatPoint mAreaStart;
    TDMatPoint mAreaCurrent;
    bool mbAreaSelect;
    bool mbAreaMoved;
};

#endif
