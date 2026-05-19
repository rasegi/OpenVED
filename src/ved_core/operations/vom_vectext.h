#pragma once

#include "vec_text.h"
#include "vop_base.h"

#include <string>

typedef TDVecText::TDVecTextParameter TDVOMVecTextParameter_s;

struct TDVecTextParameterValidity {
    bool mbXScale = false;
    bool mbYScale = false;
    bool mbAngle = false;
    bool mbIncline = false;
    bool mbLineSpacing = false;
    bool mbCharSpacing = false;
    bool mbVertical = false;
    bool mbUnderline = false;
    bool mbJustification = false;
    bool mbVerticalAlignment = false;
    bool mbResolution = false;
    bool mbVecFont = false;
    bool mbScaleDependency = false;
};

class TDVOMVecTextExtVar : public TDVOPExternalVariables {
public:
    explicit TDVOMVecTextExtVar(TDViewOperation* pParentOperation);
    TDVOMVecTextExtVar(const TDVOMVecTextExtVar&);
    TDVOMVecTextExtVar& operator=(const TDVOMVecTextExtVar&);
    ~TDVOMVecTextExtVar() override = default;

    TDVOMVecTextExtVar* Clone() const override;

    void SetParameter(const TDVOMVecTextParameter_s* pVOMVecTextParameter_s);
    void GetParameter(TDVOMVecTextParameter_s* pVOMVecTextParameter_s) const;

    void SetParameterValidity(const TDVecTextParameterValidity* pVOMVecTextParameter_v);
    void GetParameterValidity(TDVecTextParameterValidity* pVOMVecTextParameter_v) const;
    void SetAllParameterValidity(bool bValue);

    void SetText(const char* pText);
    const char* GetText() const;
    void DeleteText();
    bool TextIsOK() const;
    TDVecObjectType GetObjectType() const;

private:
    friend class TDVOMVecText;

    TDVecTextParameterValidity mParamValidity;
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
    unsigned int mnResolution;
    char msFontName[30];
    const TDVecFont* mpVecFont;
    bool mbScaleDependency;
    std::string msText;
    bool mbHasText;
    TDVecObjectType mObjectType;
};

class TDVOMVecText : public TDVOModify {
public:
    TDVOMVecText(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMVecText(const TDVOMVecText&);
    ~TDVOMVecText() override;
    TDVOMVecText& operator=(const TDVOMVecText&);
    TDVOMVecText* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    bool TextIsOK() const;

    TDMatPoint mOriginPoint;
    TDVOMVecTextExtVar* mpVOMVecTextExtVar;
};
