// ved_font_converter — native build-time tool that converts a TrueType/OpenType
// font into VED's .vfn vector font format.
//
// It wires together the two building blocks from story 15:
//   - ved::fontconvert::ConvertTrueTypeFileToVecFont  (TTF/OTF -> TDVecFont)
//   - SaveVecFontToMemory                              (TDVecFont -> .vfn bytes)
// and writes the resulting bytes to disk. Qt-free by design (only FreeType +
// VED core), so it is not part of the WebAssembly build.

#include "ttf_to_vecfont.h"
#include "vec_font.h"

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

void printUsage(const char* prog)
{
    std::cerr
        << "Usage: " << prog << " --in <font.ttf|otf> --out <font.vfn> --name <font-name>\n"
        << "                        [--face <index>] [--header <bytes>]\n\n"
        << "Convert a TrueType/OpenType font into VED's .vfn vector font format.\n"
        << "  --in      input font file (TTF/OTF)                (required)\n"
        << "  --out     output .vfn file                          (required)\n"
        << "  --name    font name stored in the .vfn,\n"
        << "            e.g. \"VC:Liberation Sans\"                  (required)\n"
        << "  --face    face index inside the font file           (default 0)\n"
        << "  --header  reserved header size in bytes             (default 1024)\n";
}

// Consume the value that follows a --flag; returns false if none is present.
bool takeValue(int argc, char** argv, int& i, std::string& out)
{
    if (i + 1 >= argc) {
        return false;
    }
    out = argv[++i];
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    std::string inPath;
    std::string outPath;
    std::string name;
    long faceIndex = 0;
    long headerSize = 1024;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;
        if (arg == "--in") {
            if (!takeValue(argc, argv, i, inPath)) { std::cerr << "error: --in requires a value\n"; return 2; }
        } else if (arg == "--out") {
            if (!takeValue(argc, argv, i, outPath)) { std::cerr << "error: --out requires a value\n"; return 2; }
        } else if (arg == "--name") {
            if (!takeValue(argc, argv, i, name)) { std::cerr << "error: --name requires a value\n"; return 2; }
        } else if (arg == "--face") {
            if (!takeValue(argc, argv, i, value)) { std::cerr << "error: --face requires a value\n"; return 2; }
            faceIndex = std::strtol(value.c_str(), nullptr, 10);
        } else if (arg == "--header") {
            if (!takeValue(argc, argv, i, value)) { std::cerr << "error: --header requires a value\n"; return 2; }
            headerSize = std::strtol(value.c_str(), nullptr, 10);
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "error: unknown argument '" << arg << "'\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    if (inPath.empty() || outPath.empty() || name.empty()) {
        std::cerr << "error: --in, --out and --name are required\n";
        printUsage(argv[0]);
        return 2;
    }
    if (headerSize < 0) {
        std::cerr << "error: --header must be >= 0\n";
        return 2;
    }

    std::unique_ptr<TDVecFont> font =
        ved::fontconvert::ConvertTrueTypeFileToVecFont(inPath, faceIndex, name);
    if (!font) {
        std::cerr << "error: failed to load or convert '" << inPath
                  << "' (face " << faceIndex << ")\n";
        return 1;
    }
    if (font->CountVecCharacters() <= 0) {
        std::cerr << "error: converted font has no characters\n";
        return 1;
    }

    const std::vector<std::byte> bytes = SaveVecFontToMemory(*font, headerSize);

    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "error: cannot open output '" << outPath << "' for writing\n";
        return 1;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        std::cerr << "error: failed while writing '" << outPath << "'\n";
        return 1;
    }

    std::cout << "ok: " << inPath << " -> " << outPath << "  ("
              << font->CountVecCharacters() << " chars, "
              << bytes.size() << " bytes, name=\"" << name << "\")\n";
    return 0;
}
