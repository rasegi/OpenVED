//---------------------------------------------------------------------------
// MODULE: View-Operation-Modify-Select-Object
//---------------------------------------------------------------------------
#define __VOM_SELECT_OBJECT_CPP

#include "vom_select_object.h"

#include "vec_edit_cad.h"
#include "vec_model.h"

#include <algorithm>
#include <cmath>

TDVOMSelectObject::TDVOMSelectObject(
    TDVecModel* pVecModel,
    TDVecEditCad* pVecEditCad,
    TDViewOperationManager* pParentOperationManager)
    : TDVOModify(pVecModel, pVecEditCad, pParentOperationManager),
      miSelectObject(-1),
      mpSelectObject(nullptr),
      mAreaStart{0.0, 0.0},
      mAreaCurrent{0.0, 0.0},
      mbAreaSelect(false),
      mbAreaMoved(false) {
    SetState(OSTATE_RUNNING);
    mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
}

TDVOMSelectObject::~TDVOMSelectObject() = default;

TDVOMSelectObject::TDVOMSelectObject(const TDVOMSelectObject& oldOperation)
    : TDVOModify(oldOperation),
      miSelectObject(oldOperation.miSelectObject),
      mpSelectObject(oldOperation.mpSelectObject),
      mAreaStart(oldOperation.mAreaStart),
      mAreaCurrent(oldOperation.mAreaCurrent),
      mbAreaSelect(oldOperation.mbAreaSelect),
      mbAreaMoved(oldOperation.mbAreaMoved) {
}

TDVOMSelectObject& TDVOMSelectObject::operator=(const TDVOMSelectObject& oldOperation) {
    if (this == &oldOperation) {
        return *this;
    }

    TDVOModify::operator=(oldOperation);
    miSelectObject = oldOperation.miSelectObject;
    mpSelectObject = oldOperation.mpSelectObject;
    mAreaStart = oldOperation.mAreaStart;
    mAreaCurrent = oldOperation.mAreaCurrent;
    mbAreaSelect = oldOperation.mbAreaSelect;
    mbAreaMoved = oldOperation.mbAreaMoved;
    return *this;
}

TDVOMSelectObject* TDVOMSelectObject::Clone() const {
    return new TDVOMSelectObject(*this);
}

double TDVOMSelectObject::MouseToleranceReal() const {
    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    if (!graphicEngine) {
        return 0.0;
    }

    const long tolerancePixels = graphicEngine->GetMouseTolerance();
    return std::max(graphicEngine->ScreenToXVal(tolerancePixels), graphicEngine->ScreenToYVal(tolerancePixels));
}

void __fastcall TDVOMSelectObject::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    switch (GetState()) {
    case OSTATE_UNKNOWN:
    case OSTATE_ABORTED:
        return;
    case OSTATE_STARTED:
    case OSTATE_FINISHED:
        SetState(OSTATE_RUNNING);
        mpVecEditCad->UseCursor(VECVIEW_CURSOR_DEFAULT);
        break;
    case OSTATE_RUNNING:
    case OSTATE_NEEDSELECTED:
        break;
    }

    if (Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    TDGraphicEngine* graphicEngine = mpVecEditCad->GetActiveGraphicEngine();
    TDVecModelHitResult hit;
    if (graphicEngine) {
        hit = mpVecModel->FindObjectAt(graphicEngine, {X, Y}, graphicEngine->GetMouseTolerance());
    } else {
        hit = mpVecModel->FindObjectAt({X, Y}, MouseToleranceReal());
    }

    miSelectObject = hit.objectIndex;
    mpSelectObject = hit.object;
    mbAreaSelect = !hit.IsHit();
    mbAreaMoved = false;
    mAreaStart = {X, Y};
    mAreaCurrent = mAreaStart;

    if (!hit.IsHit()) {
        return;
    }

    if ((Shift & VKState_KEY_SHIFT) != 0) {
        mpVecModel->SelectObject(miSelectObject);
    } else {
        mpVecModel->SelectOnlyObject(miSelectObject);
    }

    mpVecEditCad->ViewsRefresh();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOMSelectObject::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    if (Button != VIRTMOUSEBTM_LEFT) {
        mbAreaSelect = false;
        mbAreaMoved = false;
        return;
    }

    if (!mbAreaSelect) {
        return;
    }

    mAreaCurrent = {X, Y};
    if (mbAreaMoved) {
        mpVecModel->SelectObjectsInArea(mAreaStart, mAreaCurrent, (Shift & VKState_KEY_SHIFT) != 0);
    } else {
        mpVecModel->DeselectAll();
    }

    mbAreaSelect = false;
    mbAreaMoved = false;
    mpVecEditCad->ViewsRefresh();
    bool destroyed = false;
    SetState(OSTATE_FINISHED, &destroyed);
}

void __fastcall TDVOMSelectObject::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    (void)Shift;
    if (!mbAreaSelect || Button != VIRTMOUSEBTM_LEFT) {
        return;
    }

    mAreaCurrent = {X, Y};
    const double tolerance = MouseToleranceReal();
    if (std::fabs(mAreaCurrent.x - mAreaStart.x) > tolerance ||
        std::fabs(mAreaCurrent.y - mAreaStart.y) > tolerance) {
        mbAreaMoved = true;
    }
}

void __fastcall TDVOMSelectObject::OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}

void __fastcall TDVOMSelectObject::OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) {
    (void)eVirtualKey;
    (void)StateKey;
}
