#pragma once

#include "vec_math_base.h"
#include "vec_polycurve.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class TDGraphicEngine;
class VEDBinaryReader;
class VEDBinaryWriter;

using TDPolyCurveArray = std::vector<std::unique_ptr<TDVecPolyCurve>>;

class TDVecGlyph {
public:
    TDVecGlyph();
    TDVecGlyph(const TDVecGlyph& glyph);
    ~TDVecGlyph();
    TDVecGlyph& operator=(const TDVecGlyph& glyph);
    TDVecGlyph* Clone() const;

    void WriteTo(VEDBinaryWriter& writer) const;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecGlyph> ReadFrom(VEDBinaryReader& reader);

    bool IsSpacing() const;
    void SetBaseLine(double nAscent);
    TDMatLine GetBaseLine() const;
    void SetCharWidth(double nCharWidth);
    double GetCharWidth() const;

    TDVecPolyCurve* InsertPolyCurve(int i, TDVecPolyCurve* pObject);
    TDVecPolyCurve* AppendPolyCurve(TDVecPolyCurve* pObject);
    bool RemovePolyCurve(int iObject);
    TDVecPolyCurve* DropPolyCurve(int iObject);
    bool DeletePolyCurve(int iObject);
    bool ClearPolyCurves();
    TDVecPolyCurve* GetPolyCurve(int iObject) const;
    int CountPolyCurves() const;
    void CompactPolyCurves();

    void DrawGlyph(TDGraphicEngine* pGE, bool bOutLine, unsigned int nResolution) const;
    void MoveGlyph(double X, double Y);
    void Rotate(TDMatPoint MatPoint, double nAngle);
    void TransformToPoint(TDMatPoint MatPoint);
    void TransformToOrigin(TDMatPoint MatPoint);
    void ToScale(TDMatPoint MatPoint, double xScale, double yScale);
    void ToShearX(double nAngle, int nFaktor);
    void SetColor(TDRgbColor Color);
    TDMatRect GetFrame() const;
    TDMatPoint GetMidpoint() const;
    double GetWidth() const;
    double GetHeight() const;

private:
    TDMatLine mBaseLine;
    double mnCharWidth;
    TDPolyCurveArray polyCurves_;
};

using TDVecGlyphArray = std::vector<std::unique_ptr<TDVecGlyph>>;

class TDVecCharacter {
public:
    TDVecCharacter();
    TDVecCharacter(const TDVecCharacter& character);
    ~TDVecCharacter();
    TDVecCharacter& operator=(const TDVecCharacter& character);
    TDVecCharacter* Clone() const;

    void WriteTo(VEDBinaryWriter& writer) const;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecCharacter> ReadFrom(VEDBinaryReader& reader);

    void SetUnicodeValue(unsigned short iUnicodeValue);
    unsigned short GetUnicodeValue() const;
    const TDVecGlyph* GetVecGlyph() const;

    bool IsSpacing() const;
    void SetBaseLine(double nAscent);
    TDMatLine GetBaseLine() const;
    void SetCharWidth(double nCharWidth);
    double GetCharWidth() const;

    TDVecPolyCurve* InsertPolyCurve(int i, TDVecPolyCurve* pObject);
    TDVecPolyCurve* AppendPolyCurve(TDVecPolyCurve* pObject);
    bool RemovePolyCurve(int iObject);
    TDVecPolyCurve* DropPolyCurve(int iObject);
    bool DeletePolyCurve(int iObject);
    bool ClearPolyCurves();
    TDVecPolyCurve* GetPolyCurve(int iObject) const;
    int CountPolyCurves() const;
    void CompactPolyCurves();

private:
    std::unique_ptr<TDVecGlyph> mpVecGlyph;
    unsigned short miUnicodeValue;
};

using TDVecCharacterArray = std::vector<std::unique_ptr<TDVecCharacter>>;

class TDVecFont {
public:
    TDVecFont();
    TDVecFont(const TDVecFont& font);
    ~TDVecFont();
    TDVecFont& operator=(const TDVecFont& font);
    TDVecFont* Clone() const;

    void WriteTo(VEDBinaryWriter& writer) const;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecFont> ReadFrom(VEDBinaryReader& reader);

    const TDVecGlyph* GetGlyphFromCharacter(unsigned short iUnicodeValue) const;

    void SetHeight(double nHeight);
    double GetHeight() const;
    void SetAscent(double nAscent);
    double GetAscent() const;
    void SetSpacingGlyphWidth(double nSpacingGlyphWidth);
    double GetSpacingGlyphWidth() const;
    void SetFontName(const char* sFontName);
    const char* GetFontName() const;

    TDVecCharacter* InsertVecCharacter(int i, TDVecCharacter* pObject);
    TDVecCharacter* AppendVecCharacter(TDVecCharacter* pObject);
    bool RemoveVecCharacter(int iObject);
    TDVecCharacter* DropVecCharacter(int iObject);
    bool DeleteVecCharacter(int iObject);
    bool ClearVecCharacters();
    TDVecCharacter* GetVecCharacter(int iObject) const;
    int CountVecCharacters() const;
    void CompactVecCharacters();

private:
    TDVecCharacterArray characters_;
    double mnHeight;
    double mnAscent;
    double mnSpacingGlyphWidth;
    char msFontName[30];
};

std::unique_ptr<TDVecFont> LoadVecFontFromMemory(const void* data, long size, long headerSize = 1024);

// Serialize a font into the .vfn byte layout: `headerSize` reserved (zero) bytes,
// the 'vfnt' FourCC, then the TDVecFont stream. Inverse of LoadVecFontFromMemory.
std::vector<std::byte> SaveVecFontToMemory(const TDVecFont& font, long headerSize = 1024);

// Read only the embedded font name from a .vfn buffer, without decoding glyphs.
// Returns an empty string on failure or when no name is stored (e.g. legacy
// fonts). Cheap: parses just the header fields up to the name.
std::string PeekVfnFontName(const void* data, long size, long headerSize = 1024);
