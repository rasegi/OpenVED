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

* Modern C++20 implementation
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
| Language      | C++20                 |
| GUI/Rendering | Qt6                   |
| Build System  | CMake                 |
| Platforms     | Linux, macOS, Windows |

---

## Build

### Requirements

* C++20 compatible compiler
* Qt6
* CMake 3.20+
* Ninja (recommended)

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

OpenVED is released under the MIT License.

This project uses Qt under the LGPL license, as well as FreeType and HarfBuzz under their respective licenses.

See LICENSE and LICENSES/ for details.

---

## Acknowledgement

This project is dedicated to the memory of
[Jamshid Sharmahd](https://en.wikipedia.org/wiki/Jamshid_Sharmahd),
a talented software engineer, independent thinker, and longtime friend.

---

## Author

Originally developed in Hannover, Germany.

Modernized and maintained by Lukas Rasegi in Hannover, Germany.