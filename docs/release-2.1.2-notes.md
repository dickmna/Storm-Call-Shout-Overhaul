# Storm Call Shout Overhaul 2.1.2

Stable ESP/Papyrus release for Skyrim 1.7.104 and SKSE 2.3.1. Install the Address Library database for Skyrim 1.7.104. Older Skyrim runtimes are unsupported, matching the author's September 27 support announcement.

The package keeps StormCallShoutOverhaul.esp and ultrastormcallunified.pex names and record identities. It retains the released 2.1.1 gameplay: unlimited targets per pass and 1/2/3 active-search passes. Existing controller scripts have been recompiled. Indoor searches and strikes pause by default while the storm's lifetime continues. The three voice spells now reference the vanilla Storm Call menu object 000A59AC instead of an unresolved mod-local 04000865. ESP size and group/record topology remain unchanged.

SCSOProjectileBounds.dll was rebuilt against CommonLibSSE-NG 10.0.1, commit de1ca982 (full dependency commit in BUILD-VALIDATION.json). It now advertises Address Library v5 and updated structure use, explicitly accepts Skyrim 1.7.104, requires SKSE 2.3.1 and statically links fmt/spdlog. fmt.dll and spdlog.dll are no longer needed by this plugin. Do not remove another mod's copies of those libraries.

Update through the mod manager as a replacement of the existing SCSO package. Keep the ESP filename. The package is not the pure SKSE beta and should not be combined with it. Preserve a save backup before changing a scripted mod. The ESP gameplay can still be overridden by a later mod's Storm Call records; this release does not claim verified compatibility with every shout overhaul.

Resolved feedback: September 26 SSEEdit MDOB errors; August 26 runtime/DLL loading failure; July 14 indoor target-search complaint. No reply was posted to Nexus and no bug status was changed during build preparation.

Validation: both x64 DLL builds; correct exports/runtime metadata; static fmt/spdlog imports; Papyrus compiler completed with zero errors and warnings; three MDOB references compared with this installation's vanilla Skyrim.esm; six controller VMAD properties and unchanged ESP topology; archive/source/license audit. No game execution has been performed.

Feedback reviewed October 2, 2026:
https://www.nexusmods.com/skyrimspecialedition/mods/179244?tab=posts
https://www.nexusmods.com/skyrimspecialedition/mods/179244?tab=bugs

Matching source is maintained in this GitHub repository. Player downloads contain runtime files and the required license notices.
