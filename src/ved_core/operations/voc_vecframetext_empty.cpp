#include "voc_vecframetext_empty.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polygon_area.h"

#include <cstdio>
#include <memory>

namespace {
void copyFontName(char* destination, const char* source) {
    std::snprintf(destination, 30, "%s", source ? source : "");
}

TDMatRect createOrthogonalRect(TDMatPoint p1, TDMatPoint p2) {
    return {
        {p1.x, p1.y},
        {p2.x, p1.y},
        {p2.x, p2.y},
        {p1.x, p2.y}
    };
}

TDMatPoint rectEdge(const TDMatRect& rect, unsigned int edge) {
    switch (edge) {
    case 1:
        return rect.P1;
    case 2:
        return rect.P2;
    case 3:
        return rect.P3;
    case 4:
        return rect.P4;
    default:
        return {};
    }
}
}

TDVOCVecFrameTextEmpty::TDVOCVecFrameTextEmpty(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mMatRect{},
      mpTmpObject(nullptr),
      mbRectangleOK(false),
      mpVOCVecFrameTextEmptyExtVar(nullptr) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);

    mpVOPExternalVariables = new TDVOCVecFrameTextEmptyExtVar(this);
    mpVOCVecFrameTextEmptyExtVar = dynamic_cast<TDVOCVecFrameTextEmptyExtVar*>(mpVOPExternalVariables);

    mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
}

TDVOCVecFrameTextEmpty::TDVOCVecFrameTextEmpty(const TDVOCVecFrameTextEmpty& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mMatRect(oldOperation.mMatRect),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbRectangleOK(oldOperation.mbRectangleOK),
      mpVOCVecFrameTextEmptyExtVar(nullptr) {
    auto* extVar = new TDVOCVecFrameTextEmptyExtVar(this);
    if (oldOperation.mpVOCVecFrameTextEmptyExtVar) {
        *extVar = *oldOperation.mpVOCVecFrameTextEmptyExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCVecFrameTextEmptyExtVar = extVar;
}

TDVOCVecFrameTextEmpty::~TDVOCVecFrameTextEmpty() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOCVecFrameTextEmptyExtVar = nullptr;
}

TDVOCVecFrameTextEmpty& TDVOCVecFrameTextEmpty::operator=(const TDVOCVecFrameTextEmpty& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mMatRect = oldOperation.mMatRect;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbRectangleOK = oldOperation.mbRectangleOK;

    delete mpVOPExternalVariables;
    auto* extVar = new TDVOCVecFrameTextEmptyExtVar(this);
    if (oldOperation.mpVOCVecFrameTextEmptyExtVar) {
        *extVar = *oldOperation.mpVOCVecFrameTextEmptyExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCVecFrameTextEmptyExtVar = extVar;
    return *this;
}

TDVOCVecFrameTextEmpty* TDVOCVecFrameTextEmpty::Clone() const {
    return new TDVOCVecFrameTextEmpty(*this);
}

void TDVOCVecFrameTextEmpty::Initialize() {
    if (mpVOCVecFrameTextEmptyExtVar) {
        mpVOCVecFrameTextEmptyExtVar->SendUpdateToOPManager();
    }
}

void TDVOCVecFrameTextEmpty::ExtVarIsChanged() {
}

void TDVOCVecFrameTextEmpty::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState, double X, double Y) {
    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbRectangleOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_RECTANGLE_NOTROTATED_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        }
        break;
    case OSTATE_UNKNOWN:
    case OSTATE_RUNNING:
    case OSTATE_ABORTED:
    case OSTATE_NEEDSELECTED:
        break;
    }

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();

        bool destroyed = false;
        SetState(OSTATE_FINISHED, &destroyed);
        if (destroyed) {
            return;
        }

        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_RECTANGLE_NOTROTATED_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        }
    }

    if (Button == VIRTMOUSEBTM_LEFT) {
        if (mClick > 2) {
            IniInteractivePoints();
        }
        mClick += 1;
        switch (mClick) {
        case 1:
            mPtBeg.x = X;
            mPtBeg.y = Y;
            mPtEnd.x = X;
            mPtEnd.y = Y;
            break;
        case 2:
            mPtEnd.x = X;
            mPtEnd.y = Y;
            if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
                mbRectangleOK = false;
                mClick = 1;
                mpVecEditCad->Beep();
            } else {
                mbRectangleOK = true;
            }
            break;
        default:
            break;
        }
    }
}

void TDVOCVecFrameTextEmpty::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState, double, double) {
    if (Button == VIRTMOUSEBTM_LEFT && mbRectangleOK) {
        if (mClick == 2 && (mPtBeg.x != mPtEnd.x || mPtBeg.y != mPtEnd.y)) {
            mMatRect = createOrthogonalRect(mPtBeg, mPtEnd);

            auto* frameText = new TDVecFrameText();
            TDVOCVecFrameTextEmptyParameter_s parameter;
            mpVOCVecFrameTextEmptyExtVar->GetParameter(&parameter);
            frameText->SetParameter(&parameter);
            frameText->SetScaleDependency(true);
            frameText->SetRectangle(&mMatRect);
            frameText->SetSelect(false);
            frameText->SetOriginPoint(mMatRect.P1);
            frameText->SetColor(mpVecModel->GetDefaultColor());
            frameText->Initialize();

            mpVecModel->AppendObject(frameText);
            IniInteractivePoints();
            mpVecEditCad->TmpClear();
            mpVecEditCad->ViewsRefresh();
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);

            bool destroyed = false;
            SetState(OSTATE_FINISHED, &destroyed);
            if (destroyed) {
                return;
            }
        }
    } else if (mClick == 2 && !mbRectangleOK) {
        mClick = 1;
        mpVecEditCad->Beep();
    }
}

void TDVOCVecFrameTextEmpty::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState, double X, double Y) {
    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatRect = createOrthogonalRect(mPtBeg, mPtEnd);
        auto tmpPolygonArea = std::make_unique<TDVecPolygonArea>();
        for (unsigned int i = 0; i < 4; ++i) {
            tmpPolygonArea->AppendPoint(rectEdge(mMatRect, i + 1));
        }
        tmpPolygonArea->SetSelect(false);
        mpTmpObject = std::move(tmpPolygonArea);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);

        mPtEnd.x = X;
        mPtEnd.y = Y;
        if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
            mbRectangleOK = false;
            if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_RECTANGLE_NOTROTATED_1) {
                mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
            }
        } else {
            mbRectangleOK = true;
            if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_RECTANGLE_NOTROTATED_2) {
                mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_2);
            }
        }
    }
}

void TDVOCVecFrameTextEmpty::OPKeyDown(TDOPVirtKey, TDOPVirtKeyState) {
}

void TDVOCVecFrameTextEmpty::OPKeyUp(TDOPVirtKey, TDOPVirtKeyState) {
}

TDVOCVecFrameTextEmptyExtVar::TDVOCVecFrameTextEmptyExtVar(TDViewOperation* pParentOperation)
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

TDVOCVecFrameTextEmptyExtVar::TDVOCVecFrameTextEmptyExtVar(const TDVOCVecFrameTextEmptyExtVar& oldExtVar)
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

TDVOCVecFrameTextEmptyExtVar& TDVOCVecFrameTextEmptyExtVar::operator=(const TDVOCVecFrameTextEmptyExtVar& oldExtVar) {
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

TDVOCVecFrameTextEmptyExtVar* TDVOCVecFrameTextEmptyExtVar::Clone() const {
    return new TDVOCVecFrameTextEmptyExtVar(*this);
}

void TDVOCVecFrameTextEmptyExtVar::SetParameter(const TDVOCVecFrameTextEmptyParameter_s* pVOCVecTextParameter_s) {
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

void TDVOCVecFrameTextEmptyExtVar::GetParameter(TDVOCVecFrameTextEmptyParameter_s* pVOCVecTextParameter_s) const {
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

void TDVOCVecFrameTextEmptyExtVar::SetText(const char* pText) {
    msText = pText ? pText : "";
    mbTextIsDeleted = false;
}

const char* TDVOCVecFrameTextEmptyExtVar::GetText() const {
    return msText.empty() ? nullptr : msText.c_str();
}

void TDVOCVecFrameTextEmptyExtVar::DeleteText() {
    msText.clear();
    mbTextIsDeleted = true;
}

bool TDVOCVecFrameTextEmptyExtVar::TextIsOK() const {
    return !msText.empty();
}

bool TDVOCVecFrameTextEmptyExtVar::TextIsDeleted() const {
    return mbTextIsDeleted;
}
