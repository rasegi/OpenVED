//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-BSPLine-ControlPoint
//---------------------------------------------------------------------------
#ifndef __VOC_BSPLINE_CONTROLPOINT_H
#define __VOC_BSPLINE_CONTROLPOINT_H

#include "vop_base.h"

#include <memory>

class TDVecBSPLine;

class TDVOCBSPLineControlPtExtVar : public TDVOPExternalVariables {
public:
    TDVOCBSPLineControlPtExtVar(TDViewOperation* pParentOperation);
    TDVOCBSPLineControlPtExtVar(const TDVOCBSPLineControlPtExtVar&);
    ~TDVOCBSPLineControlPtExtVar() override = default;
    TDVOCBSPLineControlPtExtVar& operator=(const TDVOCBSPLineControlPtExtVar&);
    TDVOCBSPLineControlPtExtVar* Clone() const override;

    void SetDegree(int nDegree);
    unsigned int GetDegree() const;
    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;

private:
    TDVOCBSPLineControlPtExtVar();

    unsigned int mnDegree;
    unsigned int mnResolution;
    bool mbShowControls;
    bool mbShowPolygon;
};

class TDVOCBSPLineControlPoint : public TDVOCreate {
public:
    TDVOCBSPLineControlPoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCBSPLineControlPoint(const TDVOCBSPLineControlPoint&);
    ~TDVOCBSPLineControlPoint() override;
    TDVOCBSPLineControlPoint& operator=(const TDVOCBSPLineControlPoint&);
    TDVOCBSPLineControlPoint* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDVOCBSPLineControlPoint();
    void Finish();

    TDVOCBSPLineControlPtExtVar* mpVOCBSPLineControlPtExtVar;
    std::unique_ptr<TDVecBSPLine> mpObjBSPLine;
    std::unique_ptr<TDVecBSPLine> mpTmpObjBSPLine;
    TDMatPointsArray mMatPoints;
    int mnDegree;
    bool mbShowPolygon;
    bool mbShowControls;
    unsigned int mnResolution;
};

#endif
