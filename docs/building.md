# Building the published source

The source matches Stable 2.1.2 and Pure SKSE Beta 3.0.1 for Skyrim 1.7.104 / SKSE 2.3.1. Skyrim 1.6.1170 users should use Stable 2.1.1.

## C++

Use Windows, Visual Studio 2022 with x64 C++ tools (release compiler MSVC 19.44), and CMake 3.24+. Both projects use C++23, /MD and statically linked fmt/spdlog.

From the repository root:

```powershell
./scripts/BuildDependencies.ps1 -CheckOnly
./scripts/BuildDependencies.ps1
cmake -S . -B build/stable -A x64
cmake --build build/stable --config Release
```

The dependency script builds the exact supplied source under `dependencies/` and installs it to `build/commonlib-installed`. It disables CommonLib tests, VR and IPO, enables patch safety, and uses the bundled MinHook hde64 source. It also installs the CommonLib CMake helper/version metadata needed by `find_package(CommonLibSSE 10.0.1)`.

The CommonLib source comes from 10.0.1 commit `de1ca9826919d04649e21e93358d7313459e0a68`, with the release's local export/install adjustments. The optional OpenVR SDK headers, import library and license are pinned to its submodule commit `60eb187801956ad277f1cae6680e3a410ee0873b`. No dependency needs to be cloned as a submodule. The release's dependency snapshot is preserved, rather than substituting newer versions.

Stable output: `build/stable/Data/SKSE/Plugins/SCSOProjectileBounds.dll` and `.ini`. Do not ship fmt.dll or spdlog.dll for this rebuilt plugin. Other mods may still need their own copies.

For the alternative Beta, see [../beta/README.md](../beta/README.md). Build only one variant for a player installation.

## Papyrus and ESP

Compile `src/papyrus/ultrastormcallunified.psc` with the Creation Kit Papyrus compiler, `TESV_Papyrus_Flags.flg`, Skyrim scripts and SKSE script imports. Install the resulting `Scripts/ultrastormcallunified.pex` with `StormCallShoutOverhaul.esp`. Compiler/game imports are not redistributed here.

`tools/patch_esp_vmad.py` preserves the released 2.1.1 controller integer values when preparing the existing 1.6.1 ESP. Stable 2.1.2 also sets the three Storm Call SPEL MDOB fields (`00018609`, `0001860A`, `0001860D`) to the vanilla `Skyrim.esm:000A59AC` menu object. See [esp-records.md](esp-records.md) and [release-2.1.2-validation.json](release-2.1.2-validation.json) for the exact record contract and checks.

Use the published 2.1.2 ESP as the record baseline; the repository does not redistribute the game's master files. Native/Papyrus source was restored byte-for-byte. New build instructions and CMake search defaults do not change plugin logic. A complete native rebuild and in-game test were not performed during this source sync.
