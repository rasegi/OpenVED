#include "QtVecFontProviders.h"

#include "vec_font.h"
#include "vec_polycurve.h"
#include "ttf_to_vecfont.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb-ft.h>
#include <hb.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>
#include <utility>

namespace {
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

} // namespace

struct VecShapingCache {
    QString fontId;
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    hb_font_t* hbFont = nullptr;
    double scale = 0.0;
    double hbScale = 0.0;
    double ascent = 0.0;

    ~VecShapingCache()
    {
        if (hbFont) {
            hb_font_destroy(hbFont);
        }
        if (face) {
            FT_Done_Face(face);
        }
        if (library) {
            FT_Done_FreeType(library);
        }
    }
};

TDQtSystemFontProvider::TDQtSystemFontProvider() = default;
TDQtSystemFontProvider::~TDQtSystemFontProvider() = default;

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
            const QByteArray encodedPath = QFile::encodeName(entry.path);
            return ved::fontconvert::ConvertTrueTypeFileToVecFont(
                std::string(encodedPath.constData(), static_cast<std::size_t>(encodedPath.size())),
                entry.faceIndex,
                toStdUtf8(fontIdForFamily(entry.displayName)));
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

    if (!shapingCache_ || shapingCache_->fontId != requestedFontId) {
        auto cache = std::make_unique<VecShapingCache>();
        if (FT_Init_FreeType(&cache->library) != 0 || !cache->library) {
            return false;
        }
        const QByteArray fileName = QFile::encodeName(fontEntry->path);
        if (FT_New_Face(cache->library, fileName.constData(), fontEntry->faceIndex, &cache->face) != 0 || !cache->face) {
            return false;
        }
        cache->scale = ved::fontconvert::FaceScale(cache->face);
        cache->hbScale = cache->scale / 64.0;
        cache->ascent = static_cast<double>(cache->face->ascender);
        FT_Set_Char_Size(cache->face, 0, cache->face->units_per_EM * 64, 72, 72);
        cache->hbFont = hb_ft_font_create(cache->face, nullptr);
        if (!cache->hbFont) {
            return false;
        }
        cache->fontId = requestedFontId;
        shapingCache_ = std::move(cache);
    }

    FT_Face face = shapingCache_->face;
    hb_font_t* hbFont = shapingCache_->hbFont;
    const double scale = shapingCache_->scale;
    const double hbScale = shapingCache_->hbScale;
    const double ascent = shapingCache_->ascent;

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
                shapedGlyph.glyph = ved::fontconvert::ConvertGlyphOutline(face, infos[i].codepoint, scale, ascent);
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

    return shapedAny;
}
