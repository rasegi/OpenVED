#include "vec_font_manager.h"

#include "vec_font.h"

#include <algorithm>

void TDFontManager::RegisterProvider(IVecFontProvider* provider)
{
    if (!provider) {
        return;
    }
    if (std::find(providers_.begin(), providers_.end(), provider) != providers_.end()) {
        return;
    }
    providers_.push_back(provider);
}

std::vector<TDVecFontDescriptor> TDFontManager::AvailableFonts() const
{
    std::vector<TDVecFontDescriptor> fonts;
    for (const IVecFontProvider* provider : providers_) {
        if (!provider) {
            continue;
        }
        std::vector<TDVecFontDescriptor> providerFonts = provider->AvailableFonts();
        fonts.insert(fonts.end(), providerFonts.begin(), providerFonts.end());
    }
    return fonts;
}

TDVecFont* TDFontManager::GetVecFontExact(const std::string& fontId, TDDocumentID docId)
{
    if (fontId.empty()) {
        return nullptr;
    }

    if (TDVecDocFont* loaded = FindLoadedFont(fontId, docId)) {
        return loaded->font.get();
    }
    if (docId && FindLoadedFont(fontId, nullptr)) {
        return FindLoadedFont(fontId, nullptr)->font.get();
    }

    std::unique_ptr<TDVecFont> font = LoadFromProviders(fontId);
    if (!font) {
        return nullptr;
    }

    TDVecDocFont docFont;
    docFont.fontId = fontId;
    docFont.docId = docId;
    docFont.font = std::move(font);
    fonts_.push_back(std::move(docFont));
    return fonts_.back().font.get();
}

TDVecFont* TDFontManager::GetVecFont(const std::string& fontId, TDDocumentID docId)
{
    const std::string requestedFontId = fontId.empty() ? defaultFontId_ : fontId;
    if (requestedFontId.empty()) {
        return nullptr;
    }

    if (TDVecFont* font = GetVecFontExact(requestedFontId, docId)) {
        return font;
    }
    return const_cast<TDVecFont*>(GetDefaultVecFont());
}

const TDVecFont* TDFontManager::GetDefaultVecFont() const
{
    if (defaultFontId_.empty()) {
        return nullptr;
    }
    if (const TDVecDocFont* loaded = FindLoadedFont(defaultFontId_, nullptr)) {
        return loaded->font.get();
    }
    return nullptr;
}

bool TDFontManager::SetDefaultFontId(const std::string& fontId)
{
    if (fontId.empty()) {
        return false;
    }
    defaultFontId_ = fontId;
    return GetVecFont(defaultFontId_, nullptr) != nullptr;
}

const std::string& TDFontManager::GetDefaultFontId() const
{
    return defaultFontId_;
}

TDFontManager::TDVecDocFont* TDFontManager::FindLoadedFont(const std::string& fontId, TDDocumentID docId)
{
    for (TDVecDocFont& font : fonts_) {
        if (font.fontId == fontId && font.docId == docId) {
            return &font;
        }
    }
    return nullptr;
}

const TDFontManager::TDVecDocFont* TDFontManager::FindLoadedFont(const std::string& fontId, TDDocumentID docId) const
{
    for (const TDVecDocFont& font : fonts_) {
        if (font.fontId == fontId && font.docId == docId) {
            return &font;
        }
    }
    return nullptr;
}

std::unique_ptr<TDVecFont> TDFontManager::LoadFromProviders(const std::string& fontId)
{
    for (IVecFontProvider* provider : providers_) {
        if (!provider) {
            continue;
        }
        std::unique_ptr<TDVecFont> font = provider->LoadFont(fontId);
        if (font) {
            return font;
        }
    }
    return nullptr;
}
