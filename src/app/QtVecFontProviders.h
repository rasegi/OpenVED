#pragma once

#include "vec_font_manager.h"
#include "vec_text.h"

#include <QString>

#include <memory>
#include <vector>

struct VecShapingCache;

class TDBuiltinVfnFontProvider final : public IVecFontProvider {
public:
    TDBuiltinVfnFontProvider(QString fontId, QString displayName, QString resourcePath);

    std::vector<TDVecFontDescriptor> AvailableFonts() const override;
    std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) override;

private:
    QString fontId_;
    QString displayName_;
    QString resourcePath_;
};

// Loads and shapes TrueType/OpenType fonts. Bundled TTF (Qt resources under
// :/ved/font) are always available; installed system fonts are only indexed
// when includeSystemFonts is true (controlled by the "Convert System Fonts"
// switch). Bundled fonts use the "Ved:" id prefix, system fonts "Sys:".
class TDQtSystemFontProvider final : public IVecFontProvider {
public:
    explicit TDQtSystemFontProvider(bool includeSystemFonts = false);
    ~TDQtSystemFontProvider() override;

    std::vector<TDVecFontDescriptor> AvailableFonts() const override;
    std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) override;
    bool ShapeText(const TDVecFont* font, const char* utf8Text, std::vector<TDVecShapedGlyph>& glyphs) const;

private:
    struct FontEntry {
        QString fontId;
        QString displayName;
        QString path;         // filesystem path (system) or ":/..." resource path (bundled)
        long faceIndex = 0;
        bool isResource = false;
    };

    void EnsureFontIndex() const;

    bool includeSystemFonts_ = false;
    mutable bool fontIndexBuilt_ = false;
    mutable std::vector<FontEntry> fontIndex_;
    mutable std::unique_ptr<VecShapingCache> shapingCache_;
};
