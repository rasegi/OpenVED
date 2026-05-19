//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Circle-Diagonal
//---------------------------------------------------------------------------
#ifndef __VOC_CIRCLE_DIAGONAL_H
#define __VOC_CIRCLE_DIAGONAL_H

#include "vop_base.h"

#include <memory>

class TDVecEllipse;

class TDVOCCircleDiagonal : public TDVOCreate {
public:
    TDVOCCircleDiagonal(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCCircleDiagonal(const TDVOCCircleDiagonal&);
    ~TDVOCCircleDiagonal() override = default;
    TDVOCCircleDiagonal& operator=(const TDVOCCircleDiagonal&);
    TDVOCCircleDiagonal* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCCircleDiagonal();

    std::unique_ptr<TDVecEllipse> mpObjEllipse;
    TDMatEllipse mMatEllipse;
    bool mbEllipseOK;
};

#endif
