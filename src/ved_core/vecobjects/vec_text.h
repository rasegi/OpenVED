#pragma once

#include "vec_font.h"
#include "vec_object.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

struct TDVecShapedGlyph {
    std::unique_ptr<TDVecGlyph> glyph;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double xAdvance = 0.0;
    double yAdvance = 0.0;
    bool lineBreak = false;
};

// Optional bridge for platform text shaping. The core owns no HarfBuzz/Qt
// dependency; app code may provide already shaped VED glyphs for TT fonts.
using TDVecTextShaper = std::function<bool(const TDVecFont*, const char*, std::vector<TDVecShapedGlyph>&)>;

class TDVecText : public TDVecObject {
public:
    enum TDJustification {
        JUST_UNKNOWN = -1,
        JUST_LEFT = 0,
        JUST_CENTER = 1,
        JUST_RIGHT = 2,
    };

    enum TDVerticalAlignment {
        VALIGN_UNKNOWN = -1,
        VALIGN_TOP = 0,
        VALIGN_CENTER = 1,
        VALIGN_BOTTOM = 2,
    };

    struct TDVecTextParameter {
        double mnXScale = 1.0;
        double mnYScale = 1.0;
        double mnAngle = 0.0;
        double mnIncline = 0.0;
        double mnLineSpacing = 1.0;
        double mnCharSpacing = 1.0;
        bool mbVertical = false;
        bool mbUnderline = false;
        TDJustification meJustification = JUST_LEFT;
        TDVerticalAlignment meVerticalAlignment = VALIGN_TOP;
        unsigned int mnResolution = 4;
        char msFontName[30] = {};
        const TDVecFont* mpVecFont = nullptr;
        bool mbScaleDependency = true;
    };

    TDVecText();
    TDVecText(const TDVecText& other);
    TDVecText& operator=(const TDVecText& other);
    ~TDVecText() override;

    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecText> ReadFrom(VEDBinaryReader& reader);

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    void DrawNodes(TDGraphicEngine* pGE) const override;
    TDMatRect GetFrame() const override;
    TDMatPoint GetMidpoint() const override;
    std::unique_ptr<TDVecObject> Clone() const override;
    TDVecLineHitResult HitTest(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const override;
    long PointOnNode(TDMatPoint point, double tolerance) const override;
    bool MoveBy(double dx, double dy) override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void Initialize() override;
    void SetText(const char* sText);
    const char* GetText() const;
    void SetOriginPoint(TDMatPoint OriginPoint);
    TDMatPoint GetOriginPoint() const;
    void SetXScale(double nXScale);
    double GetXScale() const;
    void SetYScale(double nYScale);
    double GetYScale() const;
    void SetAngle(double nAngle);
    double GetAngle() const;
    void SetIncline(double nIncline);
    double GetIncline() const;
    void SetLineSpacing(double nLineSpacing);
    double GetLineSpacing() const;
    void SetCharSpacing(double nCharSpacing);
    double GetCharSpacing() const;
    void SetVertical(bool bVertical);
    bool GetVertical() const;
    void SetUnderline(bool bUnderline);
    bool GetUnderline() const;
    void SetJustification(TDJustification eJustification);
    TDJustification GetJustification() const;
    void SetVerticalAlignment(TDVerticalAlignment eVerticalAlignment);
    TDVerticalAlignment GetVerticalAlignment() const;
    void SetFontName(const char* sFontName);
    const char* GetFontName() const;
    void SetVecFontPointer(const TDVecFont* pVecFont);
    const TDVecFont* GetVecFontPointer() const;
    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    virtual void SetScaleDependency(bool bValue);
    virtual bool GetScaleDependency() const;
    virtual void SetParameter(const TDVecTextParameter* pVecTextParameter);
    virtual void GetParameter(TDVecTextParameter* pVecTextParameter) const;
    int CountGlyphs() const;

protected:
    TDMatPoint mOriginPoint;
    double mnXScale;
    double mnYScale;
    double mnAngle;
    double mnIncline;
    double mnLineSpacing;
    double mnCharSpacing;
    bool mbVertical;
    bool mbUnderline;
    TDJustification meJustification;
    TDVerticalAlignment meVerticalAlignment;
    unsigned int mnResolution;
    char msFontName[30];
    const TDVecFont* mpVecFont;
    char* msText;
    TDVecGlyphArray glyphs_;

    void InitializeOriginPoint(TDMatPoint OriginPoint);
    virtual void InitializeGlyphs();
    void CopyFrom(const TDVecText& other);
};

class TDVecFrameText : public TDVecText {
public:
    TDVecFrameText();
    TDVecFrameText(const TDVecFrameText& other);
    TDVecFrameText& operator=(const TDVecFrameText& other);
    ~TDVecFrameText() override = default;

    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecFrameText> ReadFrom(VEDBinaryReader& reader);

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    void DrawNodes(TDGraphicEngine* pGE) const override;
    TDMatRect GetFrame() const override;
    TDMatPoint GetMidpoint() const override;
    std::unique_ptr<TDVecObject> Clone() const override;
    bool MoveBy(double dx, double dy) override;
    long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const override;
    long PointOnNode(TDMatPoint point, double tolerance) const override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void SetRectangle(const TDMatRect* pRectangle);
    TDMatRect GetRectangle() const;
    TDMatRect GetTextFrame() const;
    void SetScaleDependency(bool bValue) override;
    bool GetScaleDependency() const override;
    void SetParameter(const TDVecTextParameter* pVecTextParameter) override;
    void GetParameter(TDVecTextParameter* pVecTextParameter) const override;

protected:
    void InitializeGlyphs() override;

private:
    TDMatRect mRectangle;
    bool mbOriginIsjustified;
    bool mbScaleDependency;
    void JustifyOriginPoint();
};

bool IsStringBlank(const char* s);
void SetDefaultVecTextFont(const TDVecFont* font);
const TDVecFont* GetDefaultVecTextFont();
void SetVecTextShaper(TDVecTextShaper shaper);
