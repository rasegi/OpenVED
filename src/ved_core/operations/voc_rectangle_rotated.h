//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Rectangle-Rotated
//---------------------------------------------------------------------------
#ifndef __VOC_RECTANGLE_ROTATED_H
#define __VOC_RECTANGLE_ROTATED_H

#include "vop_base.h"

#include <memory>

class TDVOCRectangleRotated : public TDVOCreate {
public:
    TDVOCRectangleRotated(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCRectangleRotated(const TDVOCRectangleRotated&);
    ~TDVOCRectangleRotated() override = default;
    TDVOCRectangleRotated& operator=(const TDVOCRectangleRotated&);
    TDVOCRectangleRotated* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCRectangleRotated();

    TDMatRect mMatRect;
    TDMatLine mMatLine;
    std::unique_ptr<TDVecObject> mpTmpObject;
    double mnAngle;
    bool mbRotational;
    bool mbRectangleOK;
};

#endif
