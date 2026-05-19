//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Curve-Attribute
//---------------------------------------------------------------------------
#ifndef __VOM_MODIFY_CURVE_ATTRIBUTE_H
#define __VOM_MODIFY_CURVE_ATTRIBUTE_H

#include "vop_base.h"

class TDVecBSPLine;
class TDVecPolyCurve;

class TDVOMModifyCurveAttributeExtVar : public TDVOPExternalVariables {
public:
    TDVOMModifyCurveAttributeExtVar(TDViewOperation* pParentOperation);
    TDVOMModifyCurveAttributeExtVar(const TDVOMModifyCurveAttributeExtVar&);
    ~TDVOMModifyCurveAttributeExtVar() override = default;
    TDVOMModifyCurveAttributeExtVar& operator=(const TDVOMModifyCurveAttributeExtVar&);
    TDVOMModifyCurveAttributeExtVar* Clone() const override;

    void SetDegree(int nDegree);
    int GetDegree() const;
    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;
    void SetCopyFlag(bool bCopy);
    bool GetCopyFlag() const;
    TDVecObjectType GetObjectType() const;
    bool ObjectTypeIsOK() const;

protected:
    friend class TDVOMModifyCurveAttribute;
    void SetObjectType(TDVecObjectType eObjectType);

private:
    TDVOMModifyCurveAttributeExtVar();

    int mnDegree;
    unsigned int mnResolution;
    bool mbShowControls;
    bool mbShowPolygon;
    bool mbCopy;
    TDVecObjectType meObjectType;
};

class TDVOMModifyCurveAttribute : public TDVOModify {
public:
    TDVOMModifyCurveAttribute(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMModifyCurveAttribute(const TDVOMModifyCurveAttribute&);
    ~TDVOMModifyCurveAttribute() override;
    TDVOMModifyCurveAttribute& operator=(const TDVOMModifyCurveAttribute&);
    TDVOMModifyCurveAttribute* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDVOMModifyCurveAttribute();

    TDVOMModifyCurveAttributeExtVar* mpVOMModifyCurveAttributeExtVar;
    TDVecBSPLine* mpObjBSPLine;
    TDVecPolyCurve* mpObjPolyCurve;
    bool mbChangeOK;
    int mnDegree;
    unsigned int mnResolution;
    bool mbShowPolygon;
    bool mbShowControls;
    bool mbCopy;
};

#endif
