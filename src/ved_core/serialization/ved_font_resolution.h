#pragma once

#include "vec_font_manager.h"

#include <string>
#include <vector>

class TDVecModel;

inline constexpr const char* kVEDArialUnicodeMSFontId = "TT:Arial Unicode MS";
inline constexpr const char* kVEDSegoeUIFontId = "TT:Segoe UI";
inline constexpr const char* kVEDWpsDefaultFontId = "VC:WPS Default";

enum class VEDFontFallbackKind {
    None,
    RequestedMissingUsedArialUnicodeMS,
    RequestedMissingUsedSegoeUI,
    RequestedMissingUsedWpsDefault,
    NoFallbackAvailable
};

struct VEDFontResolutionWarning {
    VEDFontFallbackKind kind = VEDFontFallbackKind::None;
    std::string requestedFontId;
    std::string resolvedFontId;
};

struct VEDFontResolutionResult {
    std::vector<VEDFontResolutionWarning> warnings;
    bool unresolvedFont = false;

    [[nodiscard]] bool Ok() const noexcept;
};

VEDFontResolutionResult ResolveVecModelFonts(TDVecModel& model, TDFontManager& fontManager, TDDocumentID docId = nullptr);
