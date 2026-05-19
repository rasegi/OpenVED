//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-SelectMove-Object-Scale-Frame
//---------------------------------------------------------------------------
#pragma once

#include "vop_base.h"

#include <memory>
#include <vector>

class TDVOMSelectMoveObjScaleFrameExtVar : public TDVOPExternalVariables {
public:
    explicit TDVOMSelectMoveObjScaleFrameExtVar(TDViewOperation* pParentOperation);
    TDVOMSelectMoveObjScaleFrameExtVar(const TDVOMSelectMoveObjScaleFrameExtVar&);
    TDVOMSelectMoveObjScaleFrameExtVar& operator=(const TDVOMSelectMoveObjScaleFrameExtVar&);
    ~TDVOMSelectMoveObjScaleFrameExtVar() override = default;
    TDVOMSelectMoveObjScaleFrameExtVar* Clone() const override;

    TDMatRect GetFrame() const;
    double GetHeight() const;
    double GetWidth() const;
    long GetObjectsCount() const;

    void SetFrame(TDMatRect Frame);
    void SetHeight(double nHeight);
    void SetWidth(double nWidth);
    void SetObjectsCount(long nObjectsCount);

private:
    TDMatRect mFrame;
    double mnHeight;
    double mnWidth;
    long mnObjectsCount;
};

class TDVOMSelectMoveObjectScaleFrame : public TDVOModify {
public:
    TDVOMSelectMoveObjectScaleFrame(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMSelectMoveObjectScaleFrame(const TDVOMSelectMoveObjectScaleFrame&);
    TDVOMSelectMoveObjectScaleFrame& operator=(const TDVOMSelectMoveObjectScaleFrame&);
    ~TDVOMSelectMoveObjectScaleFrame() override;
    TDVOMSelectMoveObjectScaleFrame* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    long FindObjectFrame(TDMatPoint point) const;
    long PointOnSelectedFrameNode(TDVecObject* object, TDMatPoint point) const;
    double MouseToleranceReal() const;
    bool HasMovedPastTolerance(TDMatPoint point) const;
    void ResetInteractiveState();
    void FinishInteraction();
    void UpdateExternalVariables(TDVecObject* object);
    void UpdateExternalVariablesForSelection();
    void AppendTmpObject(TDVecObject* object, bool first);

    TDVOMSelectMoveObjScaleFrameExtVar* mpVOMSelectMoveObjScaleFrameExtVar;
    std::vector<std::unique_ptr<TDVecObject>> mTmpObjects;
    bool mbMoveObject;
    bool mbMoveNode;
    bool mbObjectChanged;
    bool mbMustViewsRefresh;
    bool mbDrawLassoOK;
    bool mbGroupMove;
    long mnNodeMove;
    long miObjIndex;
    bool mbToleranzOK;
    bool mbDialogOpen;
};
