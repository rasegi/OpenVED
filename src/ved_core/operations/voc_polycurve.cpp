//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-PolyCurve
//---------------------------------------------------------------------------
#define __VOC_POLYCURVE_CPP

#include "voc_polycurve.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polycurve.h"

TDVOCPolyCurve::TDVOCPolyCurve(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpVOCPolyCurveExtVar(nullptr),
      mpTmpObject(nullptr),
      mMatLine{},
      mConturPoints(),
      mbPolyCurveOK(false),
      mePreviusPointType(CPT_PRIME_LINE) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_1);

    mpVOPExternalVariables = new TDVOCPolyCurveExtVar(this);
    mpVOCPolyCurveExtVar = dynamic_cast<TDVOCPolyCurveExtVar*>(mpVOPExternalVariables);
}

TDVOCPolyCurve::~TDVOCPolyCurve() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOCPolyCurveExtVar = nullptr;
}

TDVOCPolyCurve::TDVOCPolyCurve(const TDVOCPolyCurve& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mpVOCPolyCurveExtVar(nullptr),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mMatLine(oldOperation.mMatLine),
      mConturPoints(oldOperation.mConturPoints),
      mbPolyCurveOK(oldOperation.mbPolyCurveOK),
      mePreviusPointType(oldOperation.mePreviusPointType) {
    auto* extVar = new TDVOCPolyCurveExtVar(this);
    if (oldOperation.mpVOCPolyCurveExtVar) {
        extVar->SetResolution(oldOperation.mpVOCPolyCurveExtVar->GetResolution());
        extVar->SetShowControls(oldOperation.mpVOCPolyCurveExtVar->GetShowControls());
        extVar->SetShowPolygon(oldOperation.mpVOCPolyCurveExtVar->GetShowPolygon());
    }
    mpVOPExternalVariables = extVar;
    mpVOCPolyCurveExtVar = dynamic_cast<TDVOCPolyCurveExtVar*>(mpVOPExternalVariables);
}

TDVOCPolyCurve& TDVOCPolyCurve::operator=(const TDVOCPolyCurve& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    delete mpVOPExternalVariables;
    auto* extVar = new TDVOCPolyCurveExtVar(this);
    if (oldOperation.mpVOCPolyCurveExtVar) {
        extVar->SetResolution(oldOperation.mpVOCPolyCurveExtVar->GetResolution());
        extVar->SetShowControls(oldOperation.mpVOCPolyCurveExtVar->GetShowControls());
        extVar->SetShowPolygon(oldOperation.mpVOCPolyCurveExtVar->GetShowPolygon());
    }
    mpVOPExternalVariables = extVar;
    mpVOCPolyCurveExtVar = dynamic_cast<TDVOCPolyCurveExtVar*>(mpVOPExternalVariables);
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mMatLine = oldOperation.mMatLine;
    mConturPoints = oldOperation.mConturPoints;
    mbPolyCurveOK = oldOperation.mbPolyCurveOK;
    mePreviusPointType = oldOperation.mePreviusPointType;
    return *this;
}

TDVOCPolyCurve* TDVOCPolyCurve::Clone() const {
    return new TDVOCPolyCurve(*this);
}

void TDVOCPolyCurve::Initialize() {
    if (mpVOCPolyCurveExtVar) {
        mpVOCPolyCurveExtVar->SendUpdateToOPManager();
    }
}

void TDVOCPolyCurve::ExtVarIsChanged() {
}

std::unique_ptr<TDVecPolyCurve> TDVOCPolyCurve::CreatePolyCurveFromPoints() const {
    auto polyCurve = std::make_unique<TDVecPolyCurve>();
    for (const TDMatConturPoint& point : mConturPoints) {
        polyCurve->AppendPoint(point);
    }
    if (mpVOCPolyCurveExtVar) {
        polyCurve->SetResolution(mpVOCPolyCurveExtVar->GetResolution());
        polyCurve->SetShowPolygon(mpVOCPolyCurveExtVar->GetShowPolygon());
        polyCurve->SetShowControls(mpVOCPolyCurveExtVar->GetShowControls());
    }
    return polyCurve;
}

void TDVOCPolyCurve::Finish() {
    IniInteractivePoints();
    mConturPoints.clear();
    mbPolyCurveOK = false;
    mpTmpObject.reset();

    bool bDestroyed = false;
    SetState(OSTATE_FINISHED, &bDestroyed);
    if (bDestroyed) {
        return;
    }

    if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYCURVE_SMARTLINE_1) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_1);
    }
}

void __fastcall TDVOCPolyCurve::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
        IniInteractivePoints();
        mbPolyCurveOK = false;
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYCURVE_SMARTLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_1);
        }
        break;
    case OSTATE_FINISHED:
        IniInteractivePoints();
        mConturPoints.clear();
        mbPolyCurveOK = false;
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYCURVE_SMARTLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_1);
        }
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_LEFT && !(Shift & VKState_KEY_CTRL)) {
        TDMatConturPoint conturPoint{X, Y, CPT_PRIME_LINE, true};
        if (mClick == 0) {
            mPtBeg = {X, Y};
            mPtEnd = {X, Y};
        } else {
            mPtBeg = mPtEnd;
            mPtEnd = {X, Y};
        }
        mePreviusPointType = CPT_PRIME_LINE;
        mConturPoints.push_back(conturPoint);
        mbPolyCurveOK = true;
        ++mClick;
    } else if (Button == VIRTMOUSEBTM_LEFT && (Shift & VKState_KEY_CTRL)) {
        if (mClick > 0 && !mConturPoints.empty() && mConturPoints.back().eType == CPT_PRIME_LINE) {
            mPtBeg = mPtEnd;
            mPtEnd = {X, Y};
            TDMatConturPoint conturPoint{X, Y, CPT_PRIME_QSPLINE, true};
            mePreviusPointType = CPT_PRIME_QSPLINE;
            mConturPoints.push_back(conturPoint);
            mbPolyCurveOK = true;
        } else {
            mpVecEditCad->Beep();
            mbPolyCurveOK = false;
        }
    }

    if (mbPolyCurveOK && mConturPoints.size() > 1 && mePreviusPointType != CPT_PRIME_QSPLINE) {
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYCURVE_SMARTLINE_2) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_2);
        }
    } else if (mePreviusPointType == CPT_PRIME_QSPLINE) {
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_POLYCURVE_SMARTLINE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_POLYCURVE_SMARTLINE_1);
        }
    }
}

void __fastcall TDVOCPolyCurve::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)X;
    (void)Y;

    if (!mbPolyCurveOK || mConturPoints.empty()) {
        return;
    }

    if (Button == VIRTMOUSEBTM_RIGHT && !(Shift & VKState_KEY_SHIFT)) {
        if (mConturPoints.back().eType == CPT_PRIME_LINE) {
            if (mConturPoints.size() > 1) {
                TDVecPolyCurve* polyCurve = CreatePolyCurveFromPoints().release();
                polyCurve->SetSelect(false);
                polyCurve->SetColor(mpVecModel->GetDefaultColor());
                polyCurve->Initialize();
                mpVecModel->AppendObject(polyCurve);
                IniInteractivePoints();
            }
            mpVecEditCad->TmpClear();
            mpVecEditCad->ViewsRefresh();
            Finish();
        } else {
            mpVecEditCad->Beep();
        }
        return;
    }

    if (Button == VIRTMOUSEBTM_RIGHT && (Shift & VKState_KEY_SHIFT)) {
        if (mConturPoints.size() > 1) {
            TDVecPolyCurve* polyCurve = CreatePolyCurveFromPoints().release();
            const TDMatConturPoint firstPoint = mConturPoints.front();
            polyCurve->AppendPoint({firstPoint.x, firstPoint.y, CPT_PRIME_LINE, true});
            polyCurve->SetSelect(false);
            polyCurve->SetColor(mpVecModel->GetDefaultColor());
            polyCurve->Initialize();
            mpVecModel->AppendObject(polyCurve);
            IniInteractivePoints();
        }
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        Finish();
    }
}

void __fastcall TDVOCPolyCurve::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        auto tmpPolyCurve = CreatePolyCurveFromPoints();
        tmpPolyCurve->AppendPoint({mPtEnd.x, mPtEnd.y, CPT_PRIME_LINE, true});
        tmpPolyCurve->SetSelect(false);
        mpTmpObject = std::move(tmpPolyCurve);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
        mPtEnd = {X, Y};
    }
}

void __fastcall TDVOCPolyCurve::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCPolyCurve::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOCPolyCurveExtVar::TDVOCPolyCurveExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mnResolution(5),
      mbShowControls(true),
      mbShowPolygon(true) {
}

TDVOCPolyCurveExtVar::TDVOCPolyCurveExtVar(const TDVOCPolyCurveExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mnResolution(oldExtVar.mnResolution),
      mbShowControls(oldExtVar.mbShowControls),
      mbShowPolygon(oldExtVar.mbShowPolygon) {
}

TDVOCPolyCurveExtVar& TDVOCPolyCurveExtVar::operator=(const TDVOCPolyCurveExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mnResolution = oldExtVar.mnResolution;
    mbShowControls = oldExtVar.mbShowControls;
    mbShowPolygon = oldExtVar.mbShowPolygon;
    return *this;
}

TDVOCPolyCurveExtVar* TDVOCPolyCurveExtVar::Clone() const {
    return new TDVOCPolyCurveExtVar(*this);
}

void TDVOCPolyCurveExtVar::SetResolution(unsigned int nResolution) {
    mnResolution = nResolution;
}

unsigned int TDVOCPolyCurveExtVar::GetResolution() const {
    return mnResolution;
}

void TDVOCPolyCurveExtVar::SetShowControls(bool bShowControls) {
    mbShowControls = bShowControls;
}

bool TDVOCPolyCurveExtVar::GetShowControls() const {
    return mbShowControls;
}

void TDVOCPolyCurveExtVar::SetShowPolygon(bool bShowPolygon) {
    mbShowPolygon = bShowPolygon;
}

bool TDVOCPolyCurveExtVar::GetShowPolygon() const {
    return mbShowPolygon;
}
