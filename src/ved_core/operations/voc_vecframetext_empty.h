#pragma once

#include "vec_text.h"
#include "vop_base.h"

#include <memory>
#include <string>

class TDVecObject;

typedef TDVecText::TDVecTextParameter TDVOCVecFrameTextEmptyParameter_s;

class TDVOCVecFrameTextEmptyExtVar : public TDVOPExternalVariables {
public:
    explicit TDVOCVecFrameTextEmptyExtVar(TDViewOperation* pParentOperation);
    TDVOCVecFrameTextEmptyExtVar(const TDVOCVecFrameTextEmptyExtVar&);
    TDVOCVecFrameTextEmptyExtVar& operator=(const TDVOCVecFrameTextEmptyExtVar&);
    ~TDVOCVecFrameTextEmptyExtVar() override = default;

    TDVOCVecFrameTextEmptyExtVar* Clone() const override;

    void SetParameter(const TDVOCVecFrameTextEmptyParameter_s* pVOCVecTextParameter_s);
    void GetParameter(TDVOCVecFrameTextEmptyParameter_s* pVOCVecTextParameter_s) const;

    void SetText(const char* pText);
    const char* GetText() const;
    void DeleteText();
    bool TextIsOK() const;
    bool TextIsDeleted() const;

private:
    friend class TDVOCVecFrameTextEmpty;

    bool mbTextIsDeleted;
    double mnXScale;
    double mnYScale;
    double mnAngle;
    double mnIncline;
    double mnLineSpacing;
    double mnCharSpacing;
    bool mbVertical;
    bool mbUnderline;
    TDVecText::TDJustification meJustification;
    TDVecText::TDVerticalAlignment meVerticalAlignment;
    char msFontName[30];
    const TDVecFont* mpVecFont;
    std::string msText;
    unsigned int mnResolution;
};

class TDVOCVecFrameTextEmpty : public TDVOCreate {
public:
    TDVOCVecFrameTextEmpty(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCVecFrameTextEmpty(const TDVOCVecFrameTextEmpty&);
    ~TDVOCVecFrameTextEmpty() override;
    TDVOCVecFrameTextEmpty& operator=(const TDVOCVecFrameTextEmpty&);
    TDVOCVecFrameTextEmpty* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDMatRect mMatRect;
    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbRectangleOK;
    TDVOCVecFrameTextEmptyExtVar* mpVOCVecFrameTextEmptyExtVar;
};
