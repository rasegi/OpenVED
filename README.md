# OpenVED

Modern 2D vector geometry and rendering engine for Qt6/C++

*In memory of [Jamshid Sharmahd](https://en.wikipedia.org/wiki/Jamshid_Sharmahd).*

---

## Overview

OpenVED is a lightweight 2D vector drawing and geometry engine originally developed in the late 1990s in Hannover, Germany.

The project has been modernized and ported to modern C++ and Qt6 while preserving its original geometric concepts and lightweight architecture.

OpenVED focuses on:

* 2D vector geometry
* precise geometric primitives
* lightweight rendering
* modern Qt integration
* clean CMake-based builds
* reduced legacy dependencies

The original codebase has been refactored to remove historical ballast, obsolete C libraries, and platform-specific legacy code.

---

## Features

* Modern C++23 implementation
* Qt6-based rendering and UI integration
* 2D vector primitives
* Geometry operations
* Lightweight architecture
* Cross-platform design
* CMake build system
* Minimal external dependencies

---

## Project Goals

OpenVED is intended as:

* a reusable 2D vector geometry engine
* a foundation for CAD-like applications
* an educational geometry/codebase
* a preservation of historical engineering software concepts modernized for current systems

---

## Technology

| Component     | Technology            |
| ------------- | --------------------- |
| Language      | C++23                 |
| GUI/Rendering | Qt6                   |
| Build System  | CMake                 |
| Platforms     | Linux, macOS, Windows |

---

## Build

### Requirements

* C++23 compatible compiler
* Qt6 Widgets
* CMake 3.24+
* Ninja (recommended)
* FreeType development files
* HarfBuzz development files

FreeType and HarfBuzz can be provided either by the system/package manager or vendored under
`third_party/freetype` and `third_party/harfbuzz`.

---

### Build Example

```bash
git clone https://github.com/YOURNAME/OpenVED.git
cd OpenVED

mkdir build
cd build

cmake .. -G Ninja
cmake --build .
```

### macOS Build Example

Using Homebrew:

```bash
brew install cmake ninja qt freetype harfbuzz

git clone https://github.com/YOURNAME/OpenVED.git
cd OpenVED

cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix freetype);$(brew --prefix harfbuzz)"

cmake --build build
```

### Windows Build Example

Using vcpkg and Visual Studio 2022:

```powershell
git clone https://github.com/YOURNAME/OpenVED.git
cd OpenVED

vcpkg install qtbase freetype harfbuzz --triplet x64-windows

cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build
```

For a fully static Windows dependency build, use a static vcpkg triplet:

```powershell
vcpkg install qtbase freetype harfbuzz --triplet x64-windows-static

cmake -S . -B build-static -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static

cmake --build build-static
```

---

## Design Principles

OpenVED intentionally avoids unnecessary complexity and large framework abstractions where possible.

The goal is to provide:

* understandable geometry code
* long-term maintainability
* lightweight architecture
* direct control over rendering and geometry structures

---

## Status

The project is currently under active modernization and cleanup.

APIs and internal structures may still evolve.

---

## License

OpenVED is released under the MIT License — see [`LICENSE`](LICENSE).

It also links against and/or bundles third-party components under their own
licenses:

- **Qt 6** (Widgets, Gui, PrintSupport) — LGPL v3
- **FreeType** — FreeType License (FTL)
- **HarfBuzz** — "Old MIT" License
- **Bundled fonts** — Liberation family (SIL OFL 1.1), optionally DejaVu
  (Bitstream Vera + public domain)

Full texts are in [`licenses/`](licenses/); attribution, linking details and
required credits are documented in
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md).

---

## Acknowledgement

This project is dedicated to the memory of
[Jamshid Sharmahd](https://en.wikipedia.org/wiki/Jamshid_Sharmahd),
a talented software engineer, independent thinker, and longtime friend.

---

## Author

Originally developed in Hannover, Germany.

Modernized and maintained by Lukas Rasegi in Hannover, Germany.
