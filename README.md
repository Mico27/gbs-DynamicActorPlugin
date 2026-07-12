# GBS Dynamic Actor Plugin

Gives GB Studio actors always-on dynamic behaviors — gravity, velocity, tile collision,
ledge detection, bouncing, and actor-following — that run every frame in the engine
(no per-frame scripting cost). Behaviors are **designed in the IDE** by combining
components in a single event, stored in RAM slots, and assigned to any number of actors.

Works in every action scene type (GBS 4.3.0): **Platformer, Top Down, Adventure,
Shoot 'Em Up, and Point and Click**. (Logo scenes have no actors and are not hooked.)


https://github.com/user-attachments/assets/e32d2233-d1c8-464a-ba14-fb5e5caad736


## Example project

`DynamicActorPluginExample/` is a behavior test suite. **Press START to cycle scenes**:

| Scene | Type | Tests |
|---|---|---|
| Scene 1 | Platformer | Walker, walker avoiding ledges, falling/sliding platform, attach on interact, jump impulse (B) |
| Platform Movers | Platformer | Ground chaser, damped bouncing ball, ground ferry that drags the carried player |
| Top Down | Top Down | Wanderer, chaser, fleer (player-targeted), two bumpers colliding actor-vs-actor with 4-direction facing |
| Adventure Movers | Adventure | Moving ferry carrying the player (walk onto it), chaser, follower attached at a 16px offset |
| Shmup | Shoot 'Em Up | Straight projectile and homing projectile passing through walls, perfect-bounce debris |
| Point n Click | Point and Click | Sparkle attached to the player (velocity mirrored from position each frame), bouncing ball |

## How it works

1. **Define behaviors** in your scene's *On Init* script with the **Define Actor Behavior**
   event. Pick a preset or choose *Custom* and tick the components you want.
   Each definition is stored in a numbered slot (1-8 by default).
2. **Assign a slot to an actor** with **Set Actor Behavior**. Many actors can share one slot.
3. **Give actors velocity** with **Set Actor Velocity** (or the X/Y variants) and let the
   engine handle the rest.

Behavior definitions live in working RAM and are cleared on every scene load —
define them in each scene's init script (put the Define events in a custom script
to share them between scenes).

## Presets

| Preset | Scene types | Components |
|---|---|---|
| Walker | Platformer | Gravity, moves, turns at walls, walk/idle/jump animations |
| Walker (avoid ledges) | Platformer | Same, plus turns around at ledges instead of falling |
| Bouncing ball | Platformer | Gravity, bounces off walls, floor and ceiling with damping |
| Falling object | Platformer | Gravity, vertical movement only |
| Ground chaser | Platformer | Gravity walker that pursues the linked actor horizontally (defaults to the player) |
| Platform rider | Platformer | Gravity + own movement/collision **and** carried by the linked platform while standing on it |
| Wanderer | Top down / Adventure | Bounces around the room, facing its movement in 4 directions (set Bounciness 255) |
| Chaser | Top down / Adventure / flying | Moves toward the linked actor on both axes, 4-direction facing |
| Fleer | Top down / Adventure | Runs away from the linked actor, 4-direction facing |
| Slider | Any | No gravity, moves and turns at walls |
| Reflector | Any | No gravity, bounces off everything (set Bounciness 255) |
| Projectile | Shmup / any | Moves by velocity straight through walls (no tile collision) |
| Homing projectile | Shmup / any | Chases the linked actor straight through walls |
| Attached to linked actor | Any | Tracks the linked actor's movement every frame, regardless of distance; no other components enabled |
| Carried by platform | Any | Tracks the linked actor's movement only while overlapping it (platform riding); no other components enabled — for the engine-controlled player or a simple rider |

Chasers/fleers target **the player by default** (linked actor index 0) — use
*Set Actor Linked Actor* to target something else. Tune pursuit with the
**Chase speed** parameter.

## Custom components

| Component | Effect |
|---|---|
| Gravity | Adds *Gravity* to Y velocity each frame, clamped to *Max fall speed* |
| Move horizontally | Applies X velocity with wall collision |
| Move vertically | Applies Y velocity with floor/ceiling collision |
| Tile collision | On by default. Untick to move through walls/floors (ghosts, flying pickups); turning, bouncing, ledge stop and landing are skipped too — gravity actors fall forever |
| Collide with other actors | Off by default. On contact with another collidable actor (player excluded — the engine already handles it) the frame's movement is blocked and the actor turns/bounces per its *Turn at walls* / *Bounce* settings. Costs one overlap check per on-screen actor each frame |
| Turn at ledges | While grounded, ledges act like walls (smart ledge detection) |
| Turn at walls | Wall hit reverses X velocity (unticked: stops instead) |
| Bounce on floor/ceiling | Floor/ceiling hit reflects Y velocity scaled by *Bounciness* (unticked: stops) |
| Attach to linked actor | Adds the linked actor's velocity to this actor's position each frame (tile-collision checked), **then still runs the other components below** (gravity/move/collision) |
| Only while overlapping (ride as moving platform) | Only shown when *Attach to linked actor* is ticked. Restricts the tracking above to only apply while overlapping the linked actor (platform riding); unticked, the actor tracks the linked actor's movement every frame regardless of distance |
| Chase/flee linked actor | Steer velocity toward (or away from) the linked actor at *Chase speed*. With gravity, only the horizontal axis is steered (ground pursuer); without gravity both axes (top down / flying / homing) |
| Face move direction / Idle when stopped / Jump animation in air | Automatic animation handling |
| Face 4 directions | Face up/down/left/right based on the dominant movement axis — for top down / adventure actors |

Units: positions and velocities are in **subpixels** — 16 subpixels = 1 pixel.
A velocity of 16 moves the actor 1 pixel per frame. *Bounciness* is 0-255,
where 128 keeps half the energy per bounce and 255 is a perfect bounce.

### Following / moving platforms (Attach to linked actor)

*Attach to linked actor* adds the linked actor's per-frame velocity to this actor's
position (tile-collision checked), then **always** still runs the actor's own
components below (gravity/move/collision). The **Only while overlapping** checkbox
(appears once *Attach to linked actor* is ticked) controls *when* that tracking applies:

- **Unticked (default)**: tracks the linked actor's movement every frame, regardless of
  distance — a follower/pet, or an actor that always mirrors another actor's motion.
- **Ticked**: only tracks the linked actor's movement while their bounding boxes overlap
  — moving-platform riding. So an actor — or the player — can step onto a moving
  platform actor, be carried along, and still walk/fall/collide normally on top of it.
  Works in every scene type (top down, adventure, platformer…).

Setup for the "ride as moving platform" mode:
1. Give the **platform** actor a moving behavior (e.g. *Slider* or *Reflector*) so it
   patrols with an X/Y velocity.
2. Give the **rider** a behavior with *Attach to linked actor* **+ Only while
   overlapping** ticked — use the **Platform rider** preset (keeps its own
   gravity/movement) for an NPC/enemy, or the **Carried by platform** preset for the
   **engine-controlled player** (the engine moves the player; the plugin only adds
   the carry).
3. Point the rider at the platform with **Set Actor Linked Actor** (linked actor = the
   platform).

The tracking uses the linked actor's velocity as its per-frame movement, so the linked
actor must move via a dynamic behavior's velocity. With *Only while overlapping* ticked,
the rider is only carried while its bounding box overlaps the platform's, so it is
dropped naturally when it walks off the edge.

The displacement is **tile-collision checked**: this actor normally only checks
collision when it moves by itself, so the linked actor's push is run through the same
wall/floor checks (in the linked actor's movement direction) — a moving platform
cannot shove a stationary rider or the player through walls; the rider is blocked by
the wall and the platform slides on without it. Untick the behavior's *Tile collision*
option to carry through walls instead.

Since the tracking uses the *linked* actor's velocity, following only actually moves
this actor once the linked actor's velocity is non-zero (e.g. set via *Set Actor
Velocity*, or by a dynamic behavior driving it). A linked actor with no velocity of its
own (an engine-controlled, non-dynamic actor such as the player) won't produce any
tracking movement by default.

To attach something to an actor that isn't itself running a dynamic behavior — most
commonly **the player** — mirror its real per-frame movement into a velocity with a
small background loop: read its position each frame (*Actor Get Position*, in pixels),
subtract the previous frame's position to get the delta, feed `delta × 32` into *Set
Actor Velocity* on that actor, then store the current position as the new "previous"
for next frame. The Point n Click example scene's sparkle uses exactly this to stay
attached to the player.

## Spawning actors

Runtime actor spawning (pooled bullets, cannons, random off-screen spawns) lives in the
separate **Spawn Pool Actor Plugin** (`gbs-SpawnPoolActorPlugin`). It has no
dependencies on this plugin, but the two pair naturally: pre-configure each pool
actor's dynamic behavior + velocity in its *On Init* (then *Deactivate*), and spawned
actors resume that behavior. The example project's Platform Movers cannon uses both.

## Events

| Event | Purpose |
|---|---|
| Define Actor Behavior | Create/overwrite a behavior slot (preset or custom components + physics params) |
| Set Actor Behavior | Assign a slot to an actor and set its initial state (grounded / airborne / paused / keep) |
| Set Actor Velocity | Set X and Y velocity together |
| Set Actor X/Y Velocity | Set one axis |
| Set Actor Linked Actor | Set the actor to follow (used by 'Attach to linked actor') |
| Set/Get Actor State | 0 = paused, 1 = grounded, 2 = airborne (auto-managed by gravity behaviors) |
| Get Actor Behavior | Read an actor's current slot into a variable |

All numeric event inputs accept variables and expressions, so behavior parameters and
velocities can be driven by game state at runtime.

Each event above (except Define Actor Behavior) also has a **By Index** variant that
takes a raw actor index (script value) instead of an actor picker, for addressing
actors dynamically (e.g. pool actors spawned via `gbs-SpawnPoolActorPlugin`).

## Motion library events

A library of ready-made movement patterns built on top of the behavior system. Each
event drives an actor's velocity over time (with waits in between), so they are meant
to run in a script that is allowed to wait: an actor's **update script**, a scene
**On Init** thread, or any looping script. Except where noted, one event = **one cycle
of the pattern** — put it inside a *Loop* event to repeat it forever, or chain
different motion events after another to build custom sequential movement (e.g.
*accelerate → zig zag → swoop*). The actor still needs a behavior with the matching
move components assigned (the events only steer velocities; the behavior applies them
with collision, gravity, animation etc.).

All velocities are in subpixels per frame, **32 subpixels = 1 pixel/frame**, engine
range **±127** (≈4 px/frame). The wave events automatically scale their pattern down
to the fastest wave that fits that range.

| Event | Pattern | Behavior needed |
|---|---|---|
| Actor Motion: Sine Wave | Smooth oscillation on one axis (floaters, wavy flyers). Set the other axis' velocity separately for a serpentine course | Move X / Move Y |
| Actor Motion: Circle / Arc | Full circles or partial arcs (orbiters, loop-the-loop, u-turns) | Move X + Move Y (usually tile collision off) |
| Actor Motion: Zig Zag | Straight legs alternating direction on one or both axes | Move X + Move Y |
| Actor Motion: Swoop | Eased dive-then-climb on Y (bat/keese dive; combine with an X velocity to swoop while flying) | Move Y, no gravity |
| Actor Motion: Accelerate / Decelerate | Ramp one axis' velocity to a target at a given acceleration (0 = brake to a stop) | Move X / Move Y |
| Actor Motion: Hop | Jump with horizontal travel (fixed direction or toward/away from an actor), wait for landing, rest | Gravity + Move X + Move Y |
| Actor Motion: Thwomp Slam | Watch for a target overhead-crusher style: slam down to the floor, pause, rise back to the ceiling | Move Y, no gravity |
| Actor Motion: Charge At Target | Wait until row/column-aligned with a target, dash at it, stop on impact (optionally at ledges / other actors) | Move on the dash axis |
| Actor Motion: Random Wander Step | Pick a random 4-way direction, walk, pause (top-down NPC wander with full script control) | Move X + Move Y |
| Set Actor Velocity By Angle | One-shot: set both velocities from an angle (0 = up, 90 = right) and speed | Move X + Move Y |
| Wait For Actor In Range | Block until another actor is inside (or outside) an X/Y pixel range — the generic proximity trigger | any |
| Wait For Actor State | Block until grounded / airborne / paused (e.g. wait for landing) | Gravity behaviors for grounded/airborne |
| Actor Motion: Wall Crawl | Crawl along walls/ceilings/floors and wrap around corners, Zelda-Spark style (right- or left-hand wall follower, runs forever). Backed by the `vm_actor_crawl_step` engine native; fully solid tiles count as wall, map borders included | Move X + Move Y, tile collision off |
| Actor Motion: Sine Wave (Variable) | As Sine Wave, but amplitude/period/duration are script values (variables, expressions), driven at runtime by the engine sine table | Move X / Move Y |
| Actor Motion: Circle (Variable) | As Circle, but radius/period/duration are script values, driven at runtime by the engine sine table | Move X + Move Y |
| Actor Motion: Home At Target | True angular homing: constant flight speed, heading turns toward the target at a limited turn rate (256ths of a turn per update) for smooth curved pursuit. Speed/turn rate/duration are script values | Move X + Move Y (tile collision off for missiles) |

Notes:

- **Sine / Circle / Swoop / Zig Zag** bake a quantized velocity table into the script at
  compile time, so their shape fields (amplitude, period, radius…) are fixed numbers,
  not variables. The tables are drift-corrected: a full cycle displaces the actor by
  exactly zero, so they can loop forever without wandering off. Script size grows with
  `period / update interval` (capped at 64 velocity steps per cycle).
- **Sine / Circle / Zig Zag** have a *Cycles* field (0 = repeat forever inside the event)
  and a *Stop at end* checkbox — untick it to chain seamlessly into the next motion
  event without a velocity hiccup.
- The runtime events (**Ramp, Hop, Thwomp, Charge, Wander, the two Waits**) poll each
  frame with `idle`, so they cost one native call per frame per waiting actor at most.
- The waits never time out on their own; pair them with sensible behaviors (e.g. *Wait
  For Actor State: grounded* needs a gravity behavior or it waits forever).
- **Wall Crawl** moves in 8px cell steps; its speed is snapped to 1/2/4/8/16/32/64 so
  turn decisions always happen exactly on cell boundaries. Place the crawler on an
  8px-aligned tile next to a wall (tile coordinates in the editor are always aligned).
  With no wall in reach it walks in a small circle until it finds one.
- The **Variable** wave events and **Home At Target** evaluate their script-value
  fields when the event starts, cost one `VM_SIN_SCALE`/`VM_COS_SCALE` per update at
  runtime, and support a *Duration* (0 = forever). Amplitude/radius accepts 1-160 px;
  the wave/orbit speed is capped at ±127 subpx/frame at runtime.
- **Home At Target** with *Aim at target on start* off launches at the fixed angle and
  curves in from there — launch away from the player for a boomerang-style pass.

Recipe examples (update script of the enemy, everything inside a Loop event):

- **Patrolling guard with pauses**: Set X Velocity → Wait For Actor Collision
  (horizontal + pit) → Set X Velocity 0 → Wait 30 → repeat with opposite sign
  (or just use walker behavior + *Turn at walls* for the no-pause version).
- **Thwomp**: behavior = Move Y only; loop *Thwomp Slam*.
- **Keese / crow swoop**: behavior = Move X + Move Y, tile collision off; loop
  { Wait For Actor In Range → Accelerate X toward player → Swoop → Accelerate X to 0 }.
- **Hopping slime**: behavior = walker (gravity); loop { Hop toward player }.
- **Spiked-beetle charger**: behavior = Move X (+ ledge stop); loop
  { Charge At Target (horizontal) → Wait 60 }.
- **Podoboo / lava jump**: behavior = gravity + Move Y, tile collision off; loop
  { Set Y Velocity -160 → Wait For Actor State: grounded... } or gravity + collision
  and { Set Y Velocity → Hop in place }.
- **Sinusoidal shmup enemy**: behavior = projectile preset; Set X Velocity -24, then
  Sine Wave on Y, cycles 0.
- **Spark / wall hugger (Zelda dungeons)**: behavior = Move X + Move Y, tile collision
  off; update script = just the Wall Crawl event.
- **Homing missile**: behavior = projectile preset (tile collision off); spawn via
  `gbs-SpawnPoolActorPlugin`, update script = Home At Target with duration ~180 and low
  turn rate, then Deactivate.
- **Difficulty-scaled flyer**: store amplitude/period in variables, use Sine Wave
  (Variable) — one event serves every difficulty setting.

## Engine settings (Settings → Engine fields)

Group **Dynamic actor**:

| Setting | Default | Notes |
|---|---|---|
| Tile collision type | Origin point (Fast) | Triangle and Bounding box are more accurate but slower |
| Enable slope collision | Off | Slope tile support (needs slope collision tiles) |
| Max behavior slots | 8 | Slider (1-32). Each slot costs 6 bytes of RAM |

### Modular components (group **Dynamic actor components**)

Every physics/animation part of the engine is an independently compiled component.
**All are enabled by default.** Uncheck the ones your game never uses to strip that
code out of the ROM entirely (`#ifdef`-guarded at compile time). For example, a game
that only needs falling objects can keep *Gravity* + *Move vertically* and disable the
rest.

| Component setting | Removes when unchecked |
|---|---|
| Component: Gravity | Gravity acceleration |
| Component: Move horizontally | Horizontal movement + wall collision routines |
| Component: Move vertically | Vertical movement + floor/ceiling collision routines |
| Component: Turn at ledges | Ledge/pit detection routine |
| Component: Turn at walls | Wall-bounce reversal (actors stop at walls instead) |
| Component: Bounce on floor/ceiling | Bounce physics |
| Component: Attach to linked actor | Linked-actor following |
| Component: Ride linked actor (moving platform) | Moving-platform carry |
| Component: Chase/flee linked actor | Chase and flee steering |
| Component: Collide with other actors | Actor-vs-actor collision blocking |
| Component: Animation handling | Automatic face/idle/jump animation |

Notes:
- Disabling a component only removes its **code**. Behavior flags that reference a
  disabled component are simply ignored at runtime, so nothing crashes — the actor just
  won't perform that part.
- Gravity is only visible together with *Move vertically*.
- *Turn at ledges* requires *Move horizontally*; *Bounce* requires *Move vertically*.

## Performance notes

- One flag-driven update pass over the **active** actor list only (off-screen actors are
  already excluded by the engine's activation system).
- Actors with no behavior or in the paused state cost a couple of comparisons per frame.
- Collision cost scales with the collision type setting: origin point does 1-2 collision
  tile reads per moving actor per frame.
- RAM: 6 bytes × (slots + 1) for the behavior table (54 bytes at default), plus 7 bytes
  per actor for velocity/behavior/state/link.

