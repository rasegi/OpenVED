#include "vec_edit_cad.h"

#include "vop_manager.h"

TDVecEditCad::TDVecEditCad()
    : mpActiveViewInterface(nullptr),
      mpViewOperationManager(nullptr),
      mpTmpObject(nullptr),
      mbTmpObjectOutLine(false),
      mbVisualNodes(true),
      mbVisualFrame(true),
      meUsedCursor(VECVIEW_CURSOR_DEFAULT) {
    for (auto*& viewInterface : mpListViewInterface) {
        viewInterface = nullptr;
    }
}

TDVecEditCad::~TDVecEditCad() = default;

void TDVecEditCad::PaintContentOnView(TDVecViewInterfaceBase* pVecViewInterface) {
    if (!pVecViewInterface) {
        return;
    }
    TDGraphicEngine* pGE = pVecViewInterface->GetGraphicEngine();
    if (!pGE) {
        return;
    }

    if (mpVecModel) {
        for (int iElem = 0; iElem < mpVecModel->CountObjects(); iElem++) {
            TDVecObject* pObject = mpVecModel->GetObject(iElem);
            if (!pObject) {
                continue;
            }

            if (pVecViewInterface->IsRectVisible(pObject->GetFrame())) {
                pObject->Draw(pGE, false);
                if (pObject->GetSelect()) {
                    if (mbVisualNodes) {
                        pObject->DrawNodes(pGE);
                    }
                    if (mbVisualFrame) {
                        pObject->DrawFrame(pGE);
                    }
                }
            }
        }
    }

    for (TDVecObject* tmpObject : mpTmpObjects) {
        if (tmpObject) {
            tmpObject->Draw(pGE, mbTmpObjectOutLine);
        }
    }

    for (const InteractiveNode& node : mInteractiveNodes) {
        pGE->DrawNode(node.x, node.y, node.nodeType, node.lock);
    }
}

void TDVecEditCad::DrawObject(TDVecObject* pObject, bool bOutLine) {
    if (!pObject) {
        return;
    }

    if (bOutLine) {
        pObject->SetSelect(false);
    }

    for (auto* pVecViewInterface : mpListViewInterface) {
        if (!pVecViewInterface) {
            continue;
        }

        TDGraphicEngine* pGE = pVecViewInterface->GetGraphicEngine();
        if (!pGE) {
            continue;
        }

        pObject->Draw(pGE, bOutLine);
        if (pObject->GetSelect()) {
            if (mbVisualNodes) {
                pObject->DrawNodes(pGE);
            }
            if (mbVisualFrame) {
                pObject->DrawFrame(pGE);
            }
        }
    }
}

void TDVecEditCad::AppendTempNode(double X, double Y, TDNodeType eNodeType, bool bLock) {
    mInteractiveNodes.push_back({X, Y, eNodeType, bLock});
    ViewsRefresh();
}

void TDVecEditCad::ViewsRefresh() {
    for (auto* pVecViewInterface : mpListViewInterface) {
        if (pVecViewInterface) {
            pVecViewInterface->Send_ViewRefresh_ToView();
        }
    }
}

void TDVecEditCad::MouseDown(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    mpActiveViewInterface = pVecViewInterface;
    if (mpViewOperationManager) {
        mpViewOperationManager->MouseDown(Button, Shift, X, Y);
    }
}

void TDVecEditCad::MouseUp(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    mpActiveViewInterface = pVecViewInterface;
    if (mpViewOperationManager) {
        mpViewOperationManager->MouseUp(Button, Shift, X, Y);
    }
}

void TDVecEditCad::MouseMove(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) {
    mpActiveViewInterface = pVecViewInterface;
    if (mpViewOperationManager) {
        mpViewOperationManager->MouseMove(Button, Shift, X, Y);
    }
}

TDVecViewInterfaceBase* TDVecEditCad::GetActiveView() {
    return mpActiveViewInterface;
}

TDGraphicEngine* TDVecEditCad::GetActiveGraphicEngine() {
    return mpActiveViewInterface ? mpActiveViewInterface->GetGraphicEngine() : nullptr;
}

long TDVecEditCad::FindObjectFrame(TDGraphicEngine* pGE, double X, double Y) const {
    if (!pGE || !mpVecModel || mpVecModel->CountObjects() <= 0) {
        return -1;
    }

    long selectedElement = -1;
    const long tolerance = pGE->GetMouseTolerance();
    const TDMatPoint point{X, Y};

    for (int iElem = 0; iElem < mpVecModel->CountObjects(); iElem++) {
        TDVecObject* pObject = mpVecModel->GetObject(iElem);
        if (!pObject) {
            continue;
        }

        if (pObject->HitTest(pGE, point, tolerance).IsHit()) {
            selectedElement = iElem;
        } else if (pObject->GetSelect() && pObject->PointOnFrameNode(pGE, X, Y, tolerance) > -1) {
            selectedElement = iElem;
        }
    }

    return selectedElement;
}

void TDVecEditCad::SetVisualNodes(bool bVisualNodes) {
    mbVisualNodes = bVisualNodes;
}

void TDVecEditCad::SetVisualFrame(bool bVisualFrame) {
    mbVisualFrame = bVisualFrame;
}

bool TDVecEditCad::GetVisualNodes() {
    return mbVisualNodes;
}

bool TDVecEditCad::GetVisualFrame() {
    return mbVisualFrame;
}

void TDVecEditCad::RegisterViewInterface(TDVecViewInterfaceBase* pViewInterface) {
    if (!pViewInterface) {
        return;
    }

    for (auto*& registeredView : mpListViewInterface) {
        if (!registeredView) {
            registeredView = pViewInterface;
            mpActiveViewInterface = pViewInterface;
            return;
        }
    }
}

void TDVecEditCad::SetViewOperationManager(TDViewOperationManager* pViewOperationManager) {
    mpViewOperationManager = pViewOperationManager;
}

TDViewOperationManager* TDVecEditCad::GetViewOperationManager() {
    return mpViewOperationManager;
}

void TDVecEditCad::SetOperation(TDViewOperationEnum operation) {
    if (mpViewOperationManager) {
        meUsedCursor = VECVIEW_CURSOR_MAX;
        mpViewOperationManager->SetOperation(operation);
    }
}

void TDVecEditCad::TmpAppend(TDVecObject* pObject, bool bOutLine) {
    mpTmpObjects.clear();
    mpTmpObject = pObject;
    if (pObject) {
        mpTmpObjects.push_back(pObject);
    }
    mbTmpObjectOutLine = bOutLine;
    ViewsRefresh();
}

void TDVecEditCad::TmpAppendAdditional(TDVecObject* pObject, bool bOutLine) {
    mpTmpObject = pObject;
    if (pObject) {
        mpTmpObjects.push_back(pObject);
    }
    mbTmpObjectOutLine = bOutLine;
    ViewsRefresh();
}

void TDVecEditCad::TmpClear() {
    mpTmpObject = nullptr;
    mpTmpObjects.clear();
    mInteractiveNodes.clear();
    mbTmpObjectOutLine = false;
    ViewsRefresh();
}

void TDVecEditCad::ClearInteractiveNodes() {
    if (mInteractiveNodes.empty()) {
        return;
    }

    mInteractiveNodes.clear();
    ViewsRefresh();
}

void TDVecEditCad::UseCursor(TDVecViewCursor eShape) {
    // TODO untersuchen
    if (meUsedCursor == eShape) {
        return;
    }

    meUsedCursor = eShape;
    if (mpActiveViewInterface) {
        mpActiveViewInterface->Send_UseCursor_ToView(eShape);
    }
}

TDVecViewCursor TDVecEditCad::GetUsedCursor() const {
    return meUsedCursor;
}

void TDVecEditCad::Beep() {
}
