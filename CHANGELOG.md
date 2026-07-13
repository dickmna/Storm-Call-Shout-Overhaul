# Changelog

## 2.1.0

- Published the variable-pass, lower-frequency update as `v2.1.0`.
- Increased the main update interval from `0.50-1.10` seconds to `1.50-3.00` seconds.
- Changed active-controller passes from a fixed two passes to `1 / 2 / 3` passes for Storm Call words one, two, and three.
- Preserved the v1.6.1 `0.12-0.35` second randomized stagger between every pair of consecutive active passes.
- Removed the fixed three-target batch cap. A pass now grows an SKSE Form array as needed and continues until target acquisition is exhausted.
- Set `iTargetsPerUpdate=0` in all six controller VMAD attachments, where zero means unlimited.

## 1.6.1

- Reduced lightning density by changing the main update interval from `0.35-0.85` seconds to `0.50-1.10` seconds.
- Kept the two active-search passes, `0.12-0.35` second internal stagger, three-target batch size, target filters, and 600-foot dynamic sphere unchanged.

## 1.6.0

- Replaced separate ground and elevated aerial searches with one complete 600-foot 3D sphere centered on the shouter.
- Converted 600 magic feet to `12800` Papyrus world units.
- Applied the same distance rule to ground, elevated, and flying actors.
- Removed the elevated search center, minimum-height test, aerial spherical cap, and unlimited-distance airborne combat-target exception.

## 1.5.0

- Made the package standalone with only `Skyrim.esm` and `Update.esm` as masters.
- Added standalone Storm Call spell values: magnitude `30 / 60 / 120`, area `100 / 300 / 600`, and duration `60 / 120 / 180`.
- Removed the Forceful Tongue dependency while retaining normal winning-record magnitude compatibility.

## 1.4.0

- Merged the three custom storm scripts into `ultrastormcallunified`.
- Retained initial-target tracker mode and dynamic active-controller mode in one script class.
- Replaced duplicate B/C active scripts with two internal dynamic passes.
- Added a short randomized stagger between active passes.
- Increased each pass to as many as three distinct hostile targets.
- Added a shared highest-magnitude value for all script instances in the current storm.
- Set Storm Call bolt EFIT magnitudes to zero and applied direct health damage from the shared final effect magnitude, avoiding mixed or double damage.
- Disabled the scripted return-stroke compensation.
- Added the bounds-only `SCSOProjectileBounds` SKSE plugin for the real Storm Call projectile.

## Compared with 1.2

Version 1.2 used three separate custom targeting scripts, `300 / 600 / 900` search radii, `60 / 120 / 180` bolt damage records, and a scripted zero-damage return-stroke visual. Versions 1.4-1.6.1 consolidate control, separate visual carriers from magnitude damage, use a full player-centered dynamic sphere, support multiple targets per pass, and use an SKSE bounds correction instead of the return-stroke visual workaround.
