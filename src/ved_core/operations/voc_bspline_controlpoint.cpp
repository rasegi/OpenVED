//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-BSPLine-ControlPoint
//---------------------------------------------------------------------------
#define __VOC_BSPLINE_CONTROLPOINT_CPP

#include "voc_bspline_controlpoint.h"

#include "vec_bspline.h"
#include "vec_edit_cad.h"
#include "vec_model.h"

TDVOCBSPLineControlPoint::TDVOCBSPLineControlPoint(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOCBSPLineControlPtExtVar(nullptr),
      mpObjBSPLine(std::make_unique<TDVecBSPLine>()),
      mpTmpObjBSPLine(nullptr),
      mMatPoints(),
      mnDegree(2),
      mbShowPolygon(true),
      mbShowControls(true),
      mnResolution(50) {
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
    IniInteractivePoints();

    mpVOPExternalVariables = new TDVOCBSPLineControlPtExtVar(this);
    mpVOCBSPLineControlPtExtVar = dynamic_cast<TDVOCBSPLineControlPtExtVar*>(mpVOPExternalVariables);
}

TDVOCBSPLineControlPoint::~TDVOCBSPLineControlPoint() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOCBSPLineControlPtExtVar = nullptr;
}

TDVOCBSPLineControlPoint::TDVOCBSPLineControlPoint(const TDVOCBSPLineControlPoint& oldOperation)
    : TDVOCreate(oldOperation),
      mpVOCBSPLineControlPtExtVar(nullptr),
      mpObjBSPLine(oldOperation.mpObjBSPLine ? std::make_unique<TDVecBSPLine>(*oldOperation.mpObjBSPLine) : nullptr),
      mpTmpObjBSPLine(oldOperation.mpTmpObjBSPLine ? std::make_unique<TDVecBSPLine>(*oldOperation.mpTmpObjBSPLine) : nullptr),
      mMatPoints(oldOperation.mMatPoints),
      mnDegree(oldOperation.mnDegree),
      mbShowPolygon(oldOperation.mbShowPolygon),
      mbShowControls(oldOperation.mbShowControls),
      mnResolution(oldOperation.mnResolution) {
    mpVOPExternalVariables = oldOperation.mpVOCBSPLineControlPtExtVar
        ? oldOperation.mpVOCBSPLineControlPtExtVar->Clone()
        : new TDVOCBSPLineControlPtExtVar(this);
    mpVOCBSPLineControlPtExtVar = dynamic_cast<TDVOCBSPLineControlPtExtVar*>(mpVOPExternalVariables);
}

TDVOCBSPLineControlPoint& TDVOCBSPLineControlPoint::operator=(const TDVOCBSPLineControlPoint& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOCreate::operator=(oldOperation);
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = oldOperation.mpVOCBSPLineControlPtExtVar
        ? oldOperation.mpVOCBSPLineControlPtExtVar->Clone()
        : new TDVOCBSPLineControlPtExtVar(this);
    mpVOCBSPLineControlPtExtVar = dynamic_cast<TDVOCBSPLineControlPtExtVar*>(mpVOPExternalVariables);
    mpObjBSPLine = oldOperation.mpObjBSPLine ? std::make_unique<TDVecBSPLine>(*oldOperation.mpObjBSPLine) : nullptr;
    mpTmpObjBSPLine = oldOperation.mpTmpObjBSPLine ? std::make_unique<TDVecBSPLine>(*oldOperation.mpTmpObjBSPLine) : nullptr;
    mMatPoints = oldOperation.mMatPoints;
    mnDegree = oldOperation.mnDegree;
    mbShowPolygon = oldOperation.mbShowPolygon;
    mbShowControls = oldOperation.mbShowControls;
    mnResolution = oldOperation.mnResolution;
    return *this;
}

TDVOCBSPLineControlPoint* TDVOCBSPLineControlPoint::Clone() const {
    return new TDVOCBSPLineControlPoint(*this);
}

void TDVOCBSPLineControlPoint::Initialize() {
    if (mpVOCBSPLineControlPtExtVar) {
        mpVOCBSPLineControlPtExtVar->SendUpdateToOPManager();
    }
}

void TDVOCBSPLineControlPoint::ExtVarIsChanged() {
    if (!mpVOCBSPLineControlPtExtVar) {
        return;
    }
    mnDegree = static_cast<int>(mpVOCBSPLineControlPtExtVar->GetDegree());
    mnResolution = mpVOCBSPLineControlPtExtVar->GetResolution();
    mbShowControls = mpVOCBSPLineControlPtExtVar->GetShowControls();
    mbShowPolygon = mpVOCBSPLineControlPtExtVar->GetShowPolygon();
}

void TDVOCBSPLineControlPoint::Finish() {
    IniInteractivePoints();
    mpObjBSPLine.reset();
    mpTmpObjBSPLine.reset();
    mMatPoints.clear();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
    if (bDestroyed) {
        return;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_1) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
    }
}

void __fastcall TDVOCBSPLineControlPoint::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
        if (mMatPoints.empty()) {
            IniInteractivePoints();
        }
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
        }
        break;
    case OSTATE_FINISHED:
        mpObjBSPLine = std::make_unique<TDVecBSPLine>();
        mpTmpObjBSPLine.reset();
        mMatPoints.clear();
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_1);
        }
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_LEFT && Shift != VKState_KEY_SHIFT) {
        if (mClick == 0) {
            mPtBeg = {X, Y};
            mPtEnd = {X, Y};
            mMatPoints.push_back(mPtBeg);
        } else {
            mPtBeg = mPtEnd;
            mPtEnd = {X, Y};
            mMatPoints.push_back(mPtEnd);
        }
        ++mClick;
    } else if (Button == VIRTMOUSEBTM_LEFT && Shift == VKState_KEY_SHIFT && !mMatPoints.empty()) {
        const TDMatPoint firstPoint = mMatPoints.front();
        mPtBeg = firstPoint;
        mPtEnd = firstPoint;
        mMatPoints.push_back(firstPoint);
    }

    if (mMatPoints.size() > static_cast<size_t>(mnDegree) && mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_BSPLINE_2) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_BSPLINE_2);
    }
}

void __fastcall TDVOCBSPLineControlPoint::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_RIGHT && Shift != VKState_KEY_SHIFT && mMatPoints.size() > static_cast<size_t>(mnDegree)) {
        if (mMatPoints.size() > 2) {
            auto* pObjBSPLine = new TDVecBSPLine();
            pObjBSPLine->SetDegree(mnDegree);
            pObjBSPLine->SetResolution(mnResolution);

            for (const TDMatPoint& point : mMatPoints) {
                pObjBSPLine->AppendPoint(point);
                pObjBSPLine->GenerateKnot();
            }
            pObjBSPLine->SetSelect(false);
            pObjBSPLine->SetShowControls(mbShowControls);
            pObjBSPLine->SetShowPolygon(mbShowPolygon);
            IniInteractivePoints();

            pObjBSPLine->ComputeCurve();
            pObjBSPLine->SetColor(mpVecModel->GetDefaultColor());
            pObjBSPLine->Initialize();
            mpVecModel->AppendObject(pObjBSPLine);
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
    } else if (Button == VIRTMOUSEBTM_RIGHT && mMatPoints.size() <= static_cast<size_t>(mnDegree)) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
    }
}

void __fastcall TDVOCBSPLineControlPoint::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;

    mpTmpObjBSPLine = std::make_unique<TDVecBSPLine>();
    mpTmpObjBSPLine->SetDegree(mnDegree);
    mpTmpObjBSPLine->SetResolution(mnResolution);
    for (const TDMatPoint& point : mMatPoints) {
        mpTmpObjBSPLine->AppendPoint(point);
        mpTmpObjBSPLine->GenerateKnot();
    }

    mpTmpObjBSPLine->AppendPoint(mPtEnd);
    mpTmpObjBSPLine->GenerateKnot();

    mpTmpObjBSPLine->SetSelect(false);
    mpTmpObjBSPLine->SetShowControls(true);
    mpTmpObjBSPLine->SetShowPolygon(true);

    mpTmpObjBSPLine->ComputeCurve();
    mpTmpObjBSPLine->Initialize();
    mpVecEditCad->TmpAppend(mpTmpObjBSPLine.get(), true);
    mPtEnd = {X, Y};
}

void __fastcall TDVOCBSPLineControlPoint::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCBSPLineControlPoint::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOCBSPLineControlPtExtVar::TDVOCBSPLineControlPtExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mnDegree(2),
      mnResolution(50),
      mbShowControls(true),
      mbShowPolygon(true) {
}

TDVOCBSPLineControlPtExtVar::TDVOCBSPLineControlPtExtVar(const TDVOCBSPLineControlPtExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mnDegree(oldExtVar.mnDegree),
      mnResolution(oldExtVar.mnResolution),
      mbShowControls(oldExtVar.mbShowControls),
      mbShowPolygon(oldExtVar.mbShowPolygon) {
}

TDVOCBSPLineControlPtExtVar& TDVOCBSPLineControlPtExtVar::operator=(const TDVOCBSPLineControlPtExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mnDegree = oldExtVar.mnDegree;
    mnResolution = oldExtVar.mnResolution;
    mbShowControls = oldExtVar.mbShowControls;
    mbShowPolygon = oldExtVar.mbShowPolygon;
    return *this;
}

TDVOCBSPLineControlPtExtVar* TDVOCBSPLineControlPtExtVar::Clone() const {
    return new TDVOCBSPLineControlPtExtVar(*this);
}

void TDVOCBSPLineControlPtExtVar::SetDegree(int nDegree) {
    mnDegree = nDegree != 0 ? static_cast<unsigned int>(nDegree) : 2;
}

unsigned int TDVOCBSPLineControlPtExtVar::GetDegree() const {
    return mnDegree;
}

void TDVOCBSPLineControlPtExtVar::SetResolution(unsigned int nResolution) {
    mnResolution = nResolution != 0 ? nResolution : 50;
}

unsigned int TDVOCBSPLineControlPtExtVar::GetResolution() const {
    return mnResolution;
}

void TDVOCBSPLineControlPtExtVar::SetShowControls(bool bShowControls) {
    mbShowControls = bShowControls;
}

bool TDVOCBSPLineControlPtExtVar::GetShowControls() const {
    return mbShowControls;
}

void TDVOCBSPLineControlPtExtVar::SetShowPolygon(bool bShowPolygon) {
    mbShowPolygon = bShowPolygon;
}

bool TDVOCBSPLineControlPtExtVar::GetShowPolygon() const {
    return mbShowPolygon;
}
