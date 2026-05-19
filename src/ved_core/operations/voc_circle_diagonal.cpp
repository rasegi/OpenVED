//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Circle-Diagonal
//---------------------------------------------------------------------------
#define __VOC_CIRCLE_DIAGONAL_CPP

#include "voc_circle_diagonal.h"

#include "vec_edit_cad.h"
#include "vec_ellipse.h"
#include "vec_model.h"

namespace {
double minimumDistance(TDVecEditCad* editCad) {
    TDGraphicEngine* graphicEngine = editCad ? editCad->GetActiveGraphicEngine() : nullptr;
    return graphicEngine ? graphicEngine->GetMinimumDistance() : 0.0;
}
}

TDVOCCircleDiagonal::TDVOCCircleDiagonal(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mpObjEllipse(std::make_unique<TDVecEllipse>()),
      mMatEllipse{},
      mbEllipseOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
}

TDVOCCircleDiagonal::TDVOCCircleDiagonal(const TDVOCCircleDiagonal& oldOperation)
    : TDVOCreate(oldOperation),
      mpObjEllipse(oldOperation.mpObjEllipse ? std::make_unique<TDVecEllipse>(*oldOperation.mpObjEllipse) : nullptr),
      mMatEllipse(oldOperation.mMatEllipse),
      mbEllipseOK(oldOperation.mbEllipseOK) {
}

TDVOCCircleDiagonal& TDVOCCircleDiagonal::operator=(const TDVOCCircleDiagonal& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    TDVOCreate::operator=(oldOperation);
    mpObjEllipse = oldOperation.mpObjEllipse ? std::make_unique<TDVecEllipse>(*oldOperation.mpObjEllipse) : nullptr;
    mMatEllipse = oldOperation.mMatEllipse;
    mbEllipseOK = oldOperation.mbEllipseOK;
    return *this;
}

TDVOCCircleDiagonal* TDVOCCircleDiagonal::Clone() const {
    return new TDVOCCircleDiagonal(*this);
}

void __fastcall TDVOCCircleDiagonal::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    switch (GetState()) {
    case OSTATE_STARTED:
        mbEllipseOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
        }
        break;
    case OSTATE_FINISHED:
        mbEllipseOK = false;
        SetState(OSTATE_RUNNING);
        IniInteractivePoints();
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
        }
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();

        bool bDestroyed = false;
        SetState(OSTATE_FINISHED, &bDestroyed);
        if (bDestroyed) {
            return;
        }

        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
        }
    }

    if (Button == VIRTMOUSEBTM_LEFT) {
        if (mClick > 2) {
            IniInteractivePoints();
        }
        ++mClick;
        switch (mClick) {
        case 1:
            mPtBeg = {X, Y};
            mPtEnd = {X, Y};
            break;
        case 2: {
            mPtEnd = {X, Y};
            const double nToleranz = minimumDistance(mpVecEditCad);
            mMatEllipse.CreateCircleDiagonal(mPtBeg, mPtEnd);
            if (mMatEllipse.xRadius < nToleranz || mMatEllipse.yRadius < nToleranz) {
                mbEllipseOK = false;
                mClick = 1;
                if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
                    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
                }
                mpVecEditCad->Beep();
            } else {
                mbEllipseOK = true;
                if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_2) {
                    mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_2);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

void __fastcall TDVOCCircleDiagonal::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && mbEllipseOK) {
        if (mClick == 2) {
            mMatEllipse.CreateCircleDiagonal(mPtBeg, mPtEnd);
            auto* pTmpEllipse = new TDVecEllipse();
            pTmpEllipse->Import(mMatEllipse);
            pTmpEllipse->SetSelect(false);
            pTmpEllipse->SetColor(mpVecModel->GetDefaultColor());
            pTmpEllipse->SetResizeProportional(true);
            pTmpEllipse->Initialize();
            mpVecModel->AppendObject(pTmpEllipse);
            IniInteractivePoints();
            mpVecEditCad->TmpClear();
            mpVecEditCad->ViewsRefresh();
            mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);

            bool bDestroyed = false;
            SetState(OSTATE_FINISHED, &bDestroyed);
        }
    } else if (mClick == 2 && !mbEllipseOK) {
        mClick = 1;
        mpVecEditCad->Beep();
        if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
            mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
        }
    }
}

void __fastcall TDVOCCircleDiagonal::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatEllipse.CreateCircleDiagonal(mPtBeg, mPtEnd);
        mpObjEllipse->Import(mMatEllipse);
        mpVecEditCad->TmpAppend(mpObjEllipse.get(), true);
        mPtEnd = {X, Y};

        const double nToleranz = minimumDistance(mpVecEditCad);
        if (mMatEllipse.xRadius < nToleranz || mMatEllipse.yRadius < nToleranz) {
            mbEllipseOK = false;
            if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_1) {
                mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_1);
            }
        } else {
            mbEllipseOK = true;
            if (mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_CIRCLE_2) {
                mpVecEditCad->UseCursor(VECVIEW_CREATE_CIRCLE_2);
            }
        }
    }
}

void __fastcall TDVOCCircleDiagonal::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCCircleDiagonal::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
