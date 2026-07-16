# GBS Dynamic Actor Plugin

Gives GB Studio actors always-on dynamic behaviors — gravity, velocity, tile collision,
ledge detection, bouncing, and parent-actor carrying (moving platforms, followers) —
that run every frame in the engine (no per-frame scripting cost). Behaviors are
**designed in the IDE** by combining components in a single event, stored in RAM slots,
and assigned to any number of actors.

Works in every action scene type (GBS 4.3.0): **Platformer, Top Down, Adventure,
Shoot 'Em Up, and Point and Click**. (Logo scenes have no actors and are not hooked.)


https://github.com/user-attachments/assets/e32d2233-d1c8-464a-ba14-fb5e5caad736


## Example project

`DynamicActorPluginExample/` is a behavior test suite. **Press START to cycle scenes**:

| Scene | Type | Tests |
|---|---|---|
| Scene 1 | Platformer | Walker, walker avoiding ledges, falling/sliding platform, attach on interact (parenting), jump impulse (B) |
| Platform Movers | Platformer | Ground chaser (*Actor Chase Actor*), damped bouncing ball, ground ferry that drags the carried player, pooled cannon (`gbs-SpawnPoolActorPlugin`) |
| Top Down | Top Down | Wanderer, two bumpers colliding actor-vs-actor with 4-direction facing |
| Adventure Movers | Adventure | Moving ferry carrying the player (walk onto it), chasers/fleers via *Actor Chase Actor*, parented followers |
| Shmup | Shoot 'Em Up | Straight projectile and homing projectile passing through walls, perfect-bounce debris |
| Point n Click | Point and Click | Sparkle parented to the player, bouncing ball |
| Test Crawler | — | Wall Crawl motion testbed |
| Test Circle | — | Bezier (Variable) motion testbed |

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
| Moving platform | Any | Moves by velocity and carries every actor (or the player) that touches it — auto-parents on contact, releases on separation |
| Wanderer | Top down / Adventure | Bounces around the room, facing its movement in 4 directions (set Bounciness 255) |
| Slider | Any | No gravity, moves and turns at walls |
| Reflector | Any | No gravity, bounces off everything (set Bounciness 255) |
| Projectile | Shmup / any | Moves by velocity straight through walls (no tile collision) |

Chasing, fleeing and homing are **not presets** — they are waitable events that steer a
behavior's velocity: use **Actor Chase Actor** (chase/flee at the actor's movement
speed) or **Actor Motion: Home At Target** (smooth angular homing). Followers and
attachments are done with **parent actors** (see below).

## Custom components

| Component | Effect |
|---|---|
| Gravity | Adds *Gravity* to Y velocity each frame, clamped to *Max fall speed* |
| Move horizontally | Applies X velocity with wall collision |
| Move vertically | Applies Y velocity with floor/ceiling collision |
| Tile collision | On by default. Untick to move through walls/floors (ghosts, flying pickups); turning, bouncing, ledge stop and landing are skipped too — gravity actors fall forever |
| Collide with other actors | Off by default. On contact with another collidable actor (player excluded — the engine already handles it) the frame's movement is blocked and the actor turns/bounces per its *Turn at walls* / *Bounce* settings. Costs one overlap check per on-screen actor each frame |
| Moving platform | Claims every actor this actor touches as a child (they inherit its movement) and releases them when they stop touching. Skips actors that already have a different parent and — if this actor has a collision group set — actors in a different group. Combine with Move horizontally/vertically so the platform itself moves |
| Turn at ledges | While grounded, ledges act like walls (smart ledge detection) |
| Turn at walls | Wall hit reverses X velocity (unticked: stops instead) |
| Bounce on floor/ceiling | Floor/ceiling hit reflects Y velocity scaled by *Bounciness* (unticked: stops) |
| Face move direction / Idle when stopped / Jump animation in air | Automatic animation handling |
| Face 4 directions | Face up/down/left/right based on the dominant movement axis — for top down / adventure actors |

Each behavior also picks its own **Tile collision type**: *Origin point (fastest)*,
*Triangle* or *Bounding box*. Different slots can use different models — give the
fast model to swarms and the accurate one to the few actors that need it. (Only the
models enabled in engine settings are compiled into the ROM — see below.)

Units: positions and velocities are in **subpixels** — 16 subpixels = 1 pixel.
A velocity of 16 moves the actor 1 pixel per frame. *Bounciness* is 0-255,
where 128 keeps half the energy per bounce and 255 is a perfect bounce.

## Parent actors (followers / moving platforms)

Parenting is not a behavior component on the child: **any actor with a parent set
inherits the parent actor's per-frame movement** (tile-collision checked), then still
runs its own behavior physics (gravity/move/collision) on top. Two ways to set it up:

- **Explicitly**: point the child at its parent with **Set Actor Parent Actor**
  (detach with **Clear Actor Parent Actor**). A follower, a sparkle pinned to a
  character, a rider that should stay attached regardless of distance.
- **Automatically**: give the platform actor the **Moving platform** component (or
  preset). It claims every actor it touches as a child and releases them when they
  leave, so actors — or the player — can step onto it, be carried along, and still
  walk/fall/collide normally on top of it. Works in every scene type.

Details:

- **The player works on both sides.** The engine-controlled player can ride a moving
  platform, and can also *be* a parent: the plugin uses the player's real per-frame
  movement, so a parented actor follows the player with no extra scripting (the
  Point n Click sparkle does exactly this).
- Other non-dynamic parents move their children through their **velocity**, so a
  scripted parent must move via a dynamic behavior (or have its velocity set) for
  children to follow.
- The inherited displacement is **tile-collision checked** in the parent's movement
  direction: a moving platform cannot shove a rider or the player through walls —
  the rider is blocked and the platform slides on without it. Untick the child
  behavior's *Tile collision* option to carry through walls instead.
- A *Moving platform* never steals children: actors that already have a different
  parent are skipped, and if the platform has a collision group set it only claims
  actors in the same group.

## Spawning actors

Runtime actor spawning (pooled bullets, cannons, random off-screen spawns) lives in the
separate **Spawn Pool Actor Plugin** (`gbs-SpawnPoolActorPlugin`). It has no
dependencies on this plugin, but the two pair naturally: pre-configure each pool
actor's dynamic behavior + velocity in its *On Init* (then *Deactivate*), and spawned
actors resume that behavior. The example project's Platform Movers cannon uses both.

## Events

| Event | Purpose |
|---|---|
| Define Actor Behavior | Create/overwrite a behavior slot (preset or custom components + physics params + tile collision type) |
| Set Actor Behavior | Assign a slot to an actor and set its initial state (grounded / airborne / paused / keep) |
| Set Actor Velocity | Set X and Y velocity together |
| Set Actor X/Y Velocity | Set one axis |
| Set Actor Parent Actor | Parent this actor to another (it inherits the parent's movement each frame) |
| Clear Actor Parent Actor | Detach the actor from its parent |
| Set/Get Actor State | 0 = paused, 1 = grounded, 2 = airborne (auto-managed by gravity behaviors) |
| Get Actor Behavior | Read an actor's current slot into a variable |
| Get Tile Collision | Read the collision tile value at a tile coordinate into a variable |
| Get Actor Collision | Find the first collidable actor at a pixel position (index, or -1 for none) |

All numeric event inputs accept variables and expressions, so behavior parameters and
velocities can be driven by game state at runtime.

The Set/Get Behavior, Velocity, State, Parent and Wait For Actor Collision events also
have a **By Index** variant that takes a raw actor index (script value) instead of an
actor picker, for addressing actors dynamically (e.g. pool actors spawned via
`gbs-SpawnPoolActorPlugin`).

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
| Actor Chase Actor | Waitable chase **or flee**: steer toward/away from a target actor at the actor's own movement speed. With gravity only the horizontal axis is steered (ground pursuer), otherwise both axes (top down / flying). *Stop range* completes the event near/far from the target (0 = run forever); *Target refresh interval* trades accuracy for CPU | Move on the steered axes |
| Actor Move To Position By Velocity | Waitable move to a destination driven by behavior velocity — like *Actor Move To*, but with physics. Axis order or diagonal, optional *Direct to point* angle steering for smooth non-45° diagonals, *Relative* targets with unit snapping, *Cancel on collision* to give up when blocked | Move X / Move Y |
| Actor Motion: Sine Wave | Smooth oscillation on one axis (floaters, wavy flyers). Set the other axis' velocity separately for a serpentine course | Move X / Move Y |
| Actor Motion: Circle / Arc | Full circles or partial arcs (orbiters, loop-the-loop, u-turns) | Move X + Move Y (usually tile collision off) |
| Actor Motion: Zig Zag | Straight legs alternating direction on one or both axes | Move X + Move Y |
| Actor Motion: Bezier | Follow a quadratic (3-point) or cubic (4-point) Bezier curve baked into the script at compile time — control points in pixels relative to the start. *Cycles* and *Stop at end* like the wave events | Move X + Move Y |
| Actor Motion: Bezier (Variable) | As Bezier, but runtime-driven: control points/step are script values, and the actor traces the curve at its **own movement speed**, waiting for the actor to catch up to each curve sample (so tile collision can slow or block it) | Move X + Move Y |
| Actor Motion: Swoop | Eased dive-then-climb on Y (bat/keese dive; combine with an X velocity to swoop while flying) | Move Y, no gravity |
| Actor Motion: Accelerate / Decelerate | Ramp one axis' velocity to a target at a given acceleration (0 = brake to a stop) | Move X / Move Y |
| Actor Motion: Hop | Jump with horizontal travel (fixed direction or toward/away from an actor), wait for landing, rest | Gravity + Move X + Move Y |
| Actor Motion: Thwomp Slam | Watch for a target overhead-crusher style: slam down to the floor, pause, rise back to the ceiling | Move Y, no gravity |
| Actor Motion: Charge At Target | Wait until row/column-aligned with a target, dash at it, stop on impact (optionally at ledges / other actors) | Move on the dash axis |
| Actor Motion: Random Wander Step | Pick a random 4-way direction, walk, pause (top-down NPC wander with full script control) | Move X + Move Y |
| Set Actor Velocity By Angle | One-shot: set both velocities from an angle (0 = up, 90 = right) and speed | Move X + Move Y |
| Wait For Actor In Range | Block until another actor is inside (or outside) an X/Y pixel range — the generic proximity trigger | any |
| Wait For Actor State | Block until grounded / airborne / paused (e.g. wait for landing) | Gravity behaviors for grounded/airborne |
| Wait For Actor Collision | Block until the actor hits a wall, floor/ceiling, pit or another actor | matching move components |
| Actor Motion: Wall Crawl | Crawl along walls/ceilings/floors and wrap around corners, Zelda-Spark style (right- or left-hand wall follower, runs forever). Backed by the `vm_actor_crawl_step` engine native; fully solid tiles count as wall, map borders included | Move X + Move Y, tile collision off |
| Actor Motion: Sine Wave (Variable) | As Sine Wave, but amplitude/period/duration are script values (variables, expressions), driven at runtime by the engine sine table | Move X / Move Y |
| Actor Motion: Circle (Variable) | As Circle, but radius/duration are script values and the actor orbits at its **own movement speed**. Runs entirely in the `vm_actor_motion_circle_variable` engine native: each update it re-derives the orbital angle from the actor's actual position around the circle center (atan2) and steers toward a point ahead on the circle — the orbit self-corrects, so walls or pushes bend the path instead of displacing the whole circle | Move X + Move Y |
| Actor Motion: Home At Target | True angular homing: constant flight speed, heading turns toward the target at a limited turn rate (256ths of a turn per update) for smooth curved pursuit. Speed/turn rate/duration are script values | Move X + Move Y (tile collision off for missiles) |

Notes:

- **Sine / Circle / Swoop / Zig Zag / Bezier** bake a quantized velocity table into the
  script at compile time, so their shape fields (amplitude, period, radius, control
  points…) are fixed numbers, not variables. The tables are drift-corrected: a full
  cycle displaces the actor by exactly zero, so they can loop forever without wandering
  off. Script size grows with `period / update interval` (capped at 64 velocity steps
  per cycle).
- **Sine / Circle / Zig Zag / Bezier** have a *Cycles* field (0 = repeat forever inside
  the event) and a *Stop at end* checkbox — untick it to chain seamlessly into the next
  motion event without a velocity hiccup.
- The runtime events (**Chase, Move To Position, Ramp, Hop, Thwomp, Charge, Wander,
  the Waits**) poll each frame with `idle`, so they cost one native call per frame per
  waiting actor at most.
- The waits never time out on their own; pair them with sensible behaviors (e.g. *Wait
  For Actor State: grounded* needs a gravity behavior or it waits forever).
- **Wall Crawl** moves in 8px cell steps; its speed is snapped to 1/2/4/8/16/32/64 so
  turn decisions always happen exactly on cell boundaries. Place the crawler on an
  8px-aligned tile next to a wall (tile coordinates in the editor are always aligned).
  With no wall in reach it walks in a small circle until it finds one.
- **Chase**, **Move To Position By Velocity**, **Circle (Variable)** and **Bezier
  (Variable)** move at the actor's **movement speed** (the standard actor speed
  setting) — change it with the stock *Actor Set Movement Speed* event.
- The **Variable** wave events and **Home At Target** evaluate their script-value
  fields when the event starts and support a *Duration* (0 = forever).
  Amplitude/radius accepts 1-160 px; the Sine Wave (Variable) speed is capped at
  ±127 subpx/frame at runtime.
- **Home At Target** with *Aim at target on start* off launches at the fixed angle and
  curves in from there — launch away from the player for a boomerang-style pass.

Recipe examples (update script of the enemy, everything inside a Loop event):

- **Patrolling guard with pauses**: Set X Velocity → Wait For Actor Collision
  (horizontal + pit) → Set X Velocity 0 → Wait 30 → repeat with opposite sign
  (or just use walker behavior + *Turn at walls* for the no-pause version).
- **Ground chaser / fleer**: behavior = walker (gravity) or slider (top down);
  update script = *Actor Chase Actor* with stop range 0.
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
- **Swooping curved dive**: Bezier (Variable) with P1/P2 shaping the arc — the control
  points are script values, so the same event can dive at the player's current position.
- **Difficulty-scaled flyer**: store amplitude/period in variables, use Sine Wave
  (Variable) — one event serves every difficulty setting.

## Engine settings (Settings → Engine fields)

Group **Dynamic actor**:

| Setting | Default | Notes |
|---|---|---|
| Collision model: Origin point | On | Compile the single-point (fastest) collision model |
| Collision model: Triangle | On | Compile the triangle collision model |
| Collision model: Bounding box | On | Compile the bounding-box collision model |
| Enable slope collision | Off | Slope tile support (needs slope collision tiles) |
| Max behavior slots | 8 | Slider (1-32). Each slot costs 6 bytes of RAM |

The collision model each behavior uses is picked **per behavior slot** in the Define
Actor Behavior event; the three checkboxes above only control which models are
compiled into the ROM. Uncheck the models none of your behaviors use to save ROM.

### Modular components

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
| Component: Parent actors / moving platforms | Parent-actor inheritance **and** the Moving platform component |
| Component: Collide with other actors | Actor-vs-actor collision blocking |
| Component: Animation handling | Automatic face/idle/jump animation |

The scripting natives are toggleable the same way — uncheck the ones no script in
your game uses:

| VM setting | Removes when unchecked |
|---|---|
| VM: Wait for collision | `vm_wait_for_collision` (Wait For Actor Collision events) |
| VM motion: Chase actor | `vm_actor_chase_actor` (Actor Chase Actor events) |
| VM motion: Move to position by velocity | `vm_actor_move_to_pos_by_velocity` (Actor Move To Position By Velocity) |
| VM motion: Circle variable | `vm_actor_motion_circle_variable` (Actor Motion: Circle (Variable)) |
| VM motion: Bezier to | `vm_actor_move_bezier_to` (Actor Motion: Bezier (Variable)) |
| VM motion: Crawl step | `vm_actor_crawl_step` (Actor Motion: Wall Crawl) |

Notes:
- Disabling a component only removes its **code**. Behavior flags that reference a
  disabled component are simply ignored at runtime, so nothing crashes — the actor just
  won't perform that part. Disabling a **VM setting** while an event that needs it is
  still used in a script *will* fail at link time — remove the events first.
- Gravity is only visible together with *Move vertically*.
- *Turn at ledges* requires *Move horizontally*; *Bounce* requires *Move vertically*.

## Performance notes

- One flag-driven update pass over the **active** actor list only (off-screen actors are
  already excluded by the engine's activation system).
- Actors with no behavior or in the paused state cost a couple of comparisons per frame.
- Collision cost scales with the behavior's collision type: origin point does 1-2
  collision tile reads per moving actor per frame.
- RAM: 6 bytes × (slots + 1) for the behavior table (54 bytes at default), plus 6 bytes
  per actor for velocity/behavior/state/parent.
