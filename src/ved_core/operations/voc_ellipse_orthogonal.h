//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Ellipse-Orthogonal
//---------------------------------------------------------------------------
#ifndef __VOC_ELLIPSE_ORTHOGONAL_H
#define __VOC_ELLIPSE_ORTHOGONAL_H

#include "vec_edit_cad.h"
#include "vop_base.h"

#include <memory>

class TDVecEllipse;

class TDVOCEllipseOrthogonal : public TDVOCreate {
public:
    TDVOCEllipseOrthogonal(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCEllipseOrthogonal* Clone() const override;
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCEllipseOrthogonal();
    bool IsEllipseValid() const;
    void Reset();
    void AppendEllipse();

    std::unique_ptr<TDVecEllipse> mpObjEllipse;
    TDMatEllipse mMatEllipse;
    bool mbEllipseOK;
};

#endif
