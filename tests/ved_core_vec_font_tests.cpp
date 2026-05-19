#include <cassert>
#include <cmath>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_font.h"

int main()
{
    std::ifstream file("../src/app/resources/font/wps_default.vfn", std::ios::binary);
    assert(file);

    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    assert(data.size() == 115309);
    assert(data.size() > 1028);
    assert(data[1024] == 'v');
    assert(data[1025] == 'f');
    assert(data[1026] == 'n');
    assert(data[1027] == 't');

    std::unique_ptr<TDVecFont> font = LoadVecFontFromMemory(data.data(), static_cast<long>(data.size()));
    assert(font);
    assert(font->GetHeight() > 0.0);
    assert(std::fabs(font->GetAscent()) > 0.0);
    assert(font->GetSpacingGlyphWidth() > 0.0);
    assert(font->CountVecCharacters() > 100);

    const TDVecGlyph* glyphA = font->GetGlyphFromCharacter('A');
    assert(glyphA);
    assert(!glyphA->IsSpacing());
    assert(glyphA->CountPolyCurves() > 0);

    const TDVecGlyph* glyphSpace = font->GetGlyphFromCharacter(' ');
    assert(glyphSpace);
    assert(glyphSpace->IsSpacing());

    assert(font->GetGlyphFromCharacter(0xE4)); // ae
    assert(font->GetGlyphFromCharacter(0xF6)); // oe
    assert(font->GetGlyphFromCharacter(0xFC)); // ue
    assert(font->GetGlyphFromCharacter(0xDF)); // sz

    TDVecCharacter* first = font->GetVecCharacter(0);
    assert(first);
    assert(first->GetVecGlyph());

    VEDBinaryWriter writer;
    writer.WriteFourCC(TDVecFont::StreamFourCC());
    font->WriteTo(writer);
    assert(writer.Size() > 1024);

    VEDBinaryReader reader(writer.Bytes());
    assert(reader.ReadFourCC() == TDVecFont::StreamFourCC());
    std::unique_ptr<TDVecFont> roundTrip = TDVecFont::ReadFrom(reader);
    assert(reader.Ok());
    assert(roundTrip);
    assert(roundTrip->CountVecCharacters() == font->CountVecCharacters());
    assert(std::fabs(roundTrip->GetHeight() - font->GetHeight()) < 0.000001);
    assert(std::fabs(roundTrip->GetAscent() - font->GetAscent()) < 0.000001);
    assert(std::fabs(roundTrip->GetSpacingGlyphWidth() - font->GetSpacingGlyphWidth()) < 0.000001);
    assert(std::string(roundTrip->GetFontName()) == font->GetFontName());
    assert(roundTrip->GetGlyphFromCharacter('A'));
    assert(roundTrip->GetGlyphFromCharacter('A')->CountPolyCurves() == glyphA->CountPolyCurves());
    assert(roundTrip->GetGlyphFromCharacter(' '));
    assert(roundTrip->GetGlyphFromCharacter(' ')->IsSpacing());

    return 0;
}
