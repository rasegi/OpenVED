//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Round-Rectangle-NotRotated
//---------------------------------------------------------------------------
#define __VOC_ROUNDRECTANGLE_NOTROTATED_CPP

#include "voc_roundrectangle_notrotated.h"

#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_polycurve_area.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
TDMatRect createOrthogonalRect(TDMatPoint p1, TDMatPoint p2) {
    return {
        {p1.x, p1.y},
        {p2.x, p1.y},
        {p2.x, p2.y},
        {p1.x, p2.y}
    };
}

TDMatRect arrangeRect(TDMatRect rect) {
    return {
        {std::min(rect.P1.x, rect.P3.x), std::min(rect.P1.y, rect.P3.y)},
        {std::max(rect.P1.x, rect.P3.x), std::min(rect.P1.y, rect.P3.y)},
        {std::max(rect.P1.x, rect.P3.x), std::max(rect.P1.y, rect.P3.y)},
        {std::min(rect.P1.x, rect.P3.x), std::max(rect.P1.y, rect.P3.y)}
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

TDMatConturPoint conturPoint(double x, double y, TDConturPointType type) {
    return {x, y, type, true};
}

std::unique_ptr<TDVecPolyCurveArea> createRoundRectangleArea(
    const TDMatRect& rect,
    double radius,
    unsigned int resolution,
    bool showControls,
    bool showPolygon,
    TDRgbColor color,
    bool setColor) {
    auto polyCurveArea = std::make_unique<TDVecPolyCurveArea>();

    const double width = MatDistance2Point(rect.P1, rect.P2);
    const double height = MatDistance2Point(rect.P2, rect.P3);
    const double maxRadius = (width <= height) ? (width / 2.0) : (height / 2.0);
    double clampedRadius = radius;
    if (clampedRadius > maxRadius) {
        if (clampedRadius > 0.0) {
            clampedRadius = maxRadius;
        } else if (clampedRadius < 0.0) {
            clampedRadius = -maxRadius;
        }
    }

    for (unsigned int i = 1; i < 6; ++i) {
        switch (i) {
        case 1: {
            const TDMatPoint p = rectEdge(rect, i);
            polyCurveArea->AppendPoint(conturPoint(p.x + clampedRadius, p.y, CPT_PRIME_LINE));
            break;
        }
        case 2: {
            const TDMatPoint p = rectEdge(rect, i);
            polyCurveArea->AppendPoint(conturPoint(p.x - clampedRadius, p.y, CPT_PRIME_LINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y, CPT_PRIME_QSPLINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y + clampedRadius, CPT_PRIME_LINE));
            break;
        }
        case 3: {
            const TDMatPoint p = rectEdge(rect, i);
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y - clampedRadius, CPT_PRIME_LINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y, CPT_PRIME_QSPLINE));
            polyCurveArea->AppendPoint(conturPoint(p.x - clampedRadius, p.y, CPT_PRIME_LINE));
            break;
        }
        case 4: {
            const TDMatPoint p = rectEdge(rect, i);
            polyCurveArea->AppendPoint(conturPoint(p.x + clampedRadius, p.y, CPT_PRIME_LINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y, CPT_PRIME_QSPLINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y - clampedRadius, CPT_PRIME_LINE));
            break;
        }
        case 5: {
            const TDMatPoint p = rectEdge(rect, 1);
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y + clampedRadius, CPT_PRIME_LINE));
            polyCurveArea->AppendPoint(conturPoint(p.x, p.y, CPT_PRIME_QSPLINE));
            polyCurveArea->AppendPoint(conturPoint(p.x + clampedRadius, p.y, CPT_PRIME_LINE));
            break;
        }
        default:
            break;
        }
    }

    polyCurveArea->SetSelect(false);
    polyCurveArea->SetRectangularLock(true);
    polyCurveArea->SetShowControls(showControls);
    polyCurveArea->SetShowPolygon(showPolygon);
    polyCurveArea->SetResolution(resolution);
    if (setColor) {
        polyCurveArea->SetColor(color);
    }
    polyCurveArea->Initialize();
    return polyCurveArea;
}
}

TDVOCRoundRectangleNotRotated::TDVOCRoundRectangleNotRotated(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mMatRect{},
      mpVOCRoundRectNotRotExtVar(nullptr),
      mpTmpObject(nullptr),
      mbRectangleOK(false),
      mnRadius(0.0),
      mnResolution(5),
      mbShowControls(false),
      mbShowPolygon(false),
      mbRectangularLock(true) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);

    mpVOPExternalVariables = new TDVOCRoundRectNotRotExtVar(this);
    mpVOCRoundRectNotRotExtVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(mpVOPExternalVariables);
}

TDVOCRoundRectangleNotRotated::TDVOCRoundRectangleNotRotated(const TDVOCRoundRectangleNotRotated& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mMatRect(oldOperation.mMatRect),
      mpVOCRoundRectNotRotExtVar(nullptr),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mbRectangleOK(oldOperation.mbRectangleOK),
      mnRadius(oldOperation.mnRadius),
      mnResolution(oldOperation.mnResolution),
      mbShowControls(oldOperation.mbShowControls),
      mbShowPolygon(oldOperation.mbShowPolygon),
      mbRectangularLock(oldOperation.mbRectangularLock) {
    auto* extVar = new TDVOCRoundRectNotRotExtVar(this);
    if (oldOperation.mpVOCRoundRectNotRotExtVar) {
        *extVar = *oldOperation.mpVOCRoundRectNotRotExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCRoundRectNotRotExtVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(mpVOPExternalVariables);
}

TDVOCRoundRectangleNotRotated::~TDVOCRoundRectangleNotRotated() {
    delete mpVOPExternalVariables;
    mpVOPExternalVariables = nullptr;
    mpVOCRoundRectNotRotExtVar = nullptr;
}

TDVOCRoundRectangleNotRotated& TDVOCRoundRectangleNotRotated::operator=(const TDVOCRoundRectangleNotRotated& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    delete mpVOPExternalVariables;
    auto* extVar = new TDVOCRoundRectNotRotExtVar(this);
    if (oldOperation.mpVOCRoundRectNotRotExtVar) {
        *extVar = *oldOperation.mpVOCRoundRectNotRotExtVar;
    }
    mpVOPExternalVariables = extVar;
    mpVOCRoundRectNotRotExtVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(mpVOPExternalVariables);
    mMatRect = oldOperation.mMatRect;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mbRectangleOK = oldOperation.mbRectangleOK;
    mnRadius = oldOperation.mnRadius;
    mnResolution = oldOperation.mnResolution;
    mbShowControls = oldOperation.mbShowControls;
    mbShowPolygon = oldOperation.mbShowPolygon;
    mbRectangularLock = oldOperation.mbRectangularLock;
    return *this;
}

TDVOCRoundRectangleNotRotated* TDVOCRoundRectangleNotRotated::Clone() const {
    return new TDVOCRoundRectangleNotRotated(*this);
}

void TDVOCRoundRectangleNotRotated::Initialize() {
    if (mpVOCRoundRectNotRotExtVar) {
        mpVOCRoundRectNotRotExtVar->SendUpdateToOPManager();
    }
}

void TDVOCRoundRectangleNotRotated::ExtVarIsChanged() {
    if (!mpVOCRoundRectNotRotExtVar) {
        return;
    }

    mbRectangularLock = mpVOCRoundRectNotRotExtVar->GetRectangularLock();
    mnRadius = std::fabs(mpVOCRoundRectNotRotExtVar->GetRadius());
    mnResolution = mpVOCRoundRectNotRotExtVar->GetResolution();
    mbShowControls = mpVOCRoundRectNotRotExtVar->GetShowControls();
    mbShowPolygon = mpVOCRoundRectNotRotExtVar->GetShowPolygon();
    mpVOCRoundRectNotRotExtVar->SetInitializedFlag(true);
}

void __fastcall TDVOCRoundRectangleNotRotated::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbRectangleOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_FINISHED);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        return;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    if (mClick > 2) {
        IniInteractivePoints();
    }
    ++mClick;
    switch (mClick) {
    case 1:
        mPtBeg = {X, Y};
        mPtEnd = {X, Y};
        break;
    case 2:
        mPtEnd = {X, Y};
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

void __fastcall TDVOCRoundRectangleNotRotated::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && mbRectangleOK && mClick == 2 && (mPtBeg.x != mPtEnd.x || mPtBeg.y != mPtEnd.y)) {
        mMatRect = arrangeRect(createOrthogonalRect(mPtBeg, mPtEnd));
        std::unique_ptr<TDVecPolyCurveArea> polyCurveArea = createRoundRectangleArea(
            mMatRect,
            mnRadius,
            mnResolution,
            mbShowControls,
            mbShowPolygon,
            mpVecModel->GetDefaultColor(),
            true);
        mpVecModel->AppendObject(polyCurveArea.release());

        IniInteractivePoints();
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        SetState(OSTATE_FINISHED);
    } else if (mClick == 2 && !mbRectangleOK) {
        mClick = 1;
        mpVecEditCad->Beep();
    }
}

void __fastcall TDVOCRoundRectangleNotRotated::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    if (mClick == 1 && Button == VIRTMOUSEBTM_LEFT) {
        mClick = 2;
    }
    if (mClick >= 1) {
        mMatRect = arrangeRect(createOrthogonalRect(mPtBeg, mPtEnd));
        mpTmpObject = createRoundRectangleArea(
            mMatRect,
            mnRadius,
            mnResolution,
            mbShowControls,
            mbShowPolygon,
            0,
            false);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);

        const double width = MatDistance2Point(mMatRect.P1, mMatRect.P2);
        const double height = MatDistance2Point(mMatRect.P2, mMatRect.P3);
        if (mpVOCRoundRectNotRotExtVar) {
            mpVOCRoundRectNotRotExtVar->SetRectWidth(width);
            mpVOCRoundRectNotRotExtVar->SetRectHeight(height);
            mpVOCRoundRectNotRotExtVar->SendUpdateToOPManager();
        }

        mPtEnd = {X, Y};
        if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
            mbRectangleOK = false;
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        } else {
            mbRectangleOK = true;
            mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_2);
        }
    }
}

void __fastcall TDVOCRoundRectangleNotRotated::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCRoundRectangleNotRotated::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

TDVOCRoundRectNotRotExtVar::TDVOCRoundRectNotRotExtVar(TDViewOperation* pParentOperation)
    : TDVOPExternalVariables(pParentOperation),
      mnRadius(0.0),
      mnResolution(5),
      mbShowControls(false),
      mbShowPolygon(false),
      mbRectangularLock(true),
      mbInitialized(false),
      mnRectWidth(0.0),
      mnRectHeight(0.0) {
}

TDVOCRoundRectNotRotExtVar::TDVOCRoundRectNotRotExtVar(const TDVOCRoundRectNotRotExtVar& oldExtVar)
    : TDVOPExternalVariables(oldExtVar),
      mnRadius(oldExtVar.mnRadius),
      mnResolution(oldExtVar.mnResolution),
      mbShowControls(oldExtVar.mbShowControls),
      mbShowPolygon(oldExtVar.mbShowPolygon),
      mbRectangularLock(oldExtVar.mbRectangularLock),
      mbInitialized(oldExtVar.mbInitialized),
      mnRectWidth(oldExtVar.mnRectWidth),
      mnRectHeight(oldExtVar.mnRectHeight) {
}

TDVOCRoundRectNotRotExtVar& TDVOCRoundRectNotRotExtVar::operator=(const TDVOCRoundRectNotRotExtVar& oldExtVar) {
    if (this == &oldExtVar) {
        return *this;
    }

    mnRadius = oldExtVar.mnRadius;
    mnResolution = oldExtVar.mnResolution;
    mbShowControls = oldExtVar.mbShowControls;
    mbShowPolygon = oldExtVar.mbShowPolygon;
    mbRectangularLock = oldExtVar.mbRectangularLock;
    mbInitialized = oldExtVar.mbInitialized;
    mnRectWidth = oldExtVar.mnRectWidth;
    mnRectHeight = oldExtVar.mnRectHeight;
    return *this;
}

TDVOCRoundRectNotRotExtVar* TDVOCRoundRectNotRotExtVar::Clone() const {
    return new TDVOCRoundRectNotRotExtVar(*this);
}

void TDVOCRoundRectNotRotExtVar::SetResolution(unsigned int nResolution) {
    mnResolution = nResolution != 0 ? nResolution : 5;
}

unsigned int TDVOCRoundRectNotRotExtVar::GetResolution() const {
    return mnResolution;
}

void TDVOCRoundRectNotRotExtVar::SetShowControls(bool bShowControls) {
    mbShowControls = bShowControls;
}

bool TDVOCRoundRectNotRotExtVar::GetShowControls() const {
    return mbShowControls;
}

void TDVOCRoundRectNotRotExtVar::SetShowPolygon(bool bShowPolygon) {
    mbShowPolygon = bShowPolygon;
}

bool TDVOCRoundRectNotRotExtVar::GetShowPolygon() const {
    return mbShowPolygon;
}

void TDVOCRoundRectNotRotExtVar::SetRadius(double nRadius) {
    mnRadius = nRadius;
}

double TDVOCRoundRectNotRotExtVar::GetRadius() const {
    return mnRadius;
}

void TDVOCRoundRectNotRotExtVar::SetInitializedFlag(bool bInitialized) {
    mbInitialized = bInitialized;
}

bool TDVOCRoundRectNotRotExtVar::GetInitializedFlag() const {
    return mbInitialized;
}

void TDVOCRoundRectNotRotExtVar::SetRectangularLock(bool bValue) {
    mbRectangularLock = bValue;
}

bool TDVOCRoundRectNotRotExtVar::GetRectangularLock() const {
    return mbRectangularLock;
}

void TDVOCRoundRectNotRotExtVar::SetRectWidth(double nRectWidth) {
    mnRectWidth = nRectWidth;
}

double TDVOCRoundRectNotRotExtVar::GetRectWidth() const {
    return mnRectWidth;
}

void TDVOCRoundRectNotRotExtVar::SetRectHeight(double nRectHeight) {
    mnRectHeight = nRectHeight;
}

double TDVOCRoundRectNotRotExtVar::GetRectHeight() const {
    return mnRectHeight;
}
