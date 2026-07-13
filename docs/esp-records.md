# ESP Record Contract

`StormCallShoutOverhaul.esp` is standalone and has only `Skyrim.esm` and `Update.esm` as masters.

## Shout spells

| Form ID | Editor ID | Initial effect | Self effect |
| --- | --- | --- | --- |
| `00018609` | `SCSO_VoiceStormCall01` | magnitude 5, area 100, duration 60 | magnitude 30, duration 60 |
| `0001860A` | `SCSO_VoiceStormCall02` | magnitude 5, area 300, duration 120 | magnitude 60, duration 120 |
| `0001860D` | `SCSO_VoiceStormCall03` | magnitude 5, area 600, duration 180 | magnitude 120, duration 180 |

## Controller effects

The following vanilla records carry `ultrastormcallunified` VMAD data:

- Tracker effects: `000A1A58`, `000A1A5C`, `000A1A5B`.
- Active/self effects: `000E3F0A`, `000E3F09`, `000D5E81`.

Every instance has `iTargetsPerUpdate=3`, `fDynamicSearchRadius=12800`, and `bUseReturnStrokeVisual=False`. Tracker instances use one pass; active instances use two passes.

## Visual carrier spells

The winning bolt records `000E4CB7`, `000E98A2`, and `000E98A3` use SCSO editor IDs and have EFIT magnitude `0`. Private spell `SCSO_ReturnStrokeVisualSpell` is also zero damage. These spells carry the real Storm Call projectile visuals while Papyrus applies shared-magnitude damage.

## Private globals

- `SCSOStormMagnitudeVar`: highest captured magnitude for the active storm.
- `SCSOStormActiveCountVar`: active instance count and shared-state lifetime.

## Projectile

The ESP carries a winning override for vanilla `ShockBoltAimStorm` (`PROJ 000E4CB5`). Runtime loaded-3D bounds are handled separately by `SCSOProjectileBounds.dll`.
