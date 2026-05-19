//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Delete-Object
//---------------------------------------------------------------------------
#ifndef __VOM_DELETE_OBJECT_H
#define __VOM_DELETE_OBJECT_H

#include "vec_model.h"
#include "vop_base.h"

class TDVOMDeleteObject : public TDVOModify {
public:
    TDVOMDeleteObject(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMDeleteObject(const TDVOMDeleteObject&);
    ~TDVOMDeleteObject() override;
    TDVOMDeleteObject& operator=(const TDVOMDeleteObject&);
    TDVOMDeleteObject* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMDeleteObject();

    TDVecModelHitResult FindHit(TDMatPoint point) const;
    double MouseToleranceReal() const;
    void DeleteSelectedAndFinish();
    void DeleteObjectAndFinish(int objectIndex);

    unsigned int mClick;
    long miIndexObject;
};

#endif
