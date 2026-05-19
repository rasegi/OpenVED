#pragma once

#include "vec_edit.h"
#include "vec_view_interface.h"

#include <vector>

const int ViewsAnzahl = 8;

class TDViewOperationManager;

class TDVecEditCad : public TDVecEdit {
public:
    TDVecEditCad();
    ~TDVecEditCad() override;

    void PaintContentOnView(TDVecViewInterfaceBase* pVecViewInterface);
    void DrawObject(TDVecObject* pObject, bool bOutLine);
    void AppendTempNode(double X, double Y, TDNodeType eNodeType, bool bLock);
    void ViewsRefresh() override;
    void MouseDown(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void MouseUp(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void MouseMove(TDVecViewInterfaceBase* pVecViewInterface, TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);

    TDVecViewInterfaceBase* GetActiveView();
    TDGraphicEngine* GetActiveGraphicEngine();
    long FindObjectFrame(TDGraphicEngine* pGE, double X, double Y) const;

    void SetVisualNodes(bool bVisualNodes);
    void SetVisualFrame(bool bVisualFrame);
    bool GetVisualNodes();
    bool GetVisualFrame();

    void RegisterViewInterface(TDVecViewInterfaceBase* pViewInterface);
    void SetViewOperationManager(TDViewOperationManager* pViewOperationManager);
    TDViewOperationManager* GetViewOperationManager();
    void SetOperation(TDViewOperationEnum operation);
    void TmpAppend(TDVecObject* pObject, bool bOutLine);
    void TmpAppendAdditional(TDVecObject* pObject, bool bOutLine);
    void TmpClear();
    void ClearInteractiveNodes();
    void UseCursor(TDVecViewCursor eShape);
    TDVecViewCursor GetUsedCursor() const;
    void Beep();

private:
    struct InteractiveNode {
        double x;
        double y;
        TDNodeType nodeType;
        bool lock;
    };

    TDVecViewInterfaceBase* mpListViewInterface[ViewsAnzahl];
    TDVecViewInterfaceBase* mpActiveViewInterface;
    TDViewOperationManager* mpViewOperationManager;
    TDVecObject* mpTmpObject;
    std::vector<TDVecObject*> mpTmpObjects;
    std::vector<InteractiveNode> mInteractiveNodes;
    bool mbTmpObjectOutLine;
    bool mbVisualNodes;
    bool mbVisualFrame;
    TDVecViewCursor meUsedCursor;
};
