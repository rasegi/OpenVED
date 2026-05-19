#include "voc_vectext.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <cstdio>

namespace {
void copyFontName(char* destination, const char* source) {
    std::snprintf(destination, 30, "%s", source ? source : "");
}
}

TDVOCVecText::TDVOCVecText(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mOriginPoint{},
      mpObjText(std::make_unique<TDVecText>()),
      mpVOCVecTextExtVar(nullptr) {
    IniInteractivePoints();
    SetState(OSTATE_RUNNING);

    mpVOPExternalVariables = new TDVOCVecTextExtVar(this);
    mpVOCVecTextExtVar = dynamic_cast<TDVOCVecTextExtVar*>(mpVOPExternalVariables);
    UpdateCursor();
}

TDVOCVecText::TDVOCVecText(const TDVOCVecText& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mOriginPoint(oldOperation.mOriginPoint),
      mpObjText(oldOperation.mpObjText ? std::make_unique<TDVecText>(*oldOperation.mpObjText) : std::make_unique<TDVecText>()),
      mpVOCVecTextExtVar(nullptr) {
    auto* extVar = new TDVOCVecTextExtVar(this);
    if (oldOperation.mpVOCVecTextExtVar) {
        *extVar = *oldOperation.mpVOCVecTextExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCVecTextExtVar = extVar;
}

TDVOCVecText::~TDVOCVecText() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOCVecTextExtVar = nullptr;
    if (mpVecEditCad) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }
}

TDVOCVecText& TDVOCVecText::operator=(const TDVOCVecText& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mOriginPoint = oldOperation.mOriginPoint;
    mpObjText = oldOperation.mpObjText ? std::make_unique<TDVecText>(*oldOperation.mpObjText) : std::make_unique<TDVecText>();

    delete mpVOPExternalVariables;
    auto* extVar = new TDVOCVecTextExtVar(this);
    if (oldOperation.mpVOCVecTextExtVar) {
        *extVar = *oldOperation.mpVOCVecTextExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCVecTextExtVar = extVar;
    return *this;
}

TDVOCVecText* TDVOCVecText::Clone() const {
    return new TDVOCVecText(*this);
}

void TDVOCVecText::Initialize() {
    if (mpVOCVecTextExtVar) {
        mpVOCVecTextExtVar->SendUpdateToOPManager();
    }
    UpdateCursor();
}

void TDVOCVecText::OPMouseDown(TDOPVirtMouseButton, TDOPVirtKeyState, double, double) {
}

void TDVOCVecText::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState, double X, double Y) {
    if (Button == VIRTMOUSEBTM_LEFT) {
        if (TextIsOK()) {
            mOriginPoint.x = X;
            mOriginPoint.y = Y;
            mpObjText->SetOriginPoint(mOriginPoint);

            std::unique_ptr<TDVecObject> clone = mpObjText->Clone();
            clone->SetSelect(false);
            clone->SetColor(mpVecModel->GetDefaultColor());
            mpVecModel->AppendObject(clone.release());
            mpVecEditCad->TmpClear();
            mpVecEditCad->ViewsRefresh();

            if (ISActiveChangeToDefOPAfterFinish()) {
                bool destroyed = false;
                SetState(OSTATE_FINISHED, &destroyed);
                if (destroyed) {
                    return;
                }
            }
        } else {
            mpVecEditCad->Beep();
        }
    } else if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_VECTEXT_1);
        mpVOCVecTextExtVar->DeleteText();
        mpVOCVecTextExtVar->SendUpdateToOPManager();
        mpObjText->SetText(mpVOCVecTextExtVar->msText.c_str());
        mpVecEditCad->Beep();
        mpVecEditCad->ViewsRefresh();
        mpVecEditCad->TmpAppend(mpObjText.get(), true);
    }
}

void TDVOCVecText::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState, double X, double Y) {
    mOriginPoint.x = X;
    mOriginPoint.y = Y;
    mpObjText->SetOriginPoint(mOriginPoint);
    if (TextIsOK() && Button != VIRTMOUSEBTM_RIGHT && Button != VIRTMOUSEBTM_LEFT) {
        mpVecEditCad->TmpAppend(mpObjText.get(), true);
    }
}

void TDVOCVecText::OPKeyDown(TDOPVirtKey, TDOPVirtKeyState) {
}

void TDVOCVecText::OPKeyUp(TDOPVirtKey, TDOPVirtKeyState) {
}

void TDVOCVecText::SetObjText() {
    if (!mpVOCVecTextExtVar || !mpObjText) {
        return;
    }

    TDVOCVecTextParameter_s parameter;
    mpVOCVecTextExtVar->GetParameter(&parameter);
    mpObjText->SetOriginPoint(mOriginPoint);
    mpObjText->SetParameter(&parameter);
    mpObjText->SetText(mpVOCVecTextExtVar->msText.c_str());
    UpdateCursor();
}

bool TDVOCVecText::TextIsOK() const {
    return mpVOCVecTextExtVar && mpVOCVecTextExtVar->TextIsOK();
}

void TDVOCVecText::UpdateCursor() {
    if (!mpVecEditCad) {
        return;
    }
    mpVecEditCad->UseCursor(TextIsOK() ? VECVIEW_CREATE_VECTEXT_2 : VECVIEW_CREATE_VECTEXT_1);
}

void TDVOCVecText::ExtVarIsChanged() {
    UpdateCursor();
    SetObjText();
    if (mpVecEditCad && mpObjText) {
        mpVecEditCad->TmpAppend(mpObjText.get(), true);
    }
}

TDVOCVecTextExtVar::TDVOCVecTextExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mbTextIsDeleted(false),
      mnXScale(1.0),
      mnYScale(1.0),
      mnAngle(0.0),
      mnIncline(0.0),
      mnLineSpacing(1.0),
      mnCharSpacing(1.0),
      mbVertical(false),
      mbUnderline(false),
      meJustification(TDVecText::JUST_LEFT),
      meVerticalAlignment(TDVecText::VALIGN_TOP),
      msFontName{},
      mpVecFont(GetDefaultVecTextFont()),
      msText(),
      mnResolution(4) {
    if (mpVecFont) {
        copyFontName(msFontName, mpVecFont->GetFontName());
    }
}

TDVOCVecTextExtVar::TDVOCVecTextExtVar(const TDVOCVecTextExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mbTextIsDeleted(oldExtVar.mbTextIsDeleted),
      mnXScale(oldExtVar.mnXScale),
      mnYScale(oldExtVar.mnYScale),
      mnAngle(oldExtVar.mnAngle),
      mnIncline(oldExtVar.mnIncline),
      mnLineSpacing(oldExtVar.mnLineSpacing),
      mnCharSpacing(oldExtVar.mnCharSpacing),
      mbVertical(oldExtVar.mbVertical),
      mbUnderline(oldExtVar.mbUnderline),
      meJustification(oldExtVar.meJustification),
      meVerticalAlignment(oldExtVar.meVerticalAlignment),
      msFontName{},
      mpVecFont(oldExtVar.mpVecFont),
      msText(oldExtVar.msText),
      mnResolution(oldExtVar.mnResolution) {
    copyFontName(msFontName, oldExtVar.msFontName);
}

TDVOCVecTextExtVar& TDVOCVecTextExtVar::operator=(const TDVOCVecTextExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mbTextIsDeleted = oldExtVar.mbTextIsDeleted;
    mnXScale = oldExtVar.mnXScale;
    mnYScale = oldExtVar.mnYScale;
    mnAngle = oldExtVar.mnAngle;
    mnIncline = oldExtVar.mnIncline;
    mnLineSpacing = oldExtVar.mnLineSpacing;
    mnCharSpacing = oldExtVar.mnCharSpacing;
    mbVertical = oldExtVar.mbVertical;
    mbUnderline = oldExtVar.mbUnderline;
    meJustification = oldExtVar.meJustification;
    meVerticalAlignment = oldExtVar.meVerticalAlignment;
    mnResolution = oldExtVar.mnResolution;
    mpVecFont = oldExtVar.mpVecFont;
    copyFontName(msFontName, oldExtVar.msFontName);
    msText = oldExtVar.msText;
    return *this;
}

TDVOCVecTextExtVar* TDVOCVecTextExtVar::Clone() const {
    return new TDVOCVecTextExtVar(*this);
}

void TDVOCVecTextExtVar::SetParameter(const TDVOCVecTextParameter_s* pVOCVecTextParameter_s) {
    if (!pVOCVecTextParameter_s) {
        return;
    }
    mnXScale = pVOCVecTextParameter_s->mnXScale;
    mnYScale = pVOCVecTextParameter_s->mnYScale;
    mnAngle = pVOCVecTextParameter_s->mnAngle;
    mnIncline = pVOCVecTextParameter_s->mnIncline;
    mnLineSpacing = pVOCVecTextParameter_s->mnLineSpacing;
    mnCharSpacing = pVOCVecTextParameter_s->mnCharSpacing;
    mbVertical = pVOCVecTextParameter_s->mbVertical;
    mbUnderline = pVOCVecTextParameter_s->mbUnderline;
    meJustification = pVOCVecTextParameter_s->meJustification;
    meVerticalAlignment = pVOCVecTextParameter_s->meVerticalAlignment;
    mnResolution = pVOCVecTextParameter_s->mnResolution;
    mpVecFont = pVOCVecTextParameter_s->mpVecFont;
    copyFontName(msFontName, pVOCVecTextParameter_s->msFontName);
}

void TDVOCVecTextExtVar::GetParameter(TDVOCVecTextParameter_s* pVOCVecTextParameter_s) const {
    if (!pVOCVecTextParameter_s) {
        return;
    }
    pVOCVecTextParameter_s->mnXScale = mnXScale;
    pVOCVecTextParameter_s->mnYScale = mnYScale;
    pVOCVecTextParameter_s->mnAngle = mnAngle;
    pVOCVecTextParameter_s->mnIncline = mnIncline;
    pVOCVecTextParameter_s->mnLineSpacing = mnLineSpacing;
    pVOCVecTextParameter_s->mnCharSpacing = mnCharSpacing;
    pVOCVecTextParameter_s->mbVertical = mbVertical;
    pVOCVecTextParameter_s->mbUnderline = mbUnderline;
    pVOCVecTextParameter_s->meJustification = meJustification;
    pVOCVecTextParameter_s->meVerticalAlignment = meVerticalAlignment;
    pVOCVecTextParameter_s->mnResolution = mnResolution;
    pVOCVecTextParameter_s->mpVecFont = mpVecFont;
    copyFontName(pVOCVecTextParameter_s->msFontName, msFontName);
}

void TDVOCVecTextExtVar::SetText(const char* pText) {
    msText = pText ? pText : "";
    mbTextIsDeleted = false;
}

const char* TDVOCVecTextExtVar::GetText() const {
    return msText.empty() ? nullptr : msText.c_str();
}

void TDVOCVecTextExtVar::DeleteText() {
    msText.clear();
    mbTextIsDeleted = true;
}

bool TDVOCVecTextExtVar::TextIsOK() const {
    return !msText.empty() && !IsStringBlank(msText.c_str());
}

bool TDVOCVecTextExtVar::TextIsDeleted() const {
    return mbTextIsDeleted;
}
