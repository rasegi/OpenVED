//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Polygon-Smartline
//---------------------------------------------------------------------------
#ifndef __VOC_POLYGON_SMARTLINE_H
#define __VOC_POLYGON_SMARTLINE_H

#include "vop_base.h"

#include <memory>
#include <vector>

class TDVecPolygon;

class TDVOCPolygonSmartline : public TDVOCreate {
public:
    TDVOCPolygonSmartline(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCPolygonSmartline(const TDVOCPolygonSmartline&);
    ~TDVOCPolygonSmartline() override = default;
    TDVOCPolygonSmartline& operator=(const TDVOCPolygonSmartline&);
    TDVOCPolygonSmartline* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCPolygonSmartline();
    void Finish();
    std::unique_ptr<TDVecPolygon> CreatePolygonFromPoints() const;

    std::unique_ptr<TDVecObject> mpTmpObject;
    std::vector<TDMatPoint> mMatPoints;
};

#endif
