//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Curve-Attribute
//---------------------------------------------------------------------------
#define __VOM_MODIFY_CURVE_ATTRIBUTE_CPP

#include "vom_modify_curve_attribute.h"

#include "vec_bspline.h"
#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polycurve.h"

TDVOMModifyCurveAttribute::TDVOMModifyCurveAttribute(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOMModifyCurveAttributeExtVar(nullptr),
      mpObjBSPLine(nullptr),
      mpObjPolyCurve(nullptr),
      mbChangeOK(false),
      mnDegree(0),
      mnResolution(0),
      mbShowPolygon(false),
      mbShowControls(false),
      mbCopy(false) {
    mpVOPExternalVariables = new TDVOMModifyCurveAttributeExtVar(this);
    mpVOMModifyCurveAttributeExtVar = dynamic_cast<TDVOMModifyCurveAttributeExtVar*>(mpVOPExternalVariables);
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
}

TDVOMModifyCurveAttribute::~TDVOMModifyCurveAttribute() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOMModifyCurveAttributeExtVar = nullptr;
}

TDVOMModifyCurveAttribute::TDVOMModifyCurveAttribute(const TDVOMModifyCurveAttribute& oldOperation)
    : TDVOModify(oldOperation),
      mpVOMModifyCurveAttributeExtVar(nullptr),
      mpObjBSPLine(oldOperation.mpObjBSPLine),
      mpObjPolyCurve(oldOperation.mpObjPolyCurve),
      mbChangeOK(oldOperation.mbChangeOK),
      mnDegree(oldOperation.mnDegree),
      mnResolution(oldOperation.mnResolution),
      mbShowPolygon(oldOperation.mbShowPolygon),
      mbShowControls(oldOperation.mbShowControls),
      mbCopy(oldOperation.mbCopy) {
    mpVOPExternalVariables = oldOperation.mpVOMModifyCurveAttributeExtVar
        ? oldOperation.mpVOMModifyCurveAttributeExtVar->Clone()
        : new TDVOMModifyCurveAttributeExtVar(this);
    mpVOMModifyCurveAttributeExtVar = dynamic_cast<TDVOMModifyCurveAttributeExtVar*>(mpVOPExternalVariables);
}

TDVOMModifyCurveAttribute& TDVOMModifyCurveAttribute::operator=(const TDVOMModifyCurveAttribute& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOModify::operator=(oldOperation);
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = oldOperation.mpVOMModifyCurveAttributeExtVar
        ? oldOperation.mpVOMModifyCurveAttributeExtVar->Clone()
        : new TDVOMModifyCurveAttributeExtVar(this);
    mpVOMModifyCurveAttributeExtVar = dynamic_cast<TDVOMModifyCurveAttributeExtVar*>(mpVOPExternalVariables);
    mpObjBSPLine = oldOperation.mpObjBSPLine;
    mpObjPolyCurve = oldOperation.mpObjPolyCurve;
    mbChangeOK = oldOperation.mbChangeOK;
    mnDegree = oldOperation.mnDegree;
    mnResolution = oldOperation.mnResolution;
    mbShowPolygon = oldOperation.mbShowPolygon;
    mbShowControls = oldOperation.mbShowControls;
    mbCopy = oldOperation.mbCopy;
    return *this;
}

TDVOMModifyCurveAttribute* TDVOMModifyCurveAttribute::Clone() const {
    return new TDVOMModifyCurveAttribute(*this);
}

void TDVOMModifyCurveAttribute::Initialize() {
    TDVecObject* object = mpVecModel->GetSelectedObject();
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(object)) {
        mpObjBSPLine = bspline;
        mpObjPolyCurve = nullptr;
        mpVOMModifyCurveAttributeExtVar->SetDegree(static_cast<int>(bspline->GetDegree()));
        mpVOMModifyCurveAttributeExtVar->SetResolution(bspline->GetResolution());
        mpVOMModifyCurveAttributeExtVar->SetShowControls(bspline->GetShowControls());
        mpVOMModifyCurveAttributeExtVar->SetShowPolygon(bspline->GetShowPolygon());
        mpVOMModifyCurveAttributeExtVar->SetCopyFlag(true);
        mpVOMModifyCurveAttributeExtVar->SetObjectType(bspline->GetType());
        mbChangeOK = true;
        SetState(OSTATE_STARTED);
        mpVOMModifyCurveAttributeExtVar->SendUpdateToOPManager();
    } else if (auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(object)) {
        mpObjPolyCurve = polyCurve;
        mpObjBSPLine = nullptr;
        mpVOMModifyCurveAttributeExtVar->SetDegree(0);
        mpVOMModifyCurveAttributeExtVar->SetResolution(polyCurve->GetResolution());
        mpVOMModifyCurveAttributeExtVar->SetShowControls(polyCurve->GetShowControls());
        mpVOMModifyCurveAttributeExtVar->SetShowPolygon(polyCurve->GetShowPolygon());
        mpVOMModifyCurveAttributeExtVar->SetCopyFlag(true);
        mpVOMModifyCurveAttributeExtVar->SetObjectType(polyCurve->GetType());
        mbChangeOK = true;
        SetState(OSTATE_STARTED);
        mpVOMModifyCurveAttributeExtVar->SendUpdateToOPManager();
    } else {
        mbChangeOK = false;
        SetState(OSTATE_NEEDSELECTED);
    }
}

void TDVOMModifyCurveAttribute::ExtVarIsChanged() {
    if (!mbChangeOK) {
        return;
    }

    mnDegree = mpVOMModifyCurveAttributeExtVar->GetDegree();
    mnResolution = mpVOMModifyCurveAttributeExtVar->GetResolution();
    mbShowControls = mpVOMModifyCurveAttributeExtVar->GetShowControls();
    mbShowPolygon = mpVOMModifyCurveAttributeExtVar->GetShowPolygon();
    mbCopy = mpVOMModifyCurveAttributeExtVar->GetCopyFlag();

    TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
    if (auto* bspline = dynamic_cast<TDVecBSPLine*>(selectedObject)) {
        if (mbCopy) {
            auto cloneObject = bspline->Clone();
            auto* cloneBSPLine = dynamic_cast<TDVecBSPLine*>(cloneObject.release());
            if (cloneBSPLine && mnDegree < cloneBSPLine->CountPoints()) {
                cloneBSPLine->SetSelect(false);
                cloneBSPLine->SetShowControls(mbShowControls);
                cloneBSPLine->SetShowPolygon(mbShowPolygon);
                cloneBSPLine->SetResolution(mnResolution);
                cloneBSPLine->SetDegree(mnDegree);
                cloneBSPLine->GenerateKnot();
                cloneBSPLine->Initialize();
                mpVecModel->AppendObject(cloneBSPLine);
            } else {
                delete cloneBSPLine;
            }
        } else if (mnDegree < bspline->CountPoints()) {
            bspline->SetDegree(mnDegree);
            bspline->SetResolution(mnResolution);
            bspline->SetShowControls(mbShowControls);
            bspline->SetShowPolygon(mbShowPolygon);
            bspline->GenerateKnot();
            bspline->Initialize();
            mpVecModel->MarkChanged();
        }
        mpVecEditCad->ViewsRefresh();
    } else if (auto* polyCurve = dynamic_cast<TDVecPolyCurve*>(selectedObject)) {
        if (mbCopy) {
            auto cloneObject = polyCurve->Clone();
            auto* clonePolyCurve = dynamic_cast<TDVecPolyCurve*>(cloneObject.release());
            if (clonePolyCurve) {
                clonePolyCurve->SetSelect(false);
                clonePolyCurve->SetResolution(mnResolution);
                clonePolyCurve->SetShowControls(mbShowControls);
                clonePolyCurve->SetShowPolygon(mbShowPolygon);
                clonePolyCurve->Initialize();
                mpVecModel->AppendObject(clonePolyCurve);
            }
        } else {
            polyCurve->SetResolution(mnResolution);
            polyCurve->SetShowControls(mbShowControls);
            polyCurve->SetShowPolygon(mbShowPolygon);
            polyCurve->Initialize();
            mpVecModel->MarkChanged();
        }
        mpVecEditCad->ViewsRefresh();
    }
}

void __fastcall TDVOMModifyCurveAttribute::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMModifyCurveAttribute::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMModifyCurveAttribute::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;
    (void)X;
    (void)Y;
}

void __fastcall TDVOMModifyCurveAttribute::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMModifyCurveAttribute::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOMModifyCurveAttributeExtVar::TDVOMModifyCurveAttributeExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mnDegree(0),
      mnResolution(0),
      mbShowControls(false),
      mbShowPolygon(false),
      mbCopy(false),
      meObjectType(VECOBJ_UNKNOWN) {
}

TDVOMModifyCurveAttributeExtVar::TDVOMModifyCurveAttributeExtVar(const TDVOMModifyCurveAttributeExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mnDegree(oldExtVar.mnDegree),
      mnResolution(oldExtVar.mnResolution),
      mbShowControls(oldExtVar.mbShowControls),
      mbShowPolygon(oldExtVar.mbShowPolygon),
      mbCopy(oldExtVar.mbCopy),
      meObjectType(oldExtVar.meObjectType) {
}

TDVOMModifyCurveAttributeExtVar& TDVOMModifyCurveAttributeExtVar::operator=(const TDVOMModifyCurveAttributeExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mnDegree = oldExtVar.mnDegree;
    mnResolution = oldExtVar.mnResolution;
    mbShowControls = oldExtVar.mbShowControls;
    mbShowPolygon = oldExtVar.mbShowPolygon;
    mbCopy = oldExtVar.mbCopy;
    meObjectType = oldExtVar.meObjectType;
    return *this;
}

TDVOMModifyCurveAttributeExtVar* TDVOMModifyCurveAttributeExtVar::Clone() const {
    return new TDVOMModifyCurveAttributeExtVar(*this);
}

void TDVOMModifyCurveAttributeExtVar::SetDegree(int nDegree) {
    mnDegree = nDegree;
}

int TDVOMModifyCurveAttributeExtVar::GetDegree() const {
    return mnDegree;
}

void TDVOMModifyCurveAttributeExtVar::SetResolution(unsigned int nResolution) {
    mnResolution = nResolution;
}

unsigned int TDVOMModifyCurveAttributeExtVar::GetResolution() const {
    return mnResolution;
}

void TDVOMModifyCurveAttributeExtVar::SetShowControls(bool bShowControls) {
    mbShowControls = bShowControls;
}

bool TDVOMModifyCurveAttributeExtVar::GetShowControls() const {
    return mbShowControls;
}

void TDVOMModifyCurveAttributeExtVar::SetShowPolygon(bool bShowPolygon) {
    mbShowPolygon = bShowPolygon;
}

bool TDVOMModifyCurveAttributeExtVar::GetShowPolygon() const {
    return mbShowPolygon;
}

void TDVOMModifyCurveAttributeExtVar::SetCopyFlag(bool bCopy) {
    mbCopy = bCopy;
}

bool TDVOMModifyCurveAttributeExtVar::GetCopyFlag() const {
    return mbCopy;
}

void TDVOMModifyCurveAttributeExtVar::SetObjectType(TDVecObjectType eObjectType) {
    meObjectType = eObjectType;
}

TDVecObjectType TDVOMModifyCurveAttributeExtVar::GetObjectType() const {
    return meObjectType;
}

bool TDVOMModifyCurveAttributeExtVar::ObjectTypeIsOK() const {
    return meObjectType == VECOBJ_POLYCURVE || meObjectType == VECOBJ_BSPLINE;
}
