# GBS Dynamic Actor Plugin

Gives GB Studio actors always-on dynamic behaviors — gravity, velocity, tile collision,
ledge detection, bouncing, and actor-following — that run every frame in the engine
(no per-frame scripting cost). Behaviors are **designed in the IDE** by combining
components in a single event, stored in RAM slots, and assigned to any number of actors.

Works in every action scene type (GBS 4.3.0): **Platformer, Top Down, Adventure,
Shoot 'Em Up, and Point and Click**. (Logo scenes have no actors and are not hooked.)

### Update ordering per scene type

The behavior update runs once per frame in each scene type's update function:

- **Platformer, Adventure, Point and Click**: after the player is processed (end of
  update). Linked actors track the player with no lag.
- **Top Down, Shoot 'Em Up**: at the start of the update, before the player is processed.
  These scene types have early `return` paths (e.g. Top Down landing on a trigger), so
  the hook is placed first to guarantee dynamic actors always update every frame. Linked
  actors in these scenes trail the player by one frame — usually desirable (trailing
  options/shields).

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

| Preset | Components |
|---|---|
| Walker | Gravity, moves, turns at walls, walk/idle/jump animations |
| Walker (avoid ledges) | Same, plus turns around at ledges instead of falling |
| Bouncing ball | Gravity, bounces off walls, floor and ceiling with damping |
| Falling object | Gravity, vertical movement only |
| Slider | No gravity, moves and turns at walls (fish, saw blades…) |
| Reflector | No gravity, perfect bounces off everything (pong ball, projectiles) |
| Attached to linked actor | Follows another actor at a fixed offset |

## Custom components

| Component | Effect |
|---|---|
| Gravity | Adds *Gravity* to Y velocity each frame, clamped to *Max fall speed* |
| Move horizontally | Applies X velocity with wall collision |
| Move vertically | Applies Y velocity with floor/ceiling collision |
| Turn at ledges | While grounded, ledges act like walls (smart ledge detection) |
| Turn at walls | Wall hit reverses X velocity (unticked: stops instead) |
| Bounce on floor/ceiling | Floor/ceiling hit reflects Y velocity scaled by *Bounciness* (unticked: stops) |
| Attach to linked actor | Position = linked actor + offset. Overrides everything else |
| Face move direction / Idle when stopped / Jump animation in air | Automatic animation handling |

Units: positions and velocities are in **subpixels** — 16 subpixels = 1 pixel.
A velocity of 16 moves the actor 1 pixel per frame. *Bounciness* is 0-255,
where 128 keeps half the energy per bounce and 255 is a perfect bounce.

## Events

| Event | Purpose |
|---|---|
| Define Actor Behavior | Create/overwrite a behavior slot (preset or custom components + physics params) |
| Set Actor Behavior | Assign a slot to an actor and set its initial state (grounded / airborne / paused / keep) |
| Set Actor Velocity | Set X and Y velocity together |
| Set Actor X/Y Velocity | Set one axis |
| Set Actor Linked Actor | Set the actor to follow and the X/Y offset |
| Set/Get Actor State | 0 = paused, 1 = grounded, 2 = airborne (auto-managed by gravity behaviors) |
| Get Actor Behavior | Read an actor's current slot into a variable |

All numeric event inputs accept variables and expressions, so behavior parameters and
velocities can be driven by game state at runtime.

## Engine settings (Settings → Engine fields)

Group **Dynamic actor**:

| Setting | Default | Notes |
|---|---|---|
| Tile collision type | Origin point (Fast) | Triangle and Bounding box are more accurate but slower |
| Enable slope collision | Off | Slope tile support (needs slope collision tiles) |
| Max behavior slots | 8 | Slider (1-32). Each slot costs 5 bytes of RAM |

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
| Component: Bounce on floor/ceiling | Bounce physics incl. 32-bit multiply (actors stop instead) |
| Component: Attach to linked actor | Linked-actor following |
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
- RAM: 5 bytes × (slots + 1) for the behavior table (45 bytes at default), plus 7 bytes
  per actor for velocity/behavior/state/link.

## Migrating from the previous version

Old hard-coded behavior IDs map to presets — add Define Actor Behavior events to your
scene init scripts using the same slot numbers:

| Old ID | Preset |
|---|---|
| 1 | Walker |
| 2 | Walker (avoid ledges) |
| 3 | Bouncing ball (Gravity 2, Bounciness ~128) |
| 4 | Falling object |
| 5 | Slider |
| 6 | Reflector |
| 7 | Attached to linked actor |

The old *Set Actor Linked Actor* + X/Y velocity-as-offset trick still works; the event
now also has explicit offset fields.
