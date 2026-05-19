//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Rectangle-NotRotated
//---------------------------------------------------------------------------
#ifndef __VOC_RECTANGLE_NOTROTATED_H
#define __VOC_RECTANGLE_NOTROTATED_H

#include "vop_base.h"

#include <memory>

class TDVOCRectangleNotRotated : public TDVOCreate {
public:
    TDVOCRectangleNotRotated(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCRectangleNotRotated(const TDVOCRectangleNotRotated&);
    ~TDVOCRectangleNotRotated() override = default;
    TDVOCRectangleNotRotated& operator=(const TDVOCRectangleNotRotated&);
    TDVOCRectangleNotRotated* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

private:
    TDVOCRectangleNotRotated();

    TDMatRect mMatRect;
    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbRectangleOK;
};

#endif
