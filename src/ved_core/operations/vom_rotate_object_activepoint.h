//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Rotate-Object-ActivePoint
//---------------------------------------------------------------------------
#ifndef __VOM_ROTATE_OBJECT_ACTIVEPOINT_H
#define __VOM_ROTATE_OBJECT_ACTIVEPOINT_H

#include "vop_base.h"

#include <memory>
#include <vector>

class TDVOMRotateObjectActPtExtVar : public TDVOPExternalVariables {
public:
    TDVOMRotateObjectActPtExtVar(TDViewOperation* pParentOperation);
    TDVOMRotateObjectActPtExtVar(const TDVOMRotateObjectActPtExtVar&);
    ~TDVOMRotateObjectActPtExtVar() override = default;
    TDVOMRotateObjectActPtExtVar& operator=(const TDVOMRotateObjectActPtExtVar&);
    TDVOMRotateObjectActPtExtVar* Clone() const override;

    void SetAngle(double nAngle);
    double GetAngle() const;
    TDMatPoint GetPivotPoint() const;
    void SetCopyFlag(bool bCopy);
    bool GetCopyFlag() const;
    void SetSelectFlag(bool bSelect);
    bool GetSelectFlag() const;
    bool GetInitializedFlag() const;
    unsigned long GetNumberOfObjects() const;

protected:
    friend class TDVOMRotateObjectActivePoint;
    void SetNumberOfObjects(unsigned long nObjects);
    void SetPivotPoint(TDMatPoint pivotPoint);
    void SetInitializedFlag(bool bInitialized);

private:
    TDVOMRotateObjectActPtExtVar();

    unsigned long mnObjects;
    double mnAngle;
    TDMatPoint mPivotPoint;
    bool mbCopy;
    bool mbSelect;
    bool mbInitialized;
};

class TDVOMRotateObjectActivePoint : public TDVOModify {
public:
    TDVOMRotateObjectActivePoint(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMRotateObjectActivePoint(const TDVOMRotateObjectActivePoint&);
    ~TDVOMRotateObjectActivePoint() override;
    TDVOMRotateObjectActivePoint& operator=(const TDVOMRotateObjectActivePoint&);
    TDVOMRotateObjectActivePoint* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;
    void Finish();

private:
    TDVOMRotateObjectActivePoint();

    TDVOMRotateObjectActPtExtVar* mpVOMRotateObjectActPtExtVar;
    std::vector<std::unique_ptr<TDVecObject>> mTmpObjects;
    std::vector<std::unique_ptr<TDVecObject>> mShadowObjects;
    bool mbRotateOK;
    bool mbRotated;
    double mnAngle;
    bool mbSelect;
    bool mbCopy;
};

#endif
