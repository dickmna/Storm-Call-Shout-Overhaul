# Building

## C++ SKSE plugin

Requirements:

- Visual Studio 2022 with Desktop development with C++
- CMake 3.24 or later
- vcpkg
- Git submodules

Clone and initialize dependencies:

```powershell
git clone --recurse-submodules https://github.com/dickmna/Storm-Call-Shout-Overhaul.git
cd Storm-Call-Shout-Overhaul
```

Configure with the vcpkg toolchain:

```powershell
cmake -S . -B build -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

Output:

```text
build/Data/SKSE/Plugins/SCSOProjectileBounds.dll
build/Data/SKSE/Plugins/SCSOProjectileBounds.ini
```

The shipping build dynamically links `fmt` and `spdlog`; copy the matching x64 release DLLs beside the plugin when constructing an install archive.

## Papyrus

Compile `src/papyrus/ultrastormcallunified.psc` with the Creation Kit Papyrus compiler against the Skyrim and SKSE source trees. The compiled file must be installed as:

```text
Data/Scripts/ultrastormcallunified.pex
```

The ESP VMAD properties are part of the runtime contract documented in [esp-records.md](esp-records.md). Compiling the PEX alone does not create or update those record attachments.
