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

// Scale factor from FreeType font units to VED units for a given face.
double FaceScale(FT_Face face);

// Convert a single glyph outline (addressed by glyph index) into a TDVecGlyph.
// Returns nullptr if the glyph cannot be loaded.
std::unique_ptr<TDVecGlyph> ConvertGlyphOutline(FT_Face face, FT_UInt glyphIndex, double scale, double ascent);

// Load a TrueType/OpenType font file and convert its supported character range
// into a TDVecFont. Returns nullptr on failure.
//
// encodedPath: filesystem-encoded path bytes (e.g. from QFile::encodeName),
//              passed straight to FT_New_Face.
// faceIndex:   face index inside the font file (0 for single-face fonts).
// fontName:    VED font name stored in the resulting font (e.g. "TT:Arial").
std::unique_ptr<TDVecFont> ConvertTrueTypeFileToVecFont(const std::string& encodedPath,
                                                        long faceIndex,
                                                        const std::string& fontName);

} // namespace ved::fontconvert
