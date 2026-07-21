#include "ttf_to_vecfont.h"

#include "vec_font.h"
#include "vec_polycurve.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <cmath>
#include <memory>

namespace {
constexpr double kVedEmSize = 1000.0;
constexpr double kVedHeightFactor = 10.0;
constexpr int kFirstConvertedCharacter = 32;
constexpr int kLastConvertedCharacter = 255;

struct FreeTypeLibrary {
    FT_Library library = nullptr;

    FreeTypeLibrary()
    {
        FT_Init_FreeType(&library);
    }

    ~FreeTypeLibrary()
    {
        if (library) {
            FT_Done_FreeType(library);
        }
    }
};

struct FreeTypeFace {
    FT_Face face = nullptr;

    FreeTypeFace(FT_Library library, const char* path, long faceIndex)
    {
        FT_New_Face(library, path, faceIndex, &face);
    }

    ~FreeTypeFace()
    {
        if (face) {
            FT_Done_Face(face);
        }
    }
};

TDMatConturPoint toVedPoint(const FT_Vector* point, double scale, double ascent)
{
    return {
        static_cast<double>(point->x) * scale,
        (ascent - static_cast<double>(point->y)) * scale,
        CPT_PRIME_LINE,
        true
    };
}

bool nearlySamePoint(const TDMatConturPoint& a, const TDMatConturPoint& b)
{
    return std::fabs(a.x - b.x) < 0.000001 && std::fabs(a.y - b.y) < 0.000001;
}

struct OutlineContext {
    TDVecCharacter* character = nullptr;
    TDVecGlyph* glyph = nullptr;
    TDVecPolyCurve* current = nullptr;
    TDMatConturPoint start;
    TDMatConturPoint last;
    bool hasStart = false;
    double scale = 1.0;
    double ascent = 0.0;
};

void closeCurrentContour(OutlineContext* context)
{
    if (!context || !context->current) {
        return;
    }
    if (context->hasStart && !nearlySamePoint(context->last, context->start)) {
        TDMatConturPoint closePoint = context->start;
        closePoint.eType = CPT_PRIME_LINE;
        context->current->AppendPoint(closePoint);
    }
    if (context->character) {
        context->character->AppendPolyCurve(context->current);
    } else if (context->glyph) {
        context->glyph->AppendPolyCurve(context->current);
    } else {
        delete context->current;
    }
    context->current = nullptr;
    context->hasStart = false;
}

int moveToCallback(const FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineContext*>(user);
    closeCurrentContour(context);

    context->current = new TDVecPolyCurve();
    context->current->SetResolution(5);
    context->current->SetShowControls(false);
    context->current->SetShowPolygon(false);
    context->start = toVedPoint(to, context->scale, context->ascent);
    context->start.eType = CPT_PRIME_LINE;
    context->current->AppendPoint(context->start);
    context->last = context->start;
    context->hasStart = true;
    return 0;
}

int lineToCallback(const FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineContext*>(user);
    if (!context->current) {
        return moveToCallback(to, user);
    }
    TDMatConturPoint point = toVedPoint(to, context->scale, context->ascent);
    point.eType = CPT_PRIME_LINE;
    context->current->AppendPoint(point);
    context->last = point;
    return 0;
}

int conicToCallback(const FT_Vector* control, const FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineContext*>(user);
    if (!context->current) {
        return lineToCallback(to, user);
    }

    TDMatConturPoint controlPoint = toVedPoint(control, context->scale, context->ascent);
    controlPoint.eType = CPT_PRIME_QSPLINE;
    context->current->AppendPoint(controlPoint);

    TDMatConturPoint endPoint = toVedPoint(to, context->scale, context->ascent);
    endPoint.eType = CPT_PRIME_LINE;
    context->current->AppendPoint(endPoint);
    context->last = endPoint;
    return 0;
}

FT_Vector cubicPoint(const FT_Vector& p0, const FT_Vector& p1, const FT_Vector& p2, const FT_Vector& p3, double t)
{
    const double mt = 1.0 - t;
    const double a = mt * mt * mt;
    const double b = 3.0 * mt * mt * t;
    const double c = 3.0 * mt * t * t;
    const double d = t * t * t;
    return {
        static_cast<FT_Pos>(std::lround(a * p0.x + b * p1.x + c * p2.x + d * p3.x)),
        static_cast<FT_Pos>(std::lround(a * p0.y + b * p1.y + c * p2.y + d * p3.y))
    };
}

int cubicToCallback(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineContext*>(user);
    if (!context->current) {
        return lineToCallback(to, user);
    }

    const FT_Vector from{
        static_cast<FT_Pos>(std::lround(context->last.x / context->scale)),
        static_cast<FT_Pos>(std::lround(context->ascent - context->last.y / context->scale))
    };
    constexpr int kCubicFallbackSegments = 8;
    for (int i = 1; i <= kCubicFallbackSegments; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kCubicFallbackSegments);
        FT_Vector point = cubicPoint(from, *control1, *control2, *to, t);
        lineToCallback(&point, user);
    }
    return 0;
}

double faceScale(FT_Face face)
{
    const double unitsPerEm = face && face->units_per_EM > 0 ? static_cast<double>(face->units_per_EM) : kVedEmSize;
    return (kVedEmSize / unitsPerEm) * kVedHeightFactor;
}

std::unique_ptr<TDVecGlyph> convertGlyphOutline(FT_Face face, FT_UInt glyphIndex, double scale, double ascent)
{
    if (!face || glyphIndex == 0 ||
        FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE) != 0) {
        return nullptr;
    }

    auto glyph = std::make_unique<TDVecGlyph>();
    glyph->SetCharWidth(static_cast<double>(face->glyph->advance.x) * scale);
    if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        const FT_Outline_Funcs callbacks{
            moveToCallback,
            lineToCallback,
            conicToCallback,
            cubicToCallback,
            0,
            0
        };
        OutlineContext context;
        context.glyph = glyph.get();
        context.scale = scale;
        context.ascent = ascent;
        FT_Outline_Decompose(&face->glyph->outline, &callbacks, &context);
        closeCurrentContour(&context);
    }
    glyph->SetBaseLine(ascent * scale);
    return glyph;
}
} // namespace

namespace ved::fontconvert {

double FaceScale(FT_Face face)
{
    return faceScale(face);
}

std::unique_ptr<TDVecGlyph> ConvertGlyphOutline(FT_Face face, FT_UInt glyphIndex, double scale, double ascent)
{
    return convertGlyphOutline(face, glyphIndex, scale, ascent);
}

std::unique_ptr<TDVecFont> ConvertTrueTypeFileToVecFont(const std::string& encodedPath,
                                                        long faceIndex,
                                                        const std::string& fontName)
{
    FreeTypeLibrary freeType;
    if (!freeType.library) {
        return nullptr;
    }

    FreeTypeFace faceGuard(freeType.library, encodedPath.c_str(), faceIndex);
    FT_Face face = faceGuard.face;
    if (!face) {
        return nullptr;
    }

    const double scale = faceScale(face);
    const double ascent = static_cast<double>(face->ascender);

    auto vecFont = std::make_unique<TDVecFont>();
    vecFont->SetSpacingGlyphWidth(2000.0);
    vecFont->SetHeight(static_cast<double>(face->ascender - face->descender) * scale);
    vecFont->SetAscent(ascent * scale);
    vecFont->SetFontName(fontName.c_str());

    const FT_Outline_Funcs callbacks{
        moveToCallback,
        lineToCallback,
        conicToCallback,
        cubicToCallback,
        0,
        0
    };

    for (int character = kFirstConvertedCharacter; character <= kLastConvertedCharacter; ++character) {
        auto* vecCharacter = new TDVecCharacter();
        vecCharacter->SetUnicodeValue(static_cast<unsigned short>(character));

        const FT_UInt glyphIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(character));
        if (glyphIndex != 0 &&
            FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE) == 0) {
            vecCharacter->SetCharWidth(static_cast<double>(face->glyph->advance.x) * scale);
            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
                OutlineContext context;
                context.character = vecCharacter;
                context.scale = scale;
                context.ascent = ascent;
                FT_Outline_Decompose(&face->glyph->outline, &callbacks, &context);
                closeCurrentContour(&context);
            }
        } else {
            vecCharacter->SetCharWidth(vecFont->GetSpacingGlyphWidth());
        }

        vecCharacter->SetBaseLine(vecFont->GetAscent());
        vecFont->AppendVecCharacter(vecCharacter);
    }

    return vecFont;
}

} // namespace ved::fontconvert
