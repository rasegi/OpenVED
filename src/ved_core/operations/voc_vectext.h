#pragma once

#include "vec_text.h"
#include "vop_base.h"

#include <memory>
#include <string>

typedef TDVecText::TDVecTextParameter TDVOCVecTextParameter_s;

class TDVOCVecTextExtVar : public TDVOPExternalVariables {
public:
    explicit TDVOCVecTextExtVar(TDViewOperation* pParentOperation);
    TDVOCVecTextExtVar(const TDVOCVecTextExtVar&);
    TDVOCVecTextExtVar& operator=(const TDVOCVecTextExtVar&);
    ~TDVOCVecTextExtVar() override = default;

    TDVOCVecTextExtVar* Clone() const override;

    void SetParameter(const TDVOCVecTextParameter_s* pVOCVecTextParameter_s);
    void GetParameter(TDVOCVecTextParameter_s* pVOCVecTextParameter_s) const;

    void SetText(const char* pText);
    const char* GetText() const;
    void DeleteText();
    bool TextIsOK() const;
    bool TextIsDeleted() const;

private:
    friend class TDVOCVecText;

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

class TDVOCVecText : public TDVOCreate {
public:
    TDVOCVecText(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOCVecText(const TDVOCVecText&);
    ~TDVOCVecText() override;
    TDVOCVecText& operator=(const TDVOCVecText&);
    TDVOCVecText* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    void SetObjText();
    bool TextIsOK() const;
    void UpdateCursor();

    TDMatPoint mOriginPoint;
    std::unique_ptr<TDVecText> mpObjText;
    TDVOCVecTextExtVar* mpVOCVecTextExtVar;
};
