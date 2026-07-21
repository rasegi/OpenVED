#pragma once

// Qt-free TrueType/OpenType -> VED vector font conversion.
//
// This unit isolates the FreeType-based glyph-outline conversion that used to
// live inside src/app/QtVecFontProviders.cpp so it can be shared by the Qt
// system font provider and by a standalone (Qt-free) command line converter.
// It depends only on FreeType and the VED core font model, never on Qt.

#include <ft2build.h>
#include FT_FREETYPE_H

#include <memory>
#include <string>

class TDVecFont;
class TDVecGlyph;

namespace ved::fontconvert {

// Which characters a whole-font conversion emits.
enum class CharacterCoverage {
    Latin1,    // U+0020..U+00FF only (legacy behaviour; used by the Qt provider)
    FullCmap,  // every BMP code point the font's cmap covers (U+0020..U+FFFF)
};

// Scale factor from FreeType font units to VED units for a given face.
double FaceScale(FT_Face face);

// Convert a single glyph outline (addressed by glyph index) into a TDVecGlyph.
// Returns nullptr if the glyph cannot be loaded.
std::unique_ptr<TDVecGlyph> ConvertGlyphOutline(FT_Face face, FT_UInt glyphIndex, double scale, double ascent);

// Load a TrueType/OpenType font file and convert its characters into a
// TDVecFont. Returns nullptr on failure.
//
// encodedPath: filesystem-encoded path bytes (e.g. from QFile::encodeName),
//              passed straight to FT_New_Face.
// faceIndex:   face index inside the font file (0 for single-face fonts).
// fontName:    VED font name stored in the resulting font (e.g. "TT:Arial").
// coverage:    Latin1 (default, backwards compatible) or FullCmap for the full
//              BMP range the font covers (used by the .vfn bundle converter).
std::unique_ptr<TDVecFont> ConvertTrueTypeFileToVecFont(const std::string& encodedPath,
                                                        long faceIndex,
                                                        const std::string& fontName,
                                                        CharacterCoverage coverage = CharacterCoverage::Latin1);

// Same as ConvertTrueTypeFileToVecFont but from an in-memory TTF/OTF buffer
// (e.g. a Qt resource) via FT_New_Memory_Face. The buffer only needs to stay
// valid for the duration of the call.
std::unique_ptr<TDVecFont> ConvertTrueTypeMemoryToVecFont(const void* data,
                                                          long size,
                                                          long faceIndex,
                                                          const std::string& fontName,
                                                          CharacterCoverage coverage = CharacterCoverage::Latin1);

} // namespace ved::fontconvert
