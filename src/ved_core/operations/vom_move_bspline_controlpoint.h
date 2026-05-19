//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Move-BSPLine-ControlePoint
//---------------------------------------------------------------------------
#ifndef __VOM_MOVE_BSPLINE_CONTROLEPOINT_H
#define __VOM_MOVE_BSPLINE_CONTROLEPOINT_H

#include "vop_base.h"

#include <memory>

class TDVOMMoveBSPLineControlPoint : public TDVOModify {
public:
    TDVOMMoveBSPLineControlPoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMMoveBSPLineControlPoint(const TDVOMMoveBSPLineControlPoint&);
    ~TDVOMMoveBSPLineControlPoint() override = default;
    TDVOMMoveBSPLineControlPoint& operator=(const TDVOMMoveBSPLineControlPoint&);
    TDVOMMoveBSPLineControlPoint* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOMMoveBSPLineControlPoint();

    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbDragOK;
    long miControle;
};

#endif
