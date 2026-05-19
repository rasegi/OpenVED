//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-PolyCurve
//---------------------------------------------------------------------------
#ifndef __VOC_POLYCURVE_H
#define __VOC_POLYCURVE_H

#include "vop_base.h"
#include "vec_contur_point.h"

#include <memory>
#include <vector>

class TDVecPolyCurve;

class TDVOCPolyCurveExtVar : public TDVOPExternalVariables {
public:
    TDVOCPolyCurveExtVar(TDViewOperation* pParentOperation);
    TDVOCPolyCurveExtVar(const TDVOCPolyCurveExtVar&);
    ~TDVOCPolyCurveExtVar() override = default;
    TDVOCPolyCurveExtVar& operator=(const TDVOCPolyCurveExtVar&);
    TDVOCPolyCurveExtVar* Clone() const override;

    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;

private:
    TDVOCPolyCurveExtVar();

    unsigned int mnResolution;
    bool mbShowControls;
    bool mbShowPolygon;
};

class TDVOCPolyCurve : public TDVOCreate {
public:
    TDVOCPolyCurve(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCPolyCurve(const TDVOCPolyCurve&);
    ~TDVOCPolyCurve() override;
    TDVOCPolyCurve& operator=(const TDVOCPolyCurve&);
    TDVOCPolyCurve* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDVOCPolyCurve();
    void Finish();
    std::unique_ptr<TDVecPolyCurve> CreatePolyCurveFromPoints() const;

    TDVOCPolyCurveExtVar* mpVOCPolyCurveExtVar;
    std::unique_ptr<TDVecObject> mpTmpObject;
    TDMatLine mMatLine;
    std::vector<TDMatConturPoint> mConturPoints;
    bool mbPolyCurveOK;
    TDConturPointType mePreviusPointType;
};

#endif
