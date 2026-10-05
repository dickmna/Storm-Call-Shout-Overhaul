# Storm Call Shout Overhaul

Stable **2.1.2** is for **Skyrim 1.7.104**, **SKSE 2.3.1** and the matching Address Library database. Skyrim **1.6.1170** users should keep **2.1.1**, available in the [Nexus Files section](https://www.nexusmods.com/skyrimspecialedition/mods/179244?tab=files).

Stable retains the ESP/Papyrus Storm Call implementation and its bounds plugin. It fixes the spell menu-object references, recompiles the script and updates the plugin for the current Skyrim runtime. Install Stable or the [3.0.1 Pure SKSE Beta](beta/README.md), never both.

## Source and build

- `src/skse/` and `src/papyrus/`: exact source restored from the published 2.1.2 archive.
- `beta/`: separate published 3.0.1 implementation and queue regression test.
- `dependencies/`: matching CommonLibSSE-NG 10.0.1 and library source, shared by both versions.
- `package/SKSE/Plugins/`: Stable configuration.
- `docs/`: build instructions, record contract and original release validation.

Build with MSVC 19.44 or a compatible Visual Studio 2022 C++ toolset and CMake 3.24+:

```powershell
./scripts/BuildDependencies.ps1
cmake -S . -B build/stable -A x64
cmake --build build/stable --config Release
```

See [docs/building.md](docs/building.md) for Papyrus and ESP preparation. Source belongs in this GitHub repository; player archives contain the runtime files and required licenses.

The source synchronization changes packaging/build documentation only. It does not introduce new gameplay logic or claim a new game test.

## Credits and licensing

Original mod and source by dickman290. Project source retains its [MIT License](LICENSE). CommonLibSSE-NG 10.0.1 uses GPL-3.0-or-later with its [Modding and Linking Exceptions](dependencies/CommonLibSSE-NG-10.0.1/EXCEPTIONS.md); the [COPYING](dependencies/CommonLibSSE-NG-10.0.1/COPYING.txt) and matching third-party licenses are retained. See [THIRD_PARTY.md](THIRD_PARTY.md).
