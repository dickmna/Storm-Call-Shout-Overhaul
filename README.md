# Storm Call Shout Overhaul

Storm Call Shout Overhaul (SCSO) is a standalone Skyrim Special Edition / Anniversary Edition overhaul for the vanilla Storm Call shout. The current source release is **v2.1.0**.

The storm follows the shouter, dynamically searches a 600-foot three-dimensional sphere, selects hostile actors only, supports flying targets, and can strike several different enemies during one update. Damage comes from the final active-effect magnitude, so shout overhauls can scale SCSO through normal magnitude changes instead of an SCSO-specific dragon-soul formula.

## Current behavior

- Initial spell area by word: `100 / 300 / 600` feet.
- Dynamic search radius for every word: `600` magic feet, converted to `12800` Papyrus world units.
- Duration by word: `60 / 120 / 180` seconds.
- Standalone base magnitude by word: `30 / 60 / 120`.
- No fixed per-pass target cap; a pass continues until it cannot acquire another unstruck hostile actor.
- Tracker mode retains or replaces the initial target.
- Active controller mode performs `1 / 2 / 3` dynamic passes by shout word.
- Two- and three-pass updates retain the v1.6.1 `0.12-0.35` second stagger between consecutive passes.
- Main update delay is `1.50-3.00` seconds.
- Target searches and lightning strikes pause in interior cells and resume after the shouter returns outside.
- The real Storm Call bolt spells are zero-damage visual carriers; the unified script applies the shared final magnitude once.
- The SKSE plugin changes only the loaded 3D bounds of `ShockBoltAimStorm` (`PROJ 000E4CB5`). It does not alter targeting, timing, damage, range, or scaling.

See [docs/architecture.md](docs/architecture.md) for the targeting and damage flow and [docs/esp-records.md](docs/esp-records.md) for the plugin record contract.

## Requirements

- Skyrim Special Edition or Anniversary Edition
- SKSE64
- Address Library for SKSE Plugins

Forceful Tongue is not required. The ESP masters are only `Skyrim.esm` and `Update.esm`.

## Installation

Download the install archive from [GitHub Releases](https://github.com/dickmna/Storm-Call-Shout-Overhaul/releases), install it with Mod Organizer 2 or Vortex, and enable `StormCallShoutOverhaul.esp`.

Load the ESP after mods that edit the vanilla Storm Call spells, magic effects, bolt spells, or projectile when SCSO should win those records. A conflict-resolution patch may be necessary when another shout overhaul must provide the final magnitudes.

## Source layout

- `src/papyrus/ultrastormcallunified.psc`: unified tracker and active-search controller.
- `src/skse/main.cpp`: bounds-only SKSE plugin.
- `package/SKSE/Plugins/SCSOProjectileBounds.ini`: runtime bounds settings.
- `docs/`: architecture, ESP record contract, and build notes.

The installable ESP, compiled PEX, compiled DLL, and runtime dependencies are distributed as release assets rather than committed binaries.

## Building

Clone with submodules and follow [docs/building.md](docs/building.md). The C++ project is pinned to CommonLibSSE-NG and uses vcpkg for `spdlog` and `rapidcsv`.

Papyrus compilation requires the Creation Kit compiler, the Skyrim scripts, SKSE scripts (including the SKSE `Utility.psc` array helpers), and `TESV_Papyrus_Flags.flg`.

## Credits

- Storm Call Shout Overhaul v1.2 by `dickman290`, used as the gameplay and scripting baseline.
- CommonLibSSE-NG by CharmedBaryon and contributors.
- SKSE and Address Library maintainers.

## License

Repository source is available under the [MIT License](LICENSE). Bethesda game data, SKSE, CommonLibSSE-NG, `fmt`, and `spdlog` remain under their respective licenses.
