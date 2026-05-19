#pragma once

#include "vec_font_manager.h"
#include "vec_text.h"

#include <QString>

#include <memory>
#include <vector>

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

class TDQtSystemFontProvider final : public IVecFontProvider {
public:
    std::vector<TDVecFontDescriptor> AvailableFonts() const override;
    std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) override;
    bool ShapeText(const TDVecFont* font, const char* utf8Text, std::vector<TDVecShapedGlyph>& glyphs) const;

private:
    struct FontEntry {
        QString fontId;
        QString displayName;
        QString path;
        long faceIndex = 0;
    };

    void EnsureFontIndex() const;

    mutable bool fontIndexBuilt_ = false;
    mutable std::vector<FontEntry> fontIndex_;
};
