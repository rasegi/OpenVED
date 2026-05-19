#include "ved_font_resolution.h"

#include "vec_group.h"
#include "vec_model.h"
#include "vec_object.h"
#include "vec_text.h"

namespace {

void applyResolvedFont(TDVecText& text, const TDVecFont& font)
{
    text.SetVecFontPointer(&font);
    text.SetFontName(font.GetFontName());
}

void resolveTextFont(TDVecText& text, TDFontManager& fontManager, TDDocumentID docId, VEDFontResolutionResult& result)
{
    const std::string requestedFontId = text.GetFontName() ? text.GetFontName() : "";
    if (TDVecFont* requested = fontManager.GetVecFontExact(requestedFontId, docId)) {
        applyResolvedFont(text, *requested);
        return;
    }

    if (TDVecFont* arialUnicode = fontManager.GetVecFontExact(kVEDArialUnicodeMSFontId, docId)) {
        applyResolvedFont(text, *arialUnicode);
        result.warnings.push_back({
            VEDFontFallbackKind::RequestedMissingUsedArialUnicodeMS,
            requestedFontId,
            arialUnicode->GetFontName() ? arialUnicode->GetFontName() : kVEDArialUnicodeMSFontId
        });
        return;
    }

    if (TDVecFont* wpsDefault = fontManager.GetVecFontExact(kVEDWpsDefaultFontId, nullptr)) {
        applyResolvedFont(text, *wpsDefault);
        result.warnings.push_back({
            VEDFontFallbackKind::RequestedMissingUsedWpsDefault,
            requestedFontId,
            wpsDefault->GetFontName() ? wpsDefault->GetFontName() : kVEDWpsDefaultFontId
        });
        return;
    }

    result.unresolvedFont = true;
    result.warnings.push_back({
        VEDFontFallbackKind::NoFallbackAvailable,
        requestedFontId,
        {}
    });
}

void resolveObjectFonts(TDVecObject& object, TDFontManager& fontManager, TDDocumentID docId, VEDFontResolutionResult& result)
{
    if (auto* text = dynamic_cast<TDVecText*>(&object)) {
        resolveTextFont(*text, fontManager, docId, result);
        return;
    }

    if (auto* group = dynamic_cast<TDVecGroup*>(&object)) {
        group->ForEachStoredObject([&fontManager, docId, &result](TDVecObject& child) {
            resolveObjectFonts(child, fontManager, docId, result);
        });
    }
}

} // namespace

bool VEDFontResolutionResult::Ok() const noexcept
{
    return !unresolvedFont;
}

VEDFontResolutionResult ResolveVecModelFonts(TDVecModel& model, TDFontManager& fontManager, TDDocumentID docId)
{
    VEDFontResolutionResult result;
    for (int index = 0; index < model.CountObjects(); ++index) {
        TDVecObject* object = model.GetObject(index);
        if (object) {
            resolveObjectFonts(*object, fontManager, docId, result);
        }
    }
    return result;
}
