//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Rectangle-Rotated
//---------------------------------------------------------------------------
#define __VOC_RECTANGLE_ROTATED_CPP

#include "voc_rectangle_rotated.h"

#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_polygon_area.h"

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

double lineAngle(const TDMatLine& line) {
    if (line.x2 == line.x1) {
        if (line.y2 > line.y1) {
            return 90.0;
        }
        if (line.y2 < line.y1) {
            return -90.0;
        }
        return 0.0;
    }
    return RAD_TO_DEG * std::atan2(line.y2 - line.y1, line.x2 - line.x1);
}

TDMatPoint projectPointOnLine(const TDMatLine& line, TDMatPoint point) {
    const double dx = line.x2 - line.x1;
    const double dy = line.y2 - line.y1;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared == 0.0) {
        return {line.x1, line.y1};
    }
    const double t = ((point.x - line.x1) * dx + (point.y - line.y1) * dy) / lengthSquared;
    return {line.x1 + (t * dx), line.y1 + (t * dy)};
}

bool cutPoint(const TDMatLine& a, const TDMatLine& b, TDMatPoint* result) {
    const double a1 = a.y2 - a.y1;
    const double b1 = a.x1 - a.x2;
    const double c1 = (a1 * a.x1) + (b1 * a.y1);
    const double a2 = b.y2 - b.y1;
    const double b2 = b.x1 - b.x2;
    const double c2 = (a2 * b.x1) + (b2 * b.y1);
    const double determinant = (a1 * b2) - (a2 * b1);
    if (std::fabs(determinant) <= 0.0) {
        return false;
    }
    result->x = ((b2 * c1) - (b1 * c2)) / determinant;
    result->y = ((a1 * c2) - (a2 * c1)) / determinant;
    return true;
}

void appendRectToPolygon(TDVecPolygonArea* polygonArea, const TDMatRect& rect) {
    for (unsigned int i = 0; i < 4; ++i) {
        polygonArea->AppendPoint(rectEdge(rect, i + 1));
    }
}
}

TDVOCRectangleRotated::TDVOCRectangleRotated(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOCreate(pVecModel, pVecEditCad, pParentOperationManager),
      mMatRect{},
      mMatLine{},
      mpTmpObject(nullptr),
      mnAngle(0.0),
      mbRotational(false),
      mbRectangleOK(false) {
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_ROTATED_1);
}

TDVOCRectangleRotated::TDVOCRectangleRotated(const TDVOCRectangleRotated& oldOperation)
    : TDVOCreate(oldOperation.mpVecModel, oldOperation.mpVecEditCad, oldOperation.mpParentOperationManager),
      mMatRect(oldOperation.mMatRect),
      mMatLine(oldOperation.mMatLine),
      mpTmpObject(oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr),
      mnAngle(oldOperation.mnAngle),
      mbRotational(oldOperation.mbRotational),
      mbRectangleOK(oldOperation.mbRectangleOK) {
}

TDVOCRectangleRotated& TDVOCRectangleRotated::operator=(const TDVOCRectangleRotated& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }
    mpVecModel = oldOperation.mpVecModel;
    mpVecEditCad = oldOperation.mpVecEditCad;
    mpParentOperationManager = oldOperation.mpParentOperationManager;
    mMatRect = oldOperation.mMatRect;
    mMatLine = oldOperation.mMatLine;
    mpTmpObject = oldOperation.mpTmpObject ? oldOperation.mpTmpObject->Clone() : nullptr;
    mnAngle = oldOperation.mnAngle;
    mbRotational = oldOperation.mbRotational;
    mbRectangleOK = oldOperation.mbRectangleOK;
    return *this;
}

TDVOCRectangleRotated* TDVOCRectangleRotated::Clone() const {
    return new TDVOCRectangleRotated(*this);
}

void __fastcall TDVOCRectangleRotated::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;

    switch (GetState()) {
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        mbRotational = false;
        mbRectangleOK = false;
        IniInteractivePoints();
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_ROTATED_1);
        break;
    default:
        break;
    }

    if (Button == VIRTMOUSEBTM_RIGHT) {
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_FINISHED);
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_ROTATED_1);
        return;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    ++mClick;
    switch (mClick) {
    case 1:
        mPtBeg = {X, Y};
        mPtEnd = {X, Y};
        mMatLine.Create(mPtBeg, mPtEnd);
        mMatRect.P1 = mPtBeg;
        mbRectangleOK = false;
        break;
    case 2: {
        mPtEnd = {X, Y};
        mMatLine.Create(mPtBeg, mPtEnd);
        mnAngle = lineAngle(mMatLine);
        const double dSin = sinD(mnAngle);
        const double dCos = cosD(mnAngle);
        if ((dSin == 0.0 || dCos == 1.0 || dCos == -1.0) || (dSin == 1.0 || dSin == -1.0 || dCos == 0.0)) {
            mbRotational = false;
        } else {
            mbRotational = true;
        }
        break;
    }
    case 3:
        if (!mbRectangleOK) {
            mClick = 2;
            mpVecEditCad->Beep();
        }
        break;
    default:
        break;
    }
}

void __fastcall TDVOCRectangleRotated::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    (void)X;
    (void)Y;

    if (Button == VIRTMOUSEBTM_LEFT && mbRectangleOK && mClick == 3 && (mPtBeg.x != mPtEnd.x || mPtBeg.y != mPtEnd.y)) {
        auto* polygonArea = new TDVecPolygonArea();
        appendRectToPolygon(polygonArea, mMatRect);
        polygonArea->SetSelect(false);
        polygonArea->SetRectangularLock(true);
        polygonArea->SetColor(mpVecModel->GetDefaultColor());
        polygonArea->Initialize();
        mpVecModel->AppendObject(polygonArea);

        IniInteractivePoints();
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
        SetState(OSTATE_FINISHED);
    }
}

void __fastcall TDVOCRectangleRotated::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Button;
    (void)Shift;

    switch (mClick) {
    case 1: {
        auto tmpLine = std::make_unique<TDVecLine>();
        mMatLine.Create(mPtBeg, mPtEnd);
        tmpLine->SetLine(&mMatLine);
        mpTmpObject = std::move(tmpLine);
        mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
        mPtEnd = {X, Y};
        break;
    }
    case 2: {
        std::unique_ptr<TDVecPolygonArea> tmpPolygonArea;
        if (mbRotational) {
            mMatRect.P3 = mPtEnd;
            const TDMatPoint projectPoint = projectPointOnLine(mMatLine, mPtEnd);
            mMatRect.P2 = projectPoint;

            if (!MatBelike2Double(mMatRect.P2.x, mMatRect.P3.x, 4) && !MatBelike2Double(mMatRect.P2.y, mMatRect.P3.y, 4)) {
                const TDMatLine line1{mMatRect.P1.x, mMatRect.P1.y, mMatRect.P2.x, mMatRect.P2.y};
                const TDMatLine line2{mMatRect.P2.x, mMatRect.P2.y, mMatRect.P3.x, mMatRect.P3.y};
                const TDMatLine line3{
                    mMatRect.P3.x,
                    mMatRect.P3.y,
                    mMatRect.P3.x + (line1.x2 - line1.x1),
                    mMatRect.P3.y + (line1.y2 - line1.y1)
                };
                const TDMatLine line4{
                    mMatRect.P1.x,
                    mMatRect.P1.y,
                    mMatRect.P1.x + (line2.x2 - line2.x1),
                    mMatRect.P1.y + (line2.y2 - line2.y1)
                };
                TDMatPoint p4;
                if (cutPoint(line3, line4, &p4)) {
                    mMatRect.P4 = p4;
                    tmpPolygonArea = std::make_unique<TDVecPolygonArea>();
                    appendRectToPolygon(tmpPolygonArea.get(), mMatRect);
                    tmpPolygonArea->SetSelect(false);
                    mpTmpObject = std::move(tmpPolygonArea);
                    mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
                    mbRectangleOK = true;
                } else {
                    mbRectangleOK = false;
                }
            } else {
                mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
                mbRectangleOK = false;
            }
        } else {
            mMatRect = createOrthogonalRect(mPtBeg, mPtEnd);
            tmpPolygonArea = std::make_unique<TDVecPolygonArea>();
            appendRectToPolygon(tmpPolygonArea.get(), mMatRect);
            tmpPolygonArea->SetSelect(false);
            mpTmpObject = std::move(tmpPolygonArea);
            mpVecEditCad->TmpAppend(mpTmpObject.get(), true);
            if (MatBelike2Double(mPtEnd.x, mPtBeg.x, 4) || MatBelike2Double(mPtEnd.y, mPtBeg.y, 4)) {
                mbRectangleOK = false;
                mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_1);
            } else {
                mbRectangleOK = true;
                mpVecEditCad->UseCursor(VECVIEW_CREATE_RECTANGLE_NOTROTATED_2);
            }
        }
        mPtEnd = {X, Y};
        break;
    }
    default:
        break;
    }
}

void __fastcall TDVOCRectangleRotated::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOCRectangleRotated::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
