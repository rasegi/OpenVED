//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Circle-Diameter
//---------------------------------------------------------------------------
#ifndef __VOC_CIRCLE_DIAMETER_H
#define __VOC_CIRCLE_DIAMETER_H

#include "vec_edit_cad.h"
#include "vop_base.h"

#include <memory>

class TDVecEllipse;

class TDVOCCircleDiameter : public TDVOCreate {
public:
    TDVOCCircleDiameter(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCCircleDiameter* Clone() const override;
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCCircleDiameter();
    bool IsCircleValid() const;
    void Reset();
    void AppendCircle();

    std::unique_ptr<TDVecEllipse> mpObjCircle;
    TDMatCircle mMatCircle;
    bool mbCircleOK;
};

#endif
