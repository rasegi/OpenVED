#include "vom_vectext.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <cstdio>
#include <vector>

namespace {
void copyFontName(char* destination, const char* source) {
    std::snprintf(destination, 30, "%s", source ? source : "");
}

bool isTextObjectType(TDVecObjectType type) {
    return type == VECOBJ_TEX || type == VECOBJ_FRAMETEXT;
}

bool selectedObjectsAreSameTextType(TDVecModel* model, TDVecObjectType* objectType) {
    if (!model || model->CountSelectedObjects() <= 0) {
        return false;
    }

    TDVecObjectType selectedType = VECOBJ_UNKNOWN;
    for (TDVecObject* object : model->GetSelectedObjects()) {
        if (!object || !isTextObjectType(object->GetType())) {
            return false;
        }
        if (selectedType == VECOBJ_UNKNOWN) {
            selectedType = object->GetType();
        } else if (selectedType != object->GetType()) {
            return false;
        }
    }

    if (objectType) {
        *objectType = selectedType;
    }
    return selectedType != VECOBJ_UNKNOWN;
}

void compareTextWithExtVar(TDVOMVecTextExtVar* extVar, const TDVecText* text) {
    if (!extVar || !text) {
        return;
    }
    TDVOMVecTextParameter_s parameter;
    TDVecTextParameterValidity validity;
    extVar->GetParameter(&parameter);
    extVar->GetParameterValidity(&validity);

    if (parameter.mnXScale != text->GetXScale()) {
        validity.mbXScale = false;
    }
    if (parameter.mnYScale != text->GetYScale()) {
        validity.mbYScale = false;
    }
    if (parameter.mnAngle != text->GetAngle()) {
        validity.mbAngle = false;
    }
    if (parameter.mnIncline != text->GetIncline()) {
        validity.mbIncline = false;
    }
    if (parameter.mnLineSpacing != text->GetLineSpacing()) {
        validity.mbLineSpacing = false;
    }
    if (parameter.mnCharSpacing != text->GetCharSpacing()) {
        validity.mbCharSpacing = false;
    }
    if (parameter.mbVertical != text->GetVertical()) {
        validity.mbVertical = false;
    }
    if (parameter.mbUnderline != text->GetUnderline()) {
        validity.mbUnderline = false;
    }
    if (parameter.meJustification != text->GetJustification()) {
        validity.mbJustification = false;
    }
    if (parameter.meVerticalAlignment != text->GetVerticalAlignment()) {
        validity.mbVerticalAlignment = false;
    }
    if (parameter.mnResolution != text->GetResolution()) {
        validity.mbResolution = false;
    }
    if (parameter.mpVecFont != text->GetVecFontPointer()) {
        validity.mbVecFont = false;
    }
    if (parameter.mbScaleDependency != text->GetScaleDependency()) {
        validity.mbScaleDependency = false;
    }
    extVar->SetParameterValidity(&validity);
}
}

TDVOMVecText::TDVOMVecText(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      mOriginPoint{},
      mpVOMVecTextExtVar(nullptr) {
    mpVOPExternalVariables = new TDVOMVecTextExtVar(this);
    mpVOMVecTextExtVar = dynamic_cast<TDVOMVecTextExtVar*>(mpVOPExternalVariables);
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
}

TDVOMVecText::TDVOMVecText(const TDVOMVecText& oldOperation)
    : TDVOModify(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mOriginPoint(oldOperation.mOriginPoint),
      mpVOMVecTextExtVar(nullptr) {
    auto* extVar = new TDVOMVecTextExtVar(this);
    if (oldOperation.mpVOMVecTextExtVar) {
        *extVar = *oldOperation.mpVOMVecTextExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOMVecTextExtVar = extVar;
}

TDVOMVecText::~TDVOMVecText() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOMVecTextExtVar = nullptr;
    if (mpVecEditCad) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    }
}

TDVOMVecText& TDVOMVecText::operator=(const TDVOMVecText& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mOriginPoint = oldOperation.mOriginPoint;

    delete mpVOPExternalVariables;
    auto* extVar = new TDVOMVecTextExtVar(this);
    if (oldOperation.mpVOMVecTextExtVar) {
        *extVar = *oldOperation.mpVOMVecTextExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOMVecTextExtVar = extVar;
    return *this;
}

TDVOMVecText* TDVOMVecText::Clone() const {
    return new TDVOMVecText(*this);
}

void TDVOMVecText::Initialize() {
    if (!mpVecModel || !mpVOMVecTextExtVar) {
        SetState(OSTATE_NEEDSELECTED);
        return;
    }

    const int selectedCount = mpVecModel->CountSelectedObjects();
    if (selectedCount == 1) {
        TDVecObject* selectedObject = mpVecModel->GetSelectedObject();
        if (!selectedObject || !isTextObjectType(selectedObject->GetType())) {
            SetState(OSTATE_NEEDSELECTED);
            mpVOMVecTextExtVar->SendUpdateToOPManager();
            return;
        }

        auto* text = dynamic_cast<TDVecText*>(selectedObject);
        if (!text) {
            SetState(OSTATE_NEEDSELECTED);
            mpVOMVecTextExtVar->SendUpdateToOPManager();
            return;
        }

        IniInteractivePoints();
        SetState(OSTATE_RUNNING);

        TDVOMVecTextParameter_s parameter;
        text->GetParameter(&parameter);
        mpVOMVecTextExtVar->mObjectType = selectedObject->GetType();
        mpVOMVecTextExtVar->SetParameter(&parameter);
        if (!mpVOMVecTextExtVar->mpVecFont) {
            mpVOMVecTextExtVar->mpVecFont = GetDefaultVecTextFont();
        }
        copyFontName(mpVOMVecTextExtVar->msFontName, text->GetFontName());
        mpVOMVecTextExtVar->SetText(text->GetText());
        mpVOMVecTextExtVar->SetAllParameterValidity(true);
    } else if (selectedCount > 1) {
        TDVecObjectType objectType = VECOBJ_UNKNOWN;
        if (!selectedObjectsAreSameTextType(mpVecModel, &objectType)) {
            SetState(OSTATE_NEEDSELECTED);
            mpVOMVecTextExtVar->SendUpdateToOPManager();
            return;
        }

        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        mpVOMVecTextExtVar->mObjectType = objectType;
        mpVOMVecTextExtVar->SetAllParameterValidity(true);

        const std::vector<TDVecObject*> selectedObjects = mpVecModel->GetSelectedObjects();
        auto* firstText = dynamic_cast<TDVecText*>(selectedObjects.front());
        if (firstText) {
            TDVOMVecTextParameter_s parameter;
            firstText->GetParameter(&parameter);
            mpVOMVecTextExtVar->SetParameter(&parameter);
            if (!mpVOMVecTextExtVar->mpVecFont) {
                mpVOMVecTextExtVar->mpVecFont = GetDefaultVecTextFont();
            }
            copyFontName(mpVOMVecTextExtVar->msFontName, firstText->GetFontName());
        }

        for (std::size_t i = 1; i < selectedObjects.size(); ++i) {
            compareTextWithExtVar(mpVOMVecTextExtVar, dynamic_cast<TDVecText*>(selectedObjects[i]));
        }
        mpVOMVecTextExtVar->SetText("");
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }

    mpVOMVecTextExtVar->SendUpdateToOPManager();
}

void TDVOMVecText::ExtVarIsChanged() {
    if (!mpVecModel || !mpVecEditCad || !mpVOMVecTextExtVar) {
        return;
    }

    const int selectedCount = mpVecModel->CountSelectedObjects();
    if (selectedCount == 1) {
        auto* text = dynamic_cast<TDVecText*>(mpVecModel->GetSelectedObject());
        if (!text || !isTextObjectType(text->GetType())) {
            SetState(OSTATE_NEEDSELECTED);
            return;
        }

        if (!TextIsOK()) {
            mpVecEditCad->Beep();
            return;
        }

        TDVecText::TDVecTextParameter parameter;
        mpVOMVecTextExtVar->GetParameter(&parameter);
        text->SetParameter(&parameter);
        text->SetText(mpVOMVecTextExtVar->GetText());
        text->Initialize();
        mpVecModel->MarkChanged();
        Initialize();
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        return;
    }

    if (selectedCount > 1) {
        TDVecObjectType objectType = VECOBJ_UNKNOWN;
        if (!selectedObjectsAreSameTextType(mpVecModel, &objectType)) {
            SetState(OSTATE_NEEDSELECTED);
            return;
        }

        for (TDVecObject* object : mpVecModel->GetSelectedObjects()) {
            auto* text = dynamic_cast<TDVecText*>(object);
            if (!text) {
                continue;
            }

            TDVecText::TDVecTextParameter parameter;
            text->GetParameter(&parameter);
            if (mpVOMVecTextExtVar->mParamValidity.mbXScale) {
                parameter.mnXScale = mpVOMVecTextExtVar->mnXScale;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbYScale) {
                parameter.mnYScale = mpVOMVecTextExtVar->mnYScale;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbAngle) {
                parameter.mnAngle = mpVOMVecTextExtVar->mnAngle;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbIncline) {
                parameter.mnIncline = mpVOMVecTextExtVar->mnIncline;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbLineSpacing) {
                parameter.mnLineSpacing = mpVOMVecTextExtVar->mnLineSpacing;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbCharSpacing) {
                parameter.mnCharSpacing = mpVOMVecTextExtVar->mnCharSpacing;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbVertical) {
                parameter.mbVertical = mpVOMVecTextExtVar->mbVertical;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbUnderline) {
                parameter.mbUnderline = mpVOMVecTextExtVar->mbUnderline;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbVecFont) {
                parameter.mpVecFont = mpVOMVecTextExtVar->mpVecFont;
                copyFontName(parameter.msFontName, mpVOMVecTextExtVar->msFontName);
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbJustification) {
                parameter.meJustification = mpVOMVecTextExtVar->meJustification;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbVerticalAlignment) {
                parameter.meVerticalAlignment = mpVOMVecTextExtVar->meVerticalAlignment;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbResolution) {
                parameter.mnResolution = mpVOMVecTextExtVar->mnResolution;
            }
            if (mpVOMVecTextExtVar->mParamValidity.mbScaleDependency) {
                parameter.mbScaleDependency = mpVOMVecTextExtVar->mbScaleDependency;
            }

            text->SetParameter(&parameter);
            text->Initialize();
        }

        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
    } else {
        SetState(OSTATE_NEEDSELECTED);
    }
}

void TDVOMVecText::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState, double, double) {
    if (Button == VIRTMOUSEBTM_RIGHT) {
        SetState(OSTATE_NEEDSELECTED);
    }
}

void TDVOMVecText::OPMouseUp(TDOPVirtMouseButton, TDOPVirtKeyState, double, double) {
}

void TDVOMVecText::OPMouseMove(TDOPVirtMouseButton, TDOPVirtKeyState, double, double) {
}

void TDVOMVecText::OPKeyDown(TDOPVirtKey, TDOPVirtKeyState) {
}

void TDVOMVecText::OPKeyUp(TDOPVirtKey, TDOPVirtKeyState) {
}

bool TDVOMVecText::TextIsOK() const {
    return mpVOMVecTextExtVar && mpVOMVecTextExtVar->TextIsOK();
}

TDVOMVecTextExtVar::TDVOMVecTextExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mParamValidity{},
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
      mnResolution(4),
      msFontName{},
      mpVecFont(nullptr),
      mbScaleDependency(true),
      msText(),
      mbHasText(false),
      mObjectType(VECOBJ_UNKNOWN) {
}

TDVOMVecTextExtVar::TDVOMVecTextExtVar(const TDVOMVecTextExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mParamValidity(oldExtVar.mParamValidity),
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
      mnResolution(oldExtVar.mnResolution),
      msFontName{},
      mpVecFont(oldExtVar.mpVecFont),
      mbScaleDependency(oldExtVar.mbScaleDependency),
      msText(oldExtVar.msText),
      mbHasText(oldExtVar.mbHasText),
      mObjectType(oldExtVar.mObjectType) {
    copyFontName(msFontName, oldExtVar.msFontName);
}

TDVOMVecTextExtVar& TDVOMVecTextExtVar::operator=(const TDVOMVecTextExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }
    TDVOPExternalVariables::operator=(oldExtVar);
    mParamValidity = oldExtVar.mParamValidity;
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
    mbScaleDependency = oldExtVar.mbScaleDependency;
    copyFontName(msFontName, oldExtVar.msFontName);
    msText = oldExtVar.msText;
    mbHasText = oldExtVar.mbHasText;
    mObjectType = oldExtVar.mObjectType;
    return *this;
}

TDVOMVecTextExtVar* TDVOMVecTextExtVar::Clone() const {
    return new TDVOMVecTextExtVar(*this);
}

void TDVOMVecTextExtVar::SetParameter(const TDVOMVecTextParameter_s* pVOMVecTextParameter_s) {
    if (!pVOMVecTextParameter_s) {
        return;
    }
    mnXScale = pVOMVecTextParameter_s->mnXScale;
    mnYScale = pVOMVecTextParameter_s->mnYScale;
    mnAngle = pVOMVecTextParameter_s->mnAngle;
    mnIncline = pVOMVecTextParameter_s->mnIncline;
    mnLineSpacing = pVOMVecTextParameter_s->mnLineSpacing;
    mnCharSpacing = pVOMVecTextParameter_s->mnCharSpacing;
    mbVertical = pVOMVecTextParameter_s->mbVertical;
    mbUnderline = pVOMVecTextParameter_s->mbUnderline;
    meJustification = pVOMVecTextParameter_s->meJustification;
    meVerticalAlignment = pVOMVecTextParameter_s->meVerticalAlignment;
    mnResolution = pVOMVecTextParameter_s->mnResolution;
    mpVecFont = pVOMVecTextParameter_s->mpVecFont;
    mbScaleDependency = pVOMVecTextParameter_s->mbScaleDependency;
    copyFontName(msFontName, pVOMVecTextParameter_s->msFontName);
}

void TDVOMVecTextExtVar::GetParameter(TDVOMVecTextParameter_s* pVOMVecTextParameter_s) const {
    if (!pVOMVecTextParameter_s) {
        return;
    }
    pVOMVecTextParameter_s->mnXScale = mnXScale;
    pVOMVecTextParameter_s->mnYScale = mnYScale;
    pVOMVecTextParameter_s->mnAngle = mnAngle;
    pVOMVecTextParameter_s->mnIncline = mnIncline;
    pVOMVecTextParameter_s->mnLineSpacing = mnLineSpacing;
    pVOMVecTextParameter_s->mnCharSpacing = mnCharSpacing;
    pVOMVecTextParameter_s->mbVertical = mbVertical;
    pVOMVecTextParameter_s->mbUnderline = mbUnderline;
    pVOMVecTextParameter_s->meJustification = meJustification;
    pVOMVecTextParameter_s->meVerticalAlignment = meVerticalAlignment;
    pVOMVecTextParameter_s->mnResolution = mnResolution;
    pVOMVecTextParameter_s->mpVecFont = mpVecFont;
    pVOMVecTextParameter_s->mbScaleDependency = mbScaleDependency;
    copyFontName(pVOMVecTextParameter_s->msFontName, msFontName);
}

void TDVOMVecTextExtVar::SetParameterValidity(const TDVecTextParameterValidity* pVOMVecTextParameter_v) {
    if (!pVOMVecTextParameter_v) {
        return;
    }
    mParamValidity = *pVOMVecTextParameter_v;
}

void TDVOMVecTextExtVar::GetParameterValidity(TDVecTextParameterValidity* pVOMVecTextParameter_v) const {
    if (!pVOMVecTextParameter_v) {
        return;
    }
    *pVOMVecTextParameter_v = mParamValidity;
}

void TDVOMVecTextExtVar::SetAllParameterValidity(bool bValue) {
    mParamValidity.mbXScale = bValue;
    mParamValidity.mbYScale = bValue;
    mParamValidity.mbAngle = bValue;
    mParamValidity.mbIncline = bValue;
    mParamValidity.mbLineSpacing = bValue;
    mParamValidity.mbCharSpacing = bValue;
    mParamValidity.mbVertical = bValue;
    mParamValidity.mbUnderline = bValue;
    mParamValidity.mbJustification = bValue;
    mParamValidity.mbVerticalAlignment = bValue;
    mParamValidity.mbResolution = bValue;
    mParamValidity.mbVecFont = bValue;
    mParamValidity.mbScaleDependency = bValue;
}

void TDVOMVecTextExtVar::SetText(const char* pText) {
    msText = pText ? pText : "";
    mbHasText = true;
}

const char* TDVOMVecTextExtVar::GetText() const {
    return mbHasText ? msText.c_str() : nullptr;
}

void TDVOMVecTextExtVar::DeleteText() {
    msText.clear();
    mbHasText = false;
}

bool TDVOMVecTextExtVar::TextIsOK() const {
    return mbHasText;
}

TDVecObjectType TDVOMVecTextExtVar::GetObjectType() const {
    return mObjectType;
}
