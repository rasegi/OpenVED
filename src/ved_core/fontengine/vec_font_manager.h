#pragma once

#include "vec_maindef.h"

#include <memory>
#include <string>
#include <vector>

class TDVecFont;
using TDDocumentID = const void*;

struct TDVecFontDescriptor {
    std::string fontId;
    std::string displayName;
    bool isDefault = false;
    bool isSystemFont = false;
};

class IVecFontProvider {
public:
    virtual ~IVecFontProvider() = default;

    virtual std::vector<TDVecFontDescriptor> AvailableFonts() const = 0;
    virtual std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) = 0;
};

class TDFontManager {
public:
    void RegisterProvider(IVecFontProvider* provider);

    std::vector<TDVecFontDescriptor> AvailableFonts() const;
    TDVecFont* GetVecFontExact(const std::string& fontId, TDDocumentID docId = nullptr);
    TDVecFont* GetVecFont(const std::string& fontId, TDDocumentID docId = nullptr);
    const TDVecFont* GetDefaultVecFont() const;
    bool SetDefaultFontId(const std::string& fontId);
    const std::string& GetDefaultFontId() const;

private:
    struct TDVecDocFont {
        std::string fontId;
        TDDocumentID docId = nullptr;
        std::unique_ptr<TDVecFont> font;
    };

    TDVecDocFont* FindLoadedFont(const std::string& fontId, TDDocumentID docId);
    const TDVecDocFont* FindLoadedFont(const std::string& fontId, TDDocumentID docId) const;
    std::unique_ptr<TDVecFont> LoadFromProviders(const std::string& fontId);

    std::vector<IVecFontProvider*> providers_;
    std::vector<TDVecDocFont> fonts_;
    std::string defaultFontId_;
};
