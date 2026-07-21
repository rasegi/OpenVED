#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "vec_font.h"

namespace {
unsigned char byteAt(const std::vector<std::byte>& data, std::size_t index)
{
    return std::to_integer<unsigned char>(data[index]);
}
}

int main()
{
    // Load the bundled default font.
    std::ifstream file("../src/app/resources/font/wps_default.vfn", std::ios::binary);
    assert(file);
    const std::vector<char> original((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    assert(original.size() > 1028);

    std::unique_ptr<TDVecFont> font =
        LoadVecFontFromMemory(original.data(), static_cast<long>(original.size()));
    assert(font);

    // Serialize back to the .vfn byte layout.
    const std::vector<std::byte> saved = SaveVecFontToMemory(*font);

    // Structural layout: 1024 zero header, then the 'vfnt' FourCC, then stream.
    assert(saved.size() > 1028);
    for (std::size_t i = 0; i < 1024; ++i) {
        assert(saved[i] == std::byte{0});
    }
    assert(byteAt(saved, 1024) == 'v');
    assert(byteAt(saved, 1025) == 'f');
    assert(byteAt(saved, 1026) == 'n');
    assert(byteAt(saved, 1027) == 't');

    // Byte-for-byte identical to the original file: proves the writer reproduces
    // the exact .vfn format that LoadVecFontFromMemory consumes.
    assert(saved.size() == original.size());
    for (std::size_t i = 0; i < saved.size(); ++i) {
        assert(byteAt(saved, i) == static_cast<unsigned char>(original[i]));
    }

    // Semantic round-trip: reload the written bytes and compare against the source.
    std::unique_ptr<TDVecFont> reloaded =
        LoadVecFontFromMemory(saved.data(), static_cast<long>(saved.size()));
    assert(reloaded);
    assert(reloaded->CountVecCharacters() == font->CountVecCharacters());
    assert(std::fabs(reloaded->GetHeight() - font->GetHeight()) < 0.000001);
    assert(std::fabs(reloaded->GetAscent() - font->GetAscent()) < 0.000001);
    assert(std::fabs(reloaded->GetSpacingGlyphWidth() - font->GetSpacingGlyphWidth()) < 0.000001);
    assert(std::string(reloaded->GetFontName()) == font->GetFontName());

    const TDVecGlyph* glyphA = font->GetGlyphFromCharacter('A');
    assert(glyphA);
    const TDVecGlyph* reloadedA = reloaded->GetGlyphFromCharacter('A');
    assert(reloadedA);
    assert(reloadedA->CountPolyCurves() == glyphA->CountPolyCurves());
    assert(reloaded->GetGlyphFromCharacter(' '));
    assert(reloaded->GetGlyphFromCharacter(' ')->IsSpacing());

    // Idempotence: saving the reloaded font yields byte-identical output.
    const std::vector<std::byte> savedAgain = SaveVecFontToMemory(*reloaded);
    assert(savedAgain.size() == saved.size());
    for (std::size_t i = 0; i < saved.size(); ++i) {
        assert(savedAgain[i] == saved[i]);
    }

    // A non-default header size is honored on both write and read.
    const std::vector<std::byte> saved512 = SaveVecFontToMemory(*font, 512);
    assert(byteAt(saved512, 512) == 'v');
    std::unique_ptr<TDVecFont> reloaded512 =
        LoadVecFontFromMemory(saved512.data(), static_cast<long>(saved512.size()), 512);
    assert(reloaded512);
    assert(reloaded512->CountVecCharacters() == font->CountVecCharacters());

    // A zero header size still produces a valid stream (FourCC first).
    const std::vector<std::byte> savedNoHeader = SaveVecFontToMemory(*font, 0);
    assert(byteAt(savedNoHeader, 0) == 'v');
    std::unique_ptr<TDVecFont> reloadedNoHeader =
        LoadVecFontFromMemory(savedNoHeader.data(), static_cast<long>(savedNoHeader.size()), 0);
    assert(reloadedNoHeader);
    assert(reloadedNoHeader->CountVecCharacters() == font->CountVecCharacters());

    return 0;
}
