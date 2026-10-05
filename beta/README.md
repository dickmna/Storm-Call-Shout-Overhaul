# Storm Call 3.0.1 Pure SKSE Beta

Experimental pure SKSE alternative for Skyrim 1.7.104, SKSE 2.3.1 and the matching Address Library database. Use a test save and install this or Stable, never both. Skyrim 1.6.1170 users should keep Stable 2.1.1.

The source in `src/skse/` is restored from the published 3.0.1 package. Dependencies are shared with Stable in `../dependencies/`.

## Build

Build the source dependencies once from the repository root with `./scripts/BuildDependencies.ps1`. Then, from the repository root:

```powershell
cmake -S beta -B build/beta -A x64
cmake --build build/beta --config Release
ctest --test-dir build/beta -C Release --output-on-failure
```

The output is `build/beta/Data/SKSE/Plugins/Release/SCSOStormCall.dll` for the Visual Studio generator. Install it with `package/SKSE/Plugins/SCSOStormCall.ini`. Do not include the Stable ESP, PEX or bounds plugin.

The original release's build validation is in [../docs/release-3.0.1-validation.json](../docs/release-3.0.1-validation.json). The source sync does not introduce gameplay changes or claim a new game test.
