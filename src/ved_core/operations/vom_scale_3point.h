//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Scale-Object-3Point
//---------------------------------------------------------------------------
#ifndef __VOM_SCALE_OBJECT_3POINT_H
#define __VOM_SCALE_OBJECT_3POINT_H

#include "vop_base.h"

#include <memory>

class TDVOMScaleObject3Point : public TDVOModify {
public:
    TDVOMScaleObject3Point(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMScaleObject3Point(const TDVOMScaleObject3Point&);
    ~TDVOMScaleObject3Point() override = default;
    TDVOMScaleObject3Point& operator=(const TDVOMScaleObject3Point&);
    TDVOMScaleObject3Point* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMScaleObject3Point();

    std::unique_ptr<TDVecObject> mpCloneObject;
    std::unique_ptr<TDVecObject> mpTmpObject;
    double mnXScale;
    double mnYScale;
    double mnXBasis;
    double mnYBasis;
    TDMatPoint MatPoint1;
    TDMatPoint MatPoint2;
    unsigned int mnClick;
    bool mbScaleOK;
};

#endif
