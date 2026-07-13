# Architecture

## Effect instances

The six vanilla Storm Call controller effects all attach `ultrastormcallunified`:

- The three non-self effects run in tracker mode with `bTrackInitialTarget=True` and one pass.
- The three self effects run in active-controller mode with `bTrackInitialTarget=False` and two staggered passes.

This preserves the original initial-area application while providing continuous player-centered reacquisition during the storm.

## Target acquisition

A candidate must be loaded, alive, different from the shouter, hostile to the shouter, and inside `12800` world units of the shouter. The current combat target is tried first. Random actors around the shouter are then sampled until a valid candidate is found or the configured attempt limit is reached.

Each pass can strike as many as three distinct actors. Actors already selected by the same pass are excluded. The use of `ObjectReference.GetDistance` makes the test a complete 3D sphere, so flying and ground targets use the same rule.

## Timing

Every script instance schedules its next main update after a random `0.50-1.10` second delay. An active controller runs two passes during that update and waits a random `0.12-0.35` seconds between them. When no valid player target exists, active mode retries after `0.25` seconds.

## Damage

At effect start, every instance reads `GetMagnitude()`. `SCSOStormMagnitudeVar` stores the highest positive value contributed by the currently active storm instances, while `SCSOStormActiveCountVar` controls initialization and cleanup.

The three Storm Call bolt spells and the private visual spell have zero EFIT magnitude. A valid strike remotely casts the visual carrier, then applies `DamageActorValue("Health", sharedMagnitude)` once to the already-filtered hostile actor.

SCSO does not read dragon souls or a Forceful Tongue configuration file. Compatibility is record based: another shout overhaul must expose its final scaling through at least one winning Storm Call controller effect magnitude before the effect starts.

## Projectile bounds plugin

`SCSOProjectileBounds.dll` resolves only `ShockBoltAimStorm` (`000E4CB5`). It hooks projectile, beam-projectile, and missile-projectile 3D load/update functions and expands the loaded scene graph bounds for matching projectile instances. The default bound radius is `18000` world units.

The DLL does not write spell magnitude, projectile speed/range/force, targeting settings, timing, area damage, or dragon-soul values.
