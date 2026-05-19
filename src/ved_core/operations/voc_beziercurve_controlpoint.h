//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-BezierCurve
//---------------------------------------------------------------------------
#ifndef __VOC_BEZIERCURVE_CONTROLPOINT_H
#define __VOC_BEZIERCURVE_CONTROLPOINT_H

#include "vop_base.h"

#include <memory>

class TDVecBezierCurve;
class TDVecLine;
class TDVecPolygon;

class TDVOCBezierCurveControlPoint : public TDVOCreate {
public:
    TDVOCBezierCurveControlPoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCBezierCurveControlPoint(const TDVOCBezierCurveControlPoint&);
    ~TDVOCBezierCurveControlPoint() override = default;
    TDVOCBezierCurveControlPoint& operator=(const TDVOCBezierCurveControlPoint&);
    TDVOCBezierCurveControlPoint* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCBezierCurveControlPoint();
    void Finish();

    std::unique_ptr<TDVecBezierCurve> mpObjBezierCurve;
    std::unique_ptr<TDVecLine> mpTmpLine;
    std::unique_ptr<TDVecPolygon> mpTmpPolygon;
    TDMatLine MatLine;
    TDMatPointsArray mMatPoints;
};

#endif
