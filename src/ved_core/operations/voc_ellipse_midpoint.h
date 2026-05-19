//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Ellipse-Midpoint
//---------------------------------------------------------------------------
#ifndef __VOC_ELLIPSE_MIDPOINT_H
#define __VOC_ELLIPSE_MIDPOINT_H

#include "vec_edit_cad.h"
#include "vop_base.h"

#include <memory>

class TDVecEllipse;
class TDVecLine;

class TDVOCEllipseMidpoint : public TDVOCreate {
public:
    TDVOCEllipseMidpoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCEllipseMidpoint* Clone() const override;
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCEllipseMidpoint();
    bool IsAxisValid() const;
    bool IsEllipseValid() const;
    void Reset();
    void AppendEllipse();

    std::unique_ptr<TDVecEllipse> mpObjEllipse;
    std::unique_ptr<TDVecLine> mpTmpLine;
    TDMatEllipse mMatEllipse;
    TDMatLine mMatLine;
    bool mbAxeOK;
    bool mbEllipseOK;
};

#endif
