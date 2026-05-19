//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Circle-Midpoint
//---------------------------------------------------------------------------
#ifndef __VOC_CIRCLE_MIDPOINT_H
#define __VOC_CIRCLE_MIDPOINT_H

#include "vec_edit_cad.h"
#include "vop_base.h"

#include <memory>

class TDVecEllipse;

class TDVOCCircleMidpoint : public TDVOCreate {
public:
    TDVOCCircleMidpoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCCircleMidpoint* Clone() const override;
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCCircleMidpoint();
    bool IsCircleValid() const;
    void Reset();
    void AppendCircle();

    std::unique_ptr<TDVecEllipse> mpObjCircle;
    TDMatCircle mMatCircle;
    bool mbCircleOK;
};

#endif
