# Storm Call Shout Overhaul 3.0.1 beta

Pure SKSE extension for Skyrim 1.7.104 and SKSE 2.3.1. Install the Address Library release containing the matching 1.7.104 database. The DLL was built with CommonLibSSE-NG 10.0.1, commit de1ca982 (the complete dependency commit is recorded in BUILD-VALIDATION.json).

This beta leaves the winning Storm Call spell and scripts running. Their damage, duration, targets, range, cooldown, weather and primary attack cadence stay active. The extension repairs scenegraph visibility bounds for the original ShockBoltAimStorm beam. Each recognized natural strike remains one strike at word 1; words 2 and 3 append one and two copies of the same actual spell, with accumulated 0.12–0.35 second offsets. Copies use the original shooter, cause, target, projectile base, casting source, power and scale. The game applies spell damage and resistances. The DLL does not directly subtract health or rewrite spell/projectile/weather records.

Extra strikes require a live Storm Call effect on the responsible actor whose spell matches a current winning shout variation. They stop when the effect ends, the target unloads or dies, or caster/target is indoors. Unrecognized custom implementations receive only the visibility fix. No additional target scan or custom storm lifetime is introduced. Generated copies are marked to prevent recursive multiplication; save loading and new games discard pending copies.

Install this optional beta on a new character or a separately backed-up test save. It is an architectural migration, not an in-place replacement for an existing ESP/Papyrus installation. Do not combine it with StormCallShoutOverhaul.esp, SCSOProjectileBounds.dll, the old SCSOStormCall.dll, or the old ultrastormcallunified.pex. Users retaining an existing SCSO save should use stable 2.1.2, which keeps the plugin/script identities.

Validation: release x64 build; SKSE exports and runtime metadata; static fmt/spdlog dependencies; executable tests of the exact production delay queue, pause behavior, duplicate suppression and load reset; archive structure/source/license audit. Game execution and specific Forceful Tongue, Stormcrown or Thunderchild combinations have not been tested. The beta is deliberately optional; compiled and offline-validated does not mean game-tested.

Source is included under Docs/SCSO/Source. CommonLibSSE-NG licensing is included under Docs/SCSO/CommonLibSSE-NG. Keep these folders with redistributions.
