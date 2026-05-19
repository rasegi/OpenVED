#include "QtVecFontProviders.h"

#include "vec_font.h"
#include "vec_polycurve.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb-ft.h>
#include <hb.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>
#include <utility>

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

    FreeTypeFace(FT_Library library, const QString& path, long faceIndex)
    {
        const QByteArray fileName = QFile::encodeName(path);
        FT_New_Face(library, fileName.constData(), faceIndex, &face);
    }

    ~FreeTypeFace()
    {
        if (face) {
            FT_Done_Face(face);
        }
    }
};

struct TextRun {
    std::size_t start = 0;
    std::size_t length = 0;
    bool rtl = false;
};

std::string toStdUtf8(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

QString trueTypeNameFromId(const std::string& fontId)
{
    constexpr char kPrefix[] = "TT:";
    if (!fontId.starts_with(kPrefix)) {
        return {};
    }
    return QString::fromUtf8(fontId.substr(3));
}

bool isRegularStyle(const QString& style)
{
    return style.compare(QStringLiteral("Regular"), Qt::CaseInsensitive) == 0 ||
           style.compare(QStringLiteral("Normal"), Qt::CaseInsensitive) == 0 ||
           style.compare(QStringLiteral("Roman"), Qt::CaseInsensitive) == 0 ||
           style.isEmpty();
}

bool isUserVisibleOutlineFace(FT_Face face, const QString& family)
{
    if (!face || family.isEmpty() || family.startsWith('.')) {
        return false;
    }
    return FT_IS_SCALABLE(face) && face->units_per_EM > 0;
}

QStringList fontSearchDirectories()
{
    QSet<QString> directories;
    for (const QString& path : QStandardPaths::standardLocations(QStandardPaths::FontsLocation)) {
        if (!path.isEmpty()) {
            directories.insert(QDir::cleanPath(path));
        }
    }

#if defined(Q_OS_WIN)
    directories.insert(QStringLiteral("C:/Windows/Fonts"));
#elif defined(Q_OS_MACOS)
    directories.insert(QStringLiteral("/System/Library/Fonts"));
    directories.insert(QStringLiteral("/Library/Fonts"));
    directories.insert(QDir::homePath() + QStringLiteral("/Library/Fonts"));
#else
    directories.insert(QStringLiteral("/usr/share/fonts"));
    directories.insert(QStringLiteral("/usr/local/share/fonts"));
    directories.insert(QDir::homePath() + QStringLiteral("/.fonts"));
    directories.insert(QDir::homePath() + QStringLiteral("/.local/share/fonts"));
#endif

    QStringList result = directories.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QString fontIdForFamily(const QString& family)
{
    return QStringLiteral("TT:%1").arg(family);
}

unsigned int decodeNextUtf8(std::string_view text, std::size_t& pos)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    const std::size_t remaining = text.size() - pos;
    const unsigned char first = bytes[pos];
    if (first < 0x80U) {
        ++pos;
        return first;
    }
    if (remaining >= 2 && (first & 0xE0U) == 0xC0U && (bytes[pos + 1] & 0xC0U) == 0x80U) {
        const unsigned int value = ((first & 0x1FU) << 6U) | (bytes[pos + 1] & 0x3FU);
        pos += 2;
        return value;
    }
    if (remaining >= 3 && (first & 0xF0U) == 0xE0U && (bytes[pos + 1] & 0xC0U) == 0x80U && (bytes[pos + 2] & 0xC0U) == 0x80U) {
        const unsigned int value = ((first & 0x0FU) << 12U) | ((bytes[pos + 1] & 0x3FU) << 6U) | (bytes[pos + 2] & 0x3FU);
        pos += 3;
        return value;
    }
    if (remaining >= 4 && (first & 0xF8U) == 0xF0U && (bytes[pos + 1] & 0xC0U) == 0x80U && (bytes[pos + 2] & 0xC0U) == 0x80U && (bytes[pos + 3] & 0xC0U) == 0x80U) {
        const unsigned int value = ((first & 0x07U) << 18U) | ((bytes[pos + 1] & 0x3FU) << 12U) | ((bytes[pos + 2] & 0x3FU) << 6U) | (bytes[pos + 3] & 0x3FU);
        pos += 4;
        return value;
    }
    ++pos;
    return first;
}

bool isRtlCodePoint(unsigned int codePoint)
{
    return (codePoint >= 0x0590U && codePoint <= 0x08FFU) ||
           (codePoint >= 0xFB1DU && codePoint <= 0xFDFFU) ||
           (codePoint >= 0xFE70U && codePoint <= 0xFEFFU);
}

bool isStrongLtrCodePoint(unsigned int codePoint)
{
    return (codePoint >= 'A' && codePoint <= 'Z') ||
           (codePoint >= 'a' && codePoint <= 'z') ||
           (codePoint >= '0' && codePoint <= '9') ||
           (codePoint >= 0x0400U && codePoint <= 0x052FU) ||
           (codePoint >= 0xAC00U && codePoint <= 0xD7AFU);
}

std::vector<TextRun> splitDirectionalRuns(std::string_view line)
{
    std::vector<TextRun> runs;
    std::size_t runStart = 0;
    bool runRtl = false;
    bool hasRunDirection = false;

    std::size_t pos = 0;
    while (pos < line.size()) {
        const std::size_t codePointStart = pos;
        const unsigned int codePoint = decodeNextUtf8(line, pos);
        const bool isRtl = isRtlCodePoint(codePoint);
        const bool isStrong = isRtl || isStrongLtrCodePoint(codePoint);
        if (!isStrong) {
            continue;
        }
        if (!hasRunDirection) {
            runRtl = isRtl;
            hasRunDirection = true;
            continue;
        }
        if (isRtl != runRtl) {
            runs.push_back({runStart, codePointStart - runStart, runRtl});
            runStart = codePointStart;
            runRtl = isRtl;
        }
    }

    if (runStart < line.size()) {
        runs.push_back({runStart, line.size() - runStart, hasRunDirection && runRtl});
    }
    return runs;
}

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

std::unique_ptr<TDVecFont> convertFontFileWithFreeType(const QString& path, long faceIndex, const QString& family)
{
    FreeTypeLibrary freeType;
    if (!freeType.library) {
        return nullptr;
    }

    FreeTypeFace faceGuard(freeType.library, path, faceIndex);
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
    const QByteArray fontName = fontIdForFamily(family).toUtf8();
    vecFont->SetFontName(fontName.constData());

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
} // namespace

TDBuiltinVfnFontProvider::TDBuiltinVfnFontProvider(QString fontId, QString displayName, QString resourcePath)
    : fontId_(std::move(fontId)),
      displayName_(std::move(displayName)),
      resourcePath_(std::move(resourcePath))
{
}

std::vector<TDVecFontDescriptor> TDBuiltinVfnFontProvider::AvailableFonts() const
{
    return {{
        toStdUtf8(fontId_),
        toStdUtf8(displayName_),
        true,
        false
    }};
}

std::unique_ptr<TDVecFont> TDBuiltinVfnFontProvider::LoadFont(const std::string& fontId)
{
    if (fontId != toStdUtf8(fontId_)) {
        return nullptr;
    }

    QFile file(resourcePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    const QByteArray data = file.readAll();
    return LoadVecFontFromMemory(data.constData(), static_cast<long>(data.size()));
}

void TDQtSystemFontProvider::EnsureFontIndex() const
{
    if (fontIndexBuilt_) {
        return;
    }
    fontIndexBuilt_ = true;

    FreeTypeLibrary freeType;
    if (!freeType.library) {
        return;
    }

    QSet<QString> registeredFamilies;
    const QStringList filters{
        QStringLiteral("*.ttf"),
        QStringLiteral("*.ttc"),
        QStringLiteral("*.otf"),
        QStringLiteral("*.otc")
    };

    for (const QString& directory : fontSearchDirectories()) {
        if (!QDir(directory).exists()) {
            continue;
        }
        QDirIterator it(directory, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            const QByteArray fileName = QFile::encodeName(path);
            FT_Face firstFace = nullptr;
            if (FT_New_Face(freeType.library, fileName.constData(), 0, &firstFace) != 0 || !firstFace) {
                continue;
            }
            const long faceCount = std::max<long>(1, firstFace->num_faces);
            FT_Done_Face(firstFace);

            for (long faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
                FT_Face face = nullptr;
                if (FT_New_Face(freeType.library, fileName.constData(), faceIndex, &face) != 0 || !face) {
                    continue;
                }
                const QString family = QString::fromUtf8(face->family_name ? face->family_name : "");
                const QString style = QString::fromUtf8(face->style_name ? face->style_name : "");
                if (!isUserVisibleOutlineFace(face, family) || registeredFamilies.contains(family)) {
                    FT_Done_Face(face);
                    continue;
                }
                if (!isRegularStyle(style)) {
                    FT_Done_Face(face);
                    continue;
                }

                registeredFamilies.insert(family);
                fontIndex_.push_back({
                    fontIdForFamily(family),
                    family,
                    path,
                    faceIndex
                });
                FT_Done_Face(face);
            }
        }
    }

    std::sort(fontIndex_.begin(), fontIndex_.end(), [](const FontEntry& a, const FontEntry& b) {
        return QString::compare(a.displayName, b.displayName, Qt::CaseInsensitive) < 0;
    });
}

std::vector<TDVecFontDescriptor> TDQtSystemFontProvider::AvailableFonts() const
{
    EnsureFontIndex();

    std::vector<TDVecFontDescriptor> fonts;
    fonts.reserve(fontIndex_.size());
    for (const FontEntry& entry : fontIndex_) {
        fonts.push_back({
            toStdUtf8(entry.fontId),
            toStdUtf8(entry.displayName),
            false,
            true
        });
    }
    return fonts;
}

std::unique_ptr<TDVecFont> TDQtSystemFontProvider::LoadFont(const std::string& fontId)
{
    const QString family = trueTypeNameFromId(fontId);
    if (family.isEmpty()) {
        return nullptr;
    }

    EnsureFontIndex();
    const QString requestedFontId = fontIdForFamily(family);
    for (const FontEntry& entry : fontIndex_) {
        if (entry.fontId == requestedFontId) {
            return convertFontFileWithFreeType(entry.path, entry.faceIndex, entry.displayName);
        }
    }
    return nullptr;
}

bool TDQtSystemFontProvider::ShapeText(const TDVecFont* font, const char* utf8Text, std::vector<TDVecShapedGlyph>& glyphs) const
{
    if (!font || !utf8Text || !font->GetFontName()) {
        return false;
    }

    const QString family = trueTypeNameFromId(font->GetFontName());
    if (family.isEmpty()) {
        return false;
    }

    EnsureFontIndex();
    const QString requestedFontId = fontIdForFamily(family);
    const FontEntry* fontEntry = nullptr;
    for (const FontEntry& entry : fontIndex_) {
        if (entry.fontId == requestedFontId) {
            fontEntry = &entry;
            break;
        }
    }
    if (!fontEntry) {
        return false;
    }

    FreeTypeLibrary freeType;
    if (!freeType.library) {
        return false;
    }
    FreeTypeFace faceGuard(freeType.library, fontEntry->path, fontEntry->faceIndex);
    FT_Face face = faceGuard.face;
    if (!face) {
        return false;
    }

    const double scale = faceScale(face);
    const double hbScale = scale / 64.0;
    const double ascent = static_cast<double>(face->ascender);
    FT_Set_Char_Size(face, 0, face->units_per_EM * 64, 72, 72);
    hb_font_t* hbFont = hb_ft_font_create(face, nullptr);
    if (!hbFont) {
        return false;
    }

    const std::string_view text(utf8Text);
    std::size_t lineStart = 0;
    bool shapedAny = false;
    while (lineStart <= text.size()) {
        const std::size_t lineEnd = text.find('\n', lineStart);
        const std::size_t length = (lineEnd == std::string_view::npos ? text.size() : lineEnd) - lineStart;
        const std::string_view line(text.data() + lineStart, length);

        for (const TextRun& run : splitDirectionalRuns(line)) {
            if (run.length == 0) {
                continue;
            }
            hb_buffer_t* buffer = hb_buffer_create();
            hb_buffer_add_utf8(buffer, line.data() + run.start, static_cast<int>(run.length), 0, static_cast<int>(run.length));
            hb_buffer_set_direction(buffer, run.rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
            hb_buffer_set_script(buffer, run.rtl ? HB_SCRIPT_ARABIC : HB_SCRIPT_LATIN);
            hb_buffer_set_language(buffer, run.rtl ? hb_language_from_string("fa", 2) : hb_language_from_string("en", 2));
            hb_shape(hbFont, buffer, nullptr, 0);

            unsigned int glyphCount = 0;
            hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
            hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyphCount);
            std::vector<TDVecShapedGlyph> runGlyphs;
            runGlyphs.reserve(glyphCount);
            for (unsigned int i = 0; i < glyphCount; ++i) {
                TDVecShapedGlyph shapedGlyph;
                shapedGlyph.glyph = convertGlyphOutline(face, infos[i].codepoint, scale, ascent);
                shapedGlyph.xOffset = static_cast<double>(positions[i].x_offset) * hbScale;
                shapedGlyph.yOffset = -static_cast<double>(positions[i].y_offset) * hbScale;
                shapedGlyph.xAdvance = static_cast<double>(positions[i].x_advance) * hbScale;
                shapedGlyph.yAdvance = -static_cast<double>(positions[i].y_advance) * hbScale;
                runGlyphs.push_back(std::move(shapedGlyph));
                shapedAny = true;
            }
            hb_buffer_destroy(buffer);

            if (run.rtl) {
                // HarfBuzz already returns Arabic/Farsi glyphs in visual RTL
                // order for an RTL buffer. Keep that order and let VED advance
                // normally through the shaped run.
                for (TDVecShapedGlyph& shapedGlyph : runGlyphs) {
                    glyphs.push_back(std::move(shapedGlyph));
                }
            } else {
                for (TDVecShapedGlyph& shapedGlyph : runGlyphs) {
                    glyphs.push_back(std::move(shapedGlyph));
                }
            }
        }

        if (lineEnd == std::string_view::npos) {
            break;
        }
        TDVecShapedGlyph lineBreak;
        lineBreak.lineBreak = true;
        glyphs.push_back(std::move(lineBreak));
        lineStart = lineEnd + 1;
    }

    hb_font_destroy(hbFont);
    return shapedAny;
}
