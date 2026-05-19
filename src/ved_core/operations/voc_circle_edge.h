//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Circle-Edge
//---------------------------------------------------------------------------
#ifndef __VOC_CIRCLE_EDGE_H
#define __VOC_CIRCLE_EDGE_H

#include "vec_edit_cad.h"
#include "vop_base.h"

#include <memory>

class TDVecEllipse;

class TDVOCCircleEdge : public TDVOCreate {
public:
    TDVOCCircleEdge(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCCircleEdge* Clone() const override;
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCCircleEdge();
    void Reset();
    bool UpdatePreviewCircle();
    bool UpdateFinalCircle(TDMatPoint edgePoint);
    void AppendCircle();

    std::unique_ptr<TDVecEllipse> mpObjCircle;
    TDMatCircle mMatCircle;
    bool mbCircleOK;
};

#endif
