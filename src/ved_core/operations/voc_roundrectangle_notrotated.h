//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Round-Rectangle-NotRotated
//---------------------------------------------------------------------------
#ifndef __VOC_ROUNDRECTANGLE_NOTROTATED_H
#define __VOC_ROUNDRECTANGLE_NOTROTATED_H

#include "vop_base.h"

#include <memory>

class TDVOCRoundRectNotRotExtVar : public TDVOPExternalVariables {
public:
    TDVOCRoundRectNotRotExtVar(TDViewOperation* pParentOperation);
    TDVOCRoundRectNotRotExtVar(const TDVOCRoundRectNotRotExtVar&);
    ~TDVOCRoundRectNotRotExtVar() override = default;
    TDVOCRoundRectNotRotExtVar& operator=(const TDVOCRoundRectNotRotExtVar&);
    TDVOCRoundRectNotRotExtVar* Clone() const override;

    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;
    void SetRadius(double nRadius);
    double GetRadius() const;
    void SetInitializedFlag(bool bInitialized);
    bool GetInitializedFlag() const;
    void SetRectangularLock(bool bValue);
    bool GetRectangularLock() const;
    void SetRectWidth(double nRectWidth);
    double GetRectWidth() const;
    void SetRectHeight(double nRectHeight);
    double GetRectHeight() const;

private:
    TDVOCRoundRectNotRotExtVar();

    double mnRadius;
    unsigned int mnResolution;
    bool mbShowControls;
    bool mbShowPolygon;
    bool mbRectangularLock;
    bool mbInitialized;
    double mnRectWidth;
    double mnRectHeight;
};

class TDVOCRoundRectangleNotRotated : public TDVOCreate {
public:
    TDVOCRoundRectangleNotRotated(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCRoundRectangleNotRotated(const TDVOCRoundRectangleNotRotated&);
    ~TDVOCRoundRectangleNotRotated() override;
    TDVOCRoundRectangleNotRotated& operator=(const TDVOCRoundRectangleNotRotated&);
    TDVOCRoundRectangleNotRotated* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDVOCRoundRectangleNotRotated();

    TDVOCRoundRectNotRotExtVar* mpVOCRoundRectNotRotExtVar;
    TDMatRect mMatRect;
    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbRectangleOK;
    double mnRadius;
    unsigned int mnResolution;
    bool mbShowControls;
    bool mbShowPolygon;
    bool mbRectangularLock;
};

#endif
