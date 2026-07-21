// End-to-end test of the font-conversion pipeline that ved_font_converter uses:
// real TTF -> ConvertTrueTypeFileToVecFont -> SaveVecFontToMemory ->
// LoadVecFontFromMemory. Proves a converted font is written in a form the app
// can read back. Uses a bundled source font from third_party/fonts/.

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ttf_to_vecfont.h"
#include "vec_font.h"

int main()
{
    const std::string ttf = "../third_party/fonts/liberation/LiberationSans-Regular.ttf";
    const std::string name = "VC:Liberation Sans";

    // TTF -> TDVecFont (the FreeType conversion behind the CLI).
    std::unique_ptr<TDVecFont> font =
        ved::fontconvert::ConvertTrueTypeFileToVecFont(ttf, 0, name);
    assert(font);
    assert(font->CountVecCharacters() > 0);
    assert(std::string(font->GetFontName()) == name);

    const TDVecGlyph* glyphA = font->GetGlyphFromCharacter('A');
    assert(glyphA);
    assert(!glyphA->IsSpacing());
    assert(glyphA->CountPolyCurves() > 0);

    // TDVecFont -> .vfn bytes -> TDVecFont (the save/load the CLI writes to disk).
    const std::vector<std::byte> bytes = SaveVecFontToMemory(*font);
    assert(bytes.size() > 1028);

    std::unique_ptr<TDVecFont> reloaded =
        LoadVecFontFromMemory(bytes.data(), static_cast<long>(bytes.size()));
    assert(reloaded);
    assert(reloaded->CountVecCharacters() == font->CountVecCharacters());
    assert(std::string(reloaded->GetFontName()) == name);

    const TDVecGlyph* reloadedA = reloaded->GetGlyphFromCharacter('A');
    assert(reloadedA);
    assert(reloadedA->CountPolyCurves() == glyphA->CountPolyCurves());

    // A space glyph stays a spacing glyph across the round trip.
    const TDVecGlyph* space = reloaded->GetGlyphFromCharacter(' ');
    assert(space);
    assert(space->IsSpacing());

    // Latin-1 coverage (the default) emits exactly the 32..255 range.
    assert(font->CountVecCharacters() == 224);

    // FullCmap coverage picks up non-Latin scripts. Amiri (Arabic) must then
    // carry far more characters and provide a real outline for an Arabic letter.
    std::unique_ptr<TDVecFont> arabic =
        ved::fontconvert::ConvertTrueTypeFileToVecFont(
            "../third_party/fonts/amiri/Amiri-Regular.ttf", 0, "VC:Amiri",
            ved::fontconvert::CharacterCoverage::FullCmap);
    assert(arabic);
    assert(arabic->CountVecCharacters() > 224);
    const TDVecGlyph* alef = arabic->GetGlyphFromCharacter(0x0627); // ARABIC LETTER ALEF
    assert(alef);
    assert(alef->CountPolyCurves() > 0);

    // The Latin part is still present in FullCmap mode.
    assert(arabic->GetGlyphFromCharacter('A'));

    return 0;
}
