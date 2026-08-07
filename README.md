# gbs-DynamicActorPlugin

**Version 4.3.0 — Requires GB Studio ≥ 4.3.0**

Gives GB Studio actors always-on dynamic behaviors — gravity, velocity, tile collision,
ledge detection, bouncing, and parent-actor carrying (moving platforms, followers) —
that run every frame in the engine (no per-frame scripting cost). Behaviors are
**designed in the IDE** by combining components in a single event, stored in RAM slots,
and assigned to any number of actors.

Works in every action scene type: **Platformer, Top Down, Adventure,
Shoot 'Em Up, and Point and Click**. (Logo scenes have no actors and are not hooked.)


https://github.com/user-attachments/assets/cb91889a-e531-4a79-b456-66c876b27699


https://github.com/user-attachments/assets/e32d2233-d1c8-464a-ba14-fb5e5caad736

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [Engine Settings](#engine-settings)
4. [Size Limits and Restrictions](#size-limits-and-restrictions)
5. [Events Reference](#events-reference)
6. [Memory Footprint](#memory-footprint)

---

## Concepts

### Behavior slots

Behavior definitions live in working RAM and are cleared on every scene load —
define them in each scene's init script (put the Define events in a custom script
to share them between scenes).

Units: positions and velocities are in **subpixels** — 32 subpixels = 1 pixel.
A velocity of 32 moves the actor 1 pixel per frame. *Bounciness* is 0-255,
where 128 keeps reflects the full velocity and 255 reflects x2 the velocity.

### Presets

| Preset | Scene types | Components |
|---|---|---|
| Walker | Platformer | Gravity, moves, turns at walls, walk/idle/jump animations |
| Walker (turns at ledges) | Platformer | Same, plus turns around at ledges instead of falling |
| Bouncing ball | Platformer | Gravity, bounces off walls, floor and ceiling with damping |
| Falling object | Platformer | Gravity, vertical movement only |
| Moving platform | Any | Moves by velocity and carries every actor (or the player) that touches it — auto-parents on contact, releases on separation |
| Wanderer | Top down / Adventure | Bounces around the room, facing its movement in 4 directions (set Bounciness 255) |
| Slider | Any | No gravity, moves and turns at walls |
| Reflector | Any | No gravity, bounces off everything (set Bounciness 255) |
| Projectile | Shmup / any | Moves by velocity straight through walls (no tile collision) |

Chasing, fleeing and homing are **not presets** — chase/flee is the waitable **Actor Chase
Actor** event that steers a behavior's velocity toward or away from a target. Followers and
attachments are done with **parent actors** (see below).

### Custom components

Choosing the **Custom** preset exposes the component checkboxes below. There is no
per-behavior "move on X/Y" toggle — an actor always moves by its velocity on both axes
(subject to the engine's *Move horizontally / Move vertically* compile components).
Use **Lock position axes** to freeze behavior-driven movement on an axis for a slot.

| Component | Effect |
|---|---|
| Gravity Y | Adds *Gravity* to Y velocity each frame, clamped to *Max fall speed* |
| Gravity Z | Adds *Gravity* to Z velocity instead (topdown jump/height — needs *Topdown Z axis*) |
| Tile collision | On by default. Untick to move through walls/floors (ghosts, flying pickups); turning, bouncing, ledge stop and landing are skipped too — gravity actors fall forever |
| Collide with other actors | Off by default. On contact with another collidable actor (player excluded — the engine already handles it) the frame's movement is blocked and the actor turns/bounces per its *Turn at walls* / *Bounce* settings. Costs one overlap check per on-screen actor each frame |
| Moving platform | Claims every actor this actor touches as a child (they inherit its movement) and releases them when they stop touching. Skips actors that already have a different parent and — if this actor has a collision group set — actors in a different group. Combine movement so the platform itself moves |
| Turn at ledges | While grounded, ledges act like walls (smart ledge detection) |
| Turn at walls | Wall hit reverses X velocity (unticked: stops instead) |
| Bounce on floor/ceiling | Floor/ceiling hit reflects Y velocity scaled by *Bounciness* (unticked: stops) |
| Bounce on z ground | Z hit against ground level reflects Z velocity scaled by *Bounciness* (topdown) |
| Jump animation in air (y / z axis) | Play the jump animation while airborne on the Y axis (platformer) or Z axis (topdown) |

These options are shown for **every** behavior (preset or custom):

| Option | Effect |
|---|---|
| Lock position axes | Freeze behavior-driven movement on any of X / Y / Z (or combinations) — e.g. a rail that only slides horizontally |
| Lock direction axes | Stop the automatic animation from changing the horizontal and/or vertical facing |
| Tile collision type | Collision model for this slot: *Origin point (fastest)*, *Triangle*, or *Bounding box* |
| Gravity | Acceleration in subpixels/frame added while airborne (default 8) |
| Max fall speed | Terminal velocity in subpixels/frame (default 64) |
| Bounciness | Energy kept per bounce, 0-255 (used with the Bounce components) |
| Trigger … event flags | Enable the actor-event callbacks — see [Actor event callbacks](#actor-event-callbacks) |
| Trigger other actors' On Hit / Activate triggers | Let the actor hurt actors or fire triggers — see [Triggering other actors and triggers](#triggering-other-actors-and-triggers) |

Different slots can use different collision models — give the fast *Origin point* model to
swarms and the accurate *Bounding box* to the few actors that need it. Only the models
enabled in engine settings are compiled into the ROM (see below).

### Parent actors (followers and moving platforms)

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

#### Parenting mode (engine setting)

The **Parenting mode** engine setting picks *how* a parented actor follows its parent,
globally for the whole game. Pick the cheapest one that does what you need:

| Mode | What it does | Cost |
|---|---|---|
| **Static parenting (Fast)** | The child is pinned at a fixed pixel offset from the parent every frame and runs **no other behavior code**. The offset is the child's own X/Y velocity read as a pixel offset (set it with *Set Actor X/Y Velocity*). Use for HUD pieces, a rigidly attached sparkle, a health bar over a boss | Cheapest — no physics, no tile collision |
| **Inherit first parent velocity (Slower)** | The child is carried by its **direct parent's velocity** (tile-collision checked), then still runs its own behavior physics. Only tracks a parent that has a dynamic velocity — with one exception: **the player is followed by its position delta** (the plugin keeps a player-position snapshot), since the engine-controlled player has no velocity field. Any *other* non-dynamic parent won't be followed | One velocity read per child (a player parent adds a per-frame player snapshot) |
| **Apply all parents positions delta (Slowest)** | The child is displaced by the **summed position change of its whole parent chain** since last frame (tile-collision checked), then still runs its own behavior physics. Follows **any** parent — including engine-moved ones like the player *and* dynamic moving platforms — because the carry is applied at the end of the frame (order-independent) | Walks the parent chain + an end-of-frame carry and position-snapshot pass per actor |

The default is **Apply all parents positions delta**, the most general (it is what the
example project and the notes below assume).

Details (velocity/delta modes):

- **The player works on both sides.** The engine-controlled player can ride a moving
  platform, and can also *be* a parent: a parented actor follows the player with no
  extra scripting (the Point n Click sparkle does exactly this). This works in **both**
  *delta* mode (via the player's per-frame position delta) and *velocity* mode (which
  special-cases a player parent to a position delta, since the player has no velocity
  field). In *delta* mode any parent works this way; in *velocity* mode only the player
  gets the position-delta treatment.
- In *velocity* mode a non-dynamic parent *other than the player* moves its children
  through its **velocity**, so such a scripted parent must move via a dynamic behavior
  (or have its velocity set) for children to follow. *Delta* mode has no such
  requirement — it tracks position change for every parent.
- The inherited displacement is **tile-collision checked** in the parent's movement
  direction: a moving platform cannot shove a rider or the player through walls —
  the rider is blocked and the platform slides on without it. Untick the child
  behavior's *Tile collision* option to carry through walls instead.
- A *Moving platform* never steals children: actors that already have a different
  parent are skipped, and if the platform has a collision group set it only claims
  actors in the same group. **The player is the exception — a platform always picks
  the player up regardless of its collision group.**
- Enable **Platforms attach the player only** (engine setting) to restrict platform
  auto-attach to the player — every other actor is ignored by platforms (but can still
  be parented explicitly with *Set Actor Parent Actor*). Handy for a platformer where
  only the player should ride platforms, and it skips the claim test on all other
  actors.
- Claiming/releasing happens once per frame after all actors have moved, using the
  platform's final position, and a newly claimed rider starts inheriting movement the
  next frame. Only paused platforms and explicitly set parents never auto-release.
  At most *Max moving platforms* (engine setting, default 2) platforms can claim
  riders in a scene.

### Topdown Z axis

Enable the **Topdown Z axis** engine setting to give actors a **Z position and Z velocity**
(height above the ground) in top-down / adventure scenes. With it on:

- Gravity can be applied to **Z instead of Y** (use the *Gravity Z* component), so an
  actor can hop straight up and fall back to the floor without moving on the map.
- *Bounce on z ground* and *Jump animation in air on z axis* handle the landing bounce
  and the airborne pose.
- Read and write height with **Get/Set actor z position** and **Get/Set actor z velocity**.

This is how you build a top-down jump (hopping slime, thrown pot, Zelda-style leap)
without a platformer scene. The Z fields cost 3 bytes of RAM per actor and are only
compiled when the setting is on.

### Actor event callbacks

A behavior can run a **script of your own** when certain engine events happen to the
actor — a state change, a tile collision on any side, or entering a new tile. Wire it up
in two places:

1. In **Define Actor Behavior**, tick the matching *Trigger … event* flag for the slot
   (*Trigger state change event*, *Trigger tile collision (top / right / bottom / left)
   event*, *Trigger tile enter event*).
2. **Attach a Script to a Dynamic Actor Event** to register the script that runs when that
   event fires, and **Remove a Script from a Dynamic Actor Event** to clear it. Slots:
   *State change*, *Tile collision (Top / Right / Bottom / Left / Any)*, *Tile enter*,
   *Actor activated*, *Actor deactivated*.

Before the callback runs, the plugin fills in engine fields describing what happened, so
the script can branch on them: **Event actor index** (`dynamic_actor_event_actor_idx`),
**Event behavior index**, **Event state**, **Event tile collision** value, and **Event
tile x / y**. Use these for footstep sounds on tile-enter, damage/landing reactions on a
specific-side collision, or state-driven animation swaps — all without polling per frame.

Each group of slots has its own engine setting, and unticking one compiles that group
out entirely — the dispatch disappears from the physics loop and the slots stop costing
RAM:

| Setting | Slots it compiles |
|---|---|
| *Events: Actor state changed* | State change |
| *Events: Tile collision / tile enter* | Tile collision (Top / Right / Bottom / Left / Any), Tile enter |
| *Events: Actor activated / deactivated* | Actor activated, Actor deactivated |

Attaching a script to a slot whose setting is off is refused at compile time with a
message naming the setting, rather than failing later or silently doing nothing.

### Actor activated / deactivated callbacks

*Actor activated* and *Actor deactivated* are two extra slots of **Attach a Script to a
Dynamic Actor Event**. Unlike the other callbacks they need no behavior flag and fire for
**every** actor in the scene, whether or not it runs a dynamic behavior: the engine calls
them from `activate_actor_impl()` / `deactivate_actor_impl()`, which covers the automatic
deactivation that happens when an actor scrolls offscreen as well as manual Activate /
Deactivate Actor events.

**Event actor index** tells the script which actor changed. Points worth knowing:

* The registrations are cleared by `dynamic_actor_init()` on every scene load, so attach
  them from the scene's **On Init**.
* The **player is never reported**. The engine activates it once per scene load - before
  the callback table has been cleared - and never deactivates it afterwards, so skipping
  it both closes that window and costs nothing.
* Like every other dynamic actor callback, a slot runs one script at a time: if an actor
  activates while the previous activation script is still running, that firing is skipped.
* Turn the whole thing off with the *Events: Actor activated / deactivated* engine
  setting; the plugin's `actor.c` then compiles as if the hooks were not there.

### Triggering other actors and triggers

By default only the **player** can set off an actor's On Hit script or a scene trigger.
Two per-behavior options in **Define Actor Behavior** let a dynamic actor do it too.

In both cases the triggering actor's index is written to the **Event actor index**
engine field (`dynamic_actor_event_actor_idx`) right before the script runs, so the
fired On Hit / trigger script can read it to find out which actor set it off. The field
is reset to 0 at the start of every frame.

- **Trigger other actors' On Hit on collision** — when an actor with this behavior
  overlaps another collidable actor, it runs that actor's **On Hit** script, passing
  **this actor's collision group** so the On Hit script can branch on who hit it — the
  same way the engine passes an attacker's group to the player's hit script. The actor
  must have a collision group set to deal hits (a group-less actor deals none). Only the
  **first** overlapping actor is hit per frame, and that target won't be re-triggered
  until its previous On Hit script finishes. The player is skipped (the engine already
  handles player contact). Good for enemy-on-enemy damage, thrown objects, or projectiles
  that share the actor pool. Requires the *Trigger other actors' On Hit* engine component.
- **Activate triggers on enter/leave** — an actor with this behavior fires a trigger's
  **On Enter** / **On Leave** scripts as it moves in and out of the trigger, just like
  the player does. Each actor tracks its own current trigger (one byte of RAM per actor),
  independent of the player. The player is skipped here (the engine drives its own
  trigger activation). Requires the *Actors activate triggers* engine component.

---

## Project Setup

1. **Define behaviors** in your scene's *On Init* script with the **Define Actor Behavior**
   event. Pick a preset or choose *Custom* and tick the components you want.
   Each definition is stored in a numbered slot (1-8 by default, up to 32).
2. **Assign a slot to an actor** with **Set Actor Behavior**. Many actors can share one slot.
3. **Give actors velocity** with **Set Actor Velocity** (or the X/Y variants) and let the
   engine handle the rest.

Compatibility variants are included so the plugin can coexist with other engine-file plugins. The matching variant is selected automatically when any combination of **ContinuousScenePlugin**, **ScreenScrollPlugin**, **MetaTilePlugin** and **SceneStackExPlugin** is also installed — no manual step needed.

---

## Engine Settings

Group **Dynamic actor**:

| Setting | Default | Notes |
|---|---|---|
| Collision model: Origin point | On | Compile the single-point (fastest) collision model |
| Collision model: Triangle | On | Compile the triangle collision model |
| Collision model: Bounding box | On | Compile the bounding-box collision model |
| Enable slope collision | Off | Slope tile support (needs slope collision tiles) |
| Max actors | 21 | Slider (1-21). Size of the engine's `actors[]` array, player included. See [Max actors](#max-actors) |
| Max behavior slots | 8 | Slider (1-15). Each slot costs 8 bytes of RAM. Capped at 15 — see [Behavior id and state share a byte](#behavior-id-and-state-share-a-byte) |
| Max moving platforms | 2 | Slider (1-8). How many *Moving platform* actors can claim/release riders per scene; each costs 11 bytes of RAM. Platforms past the limit still move but never pick up riders |
| Topdown Z axis | Off | Enable actor Z position/velocity fields and apply gravity to Z instead of Y (topdown jump/height). See [Topdown Z axis](#topdown-z-axis) |
| Platforms attach the player only | Off | When on, a *Moving platform* only auto-attaches the player on contact; other actors are ignored (they can still be parented explicitly). Also skips the per-frame claim test on every non-player actor. Only shown when *Parent actors* is enabled |
| Parenting mode | Apply all parents positions delta | How a parented actor follows its parent (see [Parenting mode](#parenting-mode-engine-setting)): *Static parenting* (rigid offset, no physics, fastest), *Inherit first parent velocity* (carried by direct parent's velocity, runs own physics), or *Apply all parents positions delta* (summed chain delta, follows any parent incl. the player, runs own physics). Only shown when *Parent actors* is enabled |

The collision model each behavior uses is picked **per behavior slot** in the Define
Actor Behavior event; the three checkboxes above only control which models are
compiled into the ROM. Uncheck the models none of your behaviors use to save ROM — see
[Build error: bank size overflow](#build-error-bank-size-overflow) if the build runs
out of bank space.

### Max actors

GB Studio sizes its `actors[]` array once, at compile time, for the whole game — every
slot costs a full `actor_t` of WRAM whether a scene uses it or not. Stock is 21: the
player plus the 20 actors the editor allows in a single scene. With this plugin's
per-actor fields an `actor_t` runs 60-71 bytes depending on which components are on, so
the array alone is 1.2-1.5 KB.

**Max actors** lowers that ceiling. A game whose busiest scene holds 8 actors can set it
to 9 and hand back roughly 850 bytes of WRAM. A scene that does hold more actors than the
limit still loads — the engine keeps the first ones and drops the rest — so pick the
number from your most crowded scene, not your average one.

Raising it above 21 has no effect worth having: the editor caps a scene at 20 actors, so
the extra slots stay empty and only cost RAM.

### Behavior id and state share a byte

Each actor's behavior id and behavior state live in the same byte of `actor_t`, four
bits each. That is why **Max behavior slots** stops at 15, and why an actor state has to
be **0-15**.

The four states the plugin manages itself are paused (0), grounded (1), airborne Y (2)
and airborne Z (3); **4 through 15 are free** for your own state machines. Only *Set
Actor State* can reach them, and a value above 15 wraps around — *Wait For Actor State*
wraps the value it compares the same way, so the two stay consistent.

### Modular components

Every physics/animation part of the engine is an independently compiled component.
**All are enabled by default** (except *Topdown Z axis* and *Slope collision*). Uncheck
the ones your game never uses to strip that code out of the ROM entirely. For example, a
game that only needs falling objects can keep *Gravity* + *Move vertically* and disable
the rest.

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
| Component: Trigger other actors' On Hit | Firing a collided actor's On Hit script on contact |
| Component: Actors activate triggers | Actor-driven trigger On Enter/On Leave (also frees 1 byte/actor of RAM) |
| Component: Animation handling | Automatic face/idle/jump animation |

The scripting natives are toggleable the same way — uncheck the ones no script in
your game uses:

| VM setting | Removes when unchecked |
|---|---|
| VM: Wait for collision | The Wait For Actor Collision events |
| VM: Wait for actor in range | The Wait For Actor In Range events |
| VM: Wait for actor state | The Wait For Actor State events |
| VM motion: Chase actor | The Actor Chase Actor events |
| VM motion: Move to position by velocity | The Actor Move To Position By Velocity events |
| VM motion: Crawl step | The Actor Motion: Wall Crawl events. Also requires *Move horizontally* + *Move vertically* |
| Tools: Get tile collision | The Get Tile Collision event |
| Tools: Get actor at point | The Get Actor Collision event |
| Tools: Iterate actors in area | The Actor Iterate In Area event |
| Tools: Get / Set extended actor properties | The Actor Get / Set Property (Extended) events |
| Tools: Force trigger actor script | The Actor Trigger Script event |

Notes:
- Disabling a component only removes its **code**. Behavior flags that reference a
  disabled component are simply ignored at runtime, so nothing crashes — the actor just
  won't perform that part. Disabling a **VM setting** while an event that needs it is
  still used in a script *will* fail at link time — remove the events first.
- Gravity is only visible together with *Move vertically*.
- *Turn at ledges* requires *Move horizontally*; *Bounce* requires *Move vertically*.

---

## Size Limits and Restrictions

### Build error: bank size overflow

⚠️ If you enable a lot of components at once, the build can fail with:

```
BankPack: ERROR! Area _CODE_, bank 255, size 17752 is too large for bank size 16384
(file ...\obj\dynamic_actor.o)
?ASlink-Error-<cannot open> : "...lcc61000.lk"
```

This is **not** a bug — it means the compiled code of the file named in the error
(here `dynamic_actor.o`) no longer fits in a single 16 KB ROM bank. This is usually
caused by another plugin replacing the engine’s tile-lookup helper with a larger one,
which pushes this plugin’s code over the bank limit. A single object
file cannot be split across banks, so the fix is to compile less code into it by
**unchecking settings you don't use** in Settings → Engine fields → *Dynamic actor*.
The number in the error tells you how much you need to shave off — in the example
above, `17752 - 16384 = 1368` bytes.

Check the **filename** in the error first, because different settings feed different
object files:

**If the error names `dynamic_actor.o`**, uncheck any of these (roughly largest saving
first — pick the ones your game genuinely never uses):

| Setting | Why it's a good candidate |
|---|---|
| Enable slope collision | Off by default. Adds slope handling to *every* compiled collision model, so it is usually the single biggest win |
| Collision model: Triangle / Bounding box / Origin point | Each model is a separate set of routines. Most games only use one — uncheck the other two |
| Topdown Z axis | Off by default. Adds a Z axis to movement, gravity and bounce throughout |
| Component: Parent actors / moving platforms | Large: parenting chain + platform claim/release |
| Component: Turn at ledges | Ledge/pit detection routines |
| Component: Move horizontally / vertically | Only if your game truly moves on one axis |
| Component: Bounce on floor/ceiling | Bounce physics, including the 32-bit damping math |
| Events: Tile collision / tile enter | Removes the callback dispatch from every collision site in the physics loop |
| VM: Wait for collision | Its detection loop is part of the same object file |
| VM motion: Crawl step | Its wall-crawl routine is part of the same object file |

For the exact byte figure of every setting, see
[What each engine setting costs](#what-each-engine-setting-costs).

Remember that unchecking a **VM setting** while a script still uses the matching event
fails at link time — delete those events first. Unchecking a **component** is always
safe: behavior flags referencing it are just ignored at runtime.

### Performance notes

- One flag-driven update pass over the **active** actor list only (off-screen actors are
  already excluded by the engine's activation system).
- Actors with no behavior and no parent cost two byte-compares and a pointer test per
  frame — everything else (behavior lookup, tile math, flags) is only loaded after that
  early-out.
- Collision cost scales with the behavior's collision type: origin point does 1-2
  collision tile reads per moving actor per frame.
- Moving platforms don't scan the actor list themselves: each platform caches its box
  once per frame, and a single end-of-frame pass claims/releases riders against those
  cached boxes (parented actors are only tested against their own parent's box).
- Scenes that never define a *Moving platform* behavior and never set a parent skip the
  entire end-of-frame parenting pass (claiming + position snapshots) — the parent
  component then costs one flag test per frame.
- Actors cache their own list index, so the runtime never has to compute it. That
  matters most for *Actors activate triggers*, which needs the index for every
  flagged actor on every frame.

---

## Events Reference

| Event | Purpose |
|---|---|
| Define Actor Behavior | Create/overwrite a behavior slot (preset or custom components + physics params + tile collision type + trigger flags) |
| Set Actor Behavior | Assign a slot to an actor and set its initial state (grounded / airborne / paused / keep) |
| Get Actor Behavior | Read an actor's current slot into a variable |
| Set Actor Velocity | Set X and Y velocity together |
| Set Actor X / Y Velocity | Set one axis |
| Get Actor X / Y Velocity | Read one axis into a variable |
| Set Actor Velocity By Angle | Set both velocities from an angle (0 = up, 90 = right) and speed |
| Set / Get Actor State | 0 = paused, 1 = grounded, 2 = airborne (auto-managed by gravity behaviors) |
| Set / Get Actor Z Position, Set / Get Actor Z Velocity | Read/write actor height (needs *Topdown Z axis*) |
| Set Actor Parent Actor | Parent this actor to another (it inherits the parent's movement each frame) |
| Clear Actor Parent Actor | Detach the actor from its parent |
| Get Tile Collision | Read the collision tile value at a tile coordinate into a variable |
| Get Actor Collision | Find the first collidable actor at a pixel position (index, or -1 for none) |
| Attach a Script to a Dynamic Actor Event | Register a script to run when a behavior's state-change / tile-collision / tile-enter event fires |
| Remove a Script from a Dynamic Actor Event | Clear an attached script slot |
| Actor Iterate In Area | Run a script once per actor inside a rectangular area (tiles or pixels, anchored to the scene or to the visible screen, origin or bounding box, optionally skipping inactive actors) |
| Actor Get Property (Extended) | Read an actor field no stock event exposes: collision group and its extra flags, raw flags, active / hidden / pinned / persistent / disabled, animation state, tick and frame range, base and reserved tiles, bounds, script handles |
| Actor Set Property (Extended) | Write the same fields. The active flag is preserved - use the stock Activate / Deactivate Actor events for it |
| Actor Trigger Script | Force an actor's On Interact, On Hit (per collision group) or On Update script to run |

All numeric event inputs accept variables and expressions, so behavior parameters and
velocities can be driven by game state at runtime.

Many events also have a **By Index** variant that takes a raw actor index (script value)
instead of an actor picker, for addressing actors dynamically (e.g. pool actors spawned
via the [SpawnPoolActorPlugin](https://github.com/Mico27/gbs-SpawnPoolActorPlugin)): *Set/Get Behavior*, *Set Velocity*, *Set/Get X/Y Velocity*,
*Set/Get State*, *Set Parent Actor*, *Wait For Actor Collision*, *Actor Get/Set Property
(Extended)*, and *Actor Trigger Script*.

### Motion library events

A library of ready-made movement patterns built on top of the behavior system. Each
event drives an actor's velocity over time (with waits in between), so they are meant
to run in a script that is allowed to wait: an actor's **update script**, a scene
**On Init** thread, or any looping script. Except where noted, one event = **one cycle
of the pattern** — put it inside a *Loop* event to repeat it forever, or chain different
motion events after another to build custom sequential movement. The actor still needs a
behavior assigned; these events only steer velocities, and the behavior applies them with
collision, gravity and animation. "Move X / Y" in the table means the actor is free to
move on that axis (the default — the engine move component is compiled and the axis isn't
locked by the behavior).

All velocities are in subpixels per frame, **32 subpixels = 1 pixel/frame**, engine
range **±127** (≈4 px/frame). The wave events automatically scale their pattern down
to the fastest wave that fits that range.

| Event | Pattern | Behavior needed |
|---|---|---|
| Actor Chase Actor | Waitable chase **or flee**: steer toward/away from a target actor at the actor's own movement speed. With gravity only the horizontal axis is steered (ground pursuer), otherwise both axes (top down / flying). *Stop range* completes the event near/far from the target (0 = run forever); *Target refresh interval* trades accuracy for CPU | Move on the steered axes |
| Actor Move To Position By Velocity | Waitable move to a destination driven by behavior velocity — like *Actor Move To*, but with physics. Axis order or diagonal, optional *Direct to point* angle steering for smooth non-45° diagonals, *Relative* targets with unit snapping, *Cancel on collision* to give up when blocked | Move X / Move Y |
| Actor Motion: Sine Wave | Smooth oscillation on one axis (floaters, wavy flyers). Set the other axis' velocity separately for a serpentine course | Move X / Move Y |
| Actor Motion: Circle / Arc | Full circles or partial arcs (orbiters, loop-the-loop, u-turns) | Move X + Move Y (usually tile collision off) |
| Actor Motion: Bezier | Follow a quadratic (3-point) or cubic (4-point) Bezier curve baked into the script at compile time — control points in pixels relative to the start. *Cycles* and *Stop at end* like the wave events | Move X + Move Y |
| Actor Motion: Swoop | Eased dive-then-climb on Y (bat/keese dive; combine with an X velocity to swoop while flying) | Move Y, no gravity |
| Actor Motion: Charge At Target | Wait until row/column-aligned with a target, dash at it, stop on impact (optionally at ledges / other actors) | Move on the dash axis |
| Actor Motion: Wall Crawl | Crawl along walls/ceilings/floors and wrap around corners, Zelda-Spark style (right- or left-hand wall follower, runs forever). Fully solid tiles count as wall, map borders included | Move X + Move Y, tile collision off |

### Wait events

| Event | Blocks until |
|---|---|
| Wait For Actor In Range | Another actor is inside (or outside) an X/Y pixel range — the generic proximity trigger |
| Wait For Actor State | The actor is grounded / airborne / paused (e.g. wait for landing — needs a gravity behavior) |
| Wait For Actor Collision | The actor hits a wall, floor/ceiling, pit or another actor (needs matching move components) |

Notes:

- **Sine / Circle / Swoop / Bezier** bake a quantized velocity table into the script at
  compile time, so their shape fields (amplitude, period, radius, control points…) are
  fixed numbers, not variables. The tables are drift-corrected: a full cycle displaces
  the actor by exactly zero, so they can loop forever without wandering off. Script size
  grows with `period / update interval` (capped at 64 velocity steps per cycle).
- **Sine / Circle / Bezier** have a *Cycles* field (0 = repeat forever inside the event)
  and a *Stop at end* checkbox — untick it to chain seamlessly into the next motion event
  without a velocity hiccup.
- The runtime events (**Chase, Move To Position, Charge**, the **Waits**) poll each frame,
  so they cost one native call per frame per waiting actor at most.
- **Wall Crawl** moves in 8px cell steps; its speed is snapped to 1/2/4/8/16/32/64 so
  turn decisions always happen exactly on cell boundaries. Place the crawler on an
  8px-aligned tile next to a wall (tile coordinates in the editor are always aligned).
  With no wall in reach it walks in a small circle until it finds one.
- **Chase** and **Move To Position By Velocity** move at the actor's **movement speed**
  (the standard actor speed setting) — change it with the stock *Actor Set Movement
  Speed* event.

Recipe examples (update script of the enemy, everything inside a Loop event):

- **Patrolling guard with pauses**: Set X Velocity → Wait For Actor Collision
  (horizontal + pit) → Set X Velocity 0 → Wait 30 → repeat with opposite sign
  (or just use walker behavior + *Turn at walls* for the no-pause version).
- **Ground chaser / fleer**: behavior = walker (gravity) or slider (top down);
  update script = *Actor Chase Actor* with stop range 0.
- **Keese / crow swoop**: behavior = Move X + Move Y, tile collision off; loop
  { Wait For Actor In Range → Set X Velocity toward player → Swoop → Set X Velocity 0 }.
- **Spiked-beetle charger**: behavior = Move X (+ ledge stop); loop
  { Charge At Target (horizontal) → Wait 60 }.
- **Sinusoidal shmup enemy**: behavior = projectile preset; Set X Velocity -24, then
  Sine Wave on Y, cycles 0.
- **Spark / wall hugger (Zelda dungeons)**: behavior = Move X + Move Y, tile collision
  off; update script = just the Wall Crawl event.
- **Top-down hop**: enable *Topdown Z axis*; behavior = Gravity Z + Bounce on z ground;
  loop { Set Z Velocity (up) → Wait For Actor State: grounded → Wait }.

---

<!-- SETTINGCOST:BEGIN -->
### What each engine setting costs

Every setting here changes what gets compiled. Figures are what you **get back by
turning the setting off**; rows marked *off by default* show what turning it **on**
costs instead, and sliders show the cost per step. A dash means that budget does not
move.

| Setting | Bank 0 | WRAM | Banked ROM |
|---|---|---|---|
| Collision model: Origin point | — | — | **1,574 B** |
| Collision model: Triangle | — | — | **1,779 B** |
| Collision model: Bounding box | — | — | **1,976 B** |
| Enable slope collision *(off by default — cost of turning it on)* | — | +1 B | +1,629 B |
| Max actors *(slider 1–21, default 21)* | — | 67 B/step | 31.65 B/step |
| Max behavior slots *(slider 1–15, default 8)* | — | 8 B/step | — |
| Max moving platforms *(slider 1–8, default 2)* | — | 11 B/step | — |
| Component: Gravity | — | — | **308 B** |
| Topdown Z axis *(off by default — cost of turning it on)* | +91 B | +107 B | +1,113 B |
| Component: Move horizontally | — | — | **5,735 B** |
| Component: Move vertically | — | — | **4,329 B** |
| Component: Turn at ledges | — | — | **1,947 B** |
| Component: Turn at walls | — | — | **54 B** |
| Component: Bounce on floor/ceiling | — | — | **208 B** |
| Component: Parent actors / moving platforms | — | **150 B** | **2,119 B** |
| Parenting mode → *Static parenting (Fast)* | — | −84 B | −406 B |
| Parenting mode → *Inherit first parent velocity (Slower)* | — | −80 B | −152 B |
| Platforms attach the player only *(off by default — cost of turning it on)* | — | — | +25 B |
| Component: Collide with other actors | — | — | **1,111 B** |
| Component: Trigger other actors' On Hit | — | — | **706 B** |
| Component: Actors activate triggers | — | **21 B** | **505 B** |
| Component: Animation handling | — | — | **419 B** |
| Events: Actor state changed | — | — | **131 B** |
| Events: Tile collision / tile enter | — | — | **1,171 B** |
| Events: Actor activated / deactivated | — | **10 B** | **193 B** |
| Tools: Iterate actors in area | — | — | **884 B** |
| Tools: Get / Set extended actor properties | — | — | **1,432 B** |
| Tools: Force trigger actor script | — | — | **395 B** |
| Tools: Get tile collision | — | — | **176 B** |
| Tools: Get actor at point | — | — | **376 B** |
| VM: Wait for collision | — | — | **1,129 B** |
| VM: Wait for actor in range | — | — | **431 B** |
| VM: Wait for actor state | — | — | **134 B** |
| VM motion: Chase actor | — | — | **855 B** |
| VM motion: Move to position by velocity | — | — | **2,309 B** |
| VM motion: Crawl step | — | — | **2,268 B** |

Turning off every on-by-default switch above frees **181 B** of WRAM, **34,654 B** of banked ROM — the full
span between this plugin at its fullest and stripped to nothing. Treat it as a
ceiling rather than a recipe: you keep whatever your game actually uses.

- **Max actors**: going from 1 to 21 moves WRAM by +1,340 B, banked ROM by +633 B.
- **Max behavior slots**: going from 1 to 15 moves WRAM by +112 B.
- **Max moving platforms**: going from 1 to 8 moves WRAM by +77 B.

- **Parenting mode** only applies when *Component: Parent actors / moving platforms* is enabled.
- **Platforms attach the player only** only applies when *Component: Parent actors / moving platforms* is enabled.

<details><summary>How these were measured</summary>

GB Studio 4.3.0-e1. This plugin's `engine/src/**/*.c` was compiled with the
toolchain and flags GB Studio itself uses (`lcc -msm83:gb -Wf--max-allocs-per-node 3000
-DHUGE_TRACKER -DRUMBLE_ENABLE=0x08u`) against a merged include tree, and the SDCC object
files' area records were read: `_HOME` is bank 0, `_DATA`/`_INITIALIZED`/`_BSS` are WRAM,
and `_CODE*`/`_CONST`/`_LIT`/`_INITIALIZER` are banked ROM.

Two caveats. Only this plugin's own engine sources are measured, so a setting that also
changes a struct shared with stock engine files can move a few more bytes in files the
plugin does not ship. And each setting is toggled on its own: a handful measure slightly
*negative* because enabling their code lets the compiler drop a fallback path elsewhere,
and settings that gate other settings only show their own contribution.

</details>
<!-- SETTINGCOST:END -->

## Memory Footprint

All of the plugin's state lives in WRAM. How much it consumes depends on which
components are enabled and on the slider settings. Per-actor costs are multiplied by
`MAX_ACTORS` — the **Max actors** setting, **21** by default (active *and* inactive
slots). The figures below assume that default; lowering it scales every per-actor row
down proportionally, and shrinks the engine's own `actors[]` array on top of that
(60-71 B per slot).

| What | Size | Present when |
|---|---|---|
| Behavior table | 8 B × (behavior slots + 1) → **72 B** (8 slots) … **128 B** (15 slots) | always |
| Core globals (event fields, callback table, scratch) | **44 B** | always |
| Per actor: packed behavior id + state, X/Y velocity | 3 B × 21 = **63 B** | always |
| Per actor: cached actor index | 1 B × 21 = **21 B** | always |
| Per actor: parent pointer | 2 B × 21 = **42 B** | *Parent actors* |
| Platform cache | 11 B × *Max moving platforms* → **22 B** (2) … **88 B** (8) | *Parent actors* |
| Parent flag + counter | **2 B** | *Parent actors* |
| Per actor: position snapshot | 4 B × 21 = **84 B** (+2 B × 21 = **42 B** with Z axis) | *Parent actors* **and** parenting mode = *Apply all parents positions delta* |
| Player position snapshot | 4 B (+2 B with Z axis) | *Parent actors* **and** parenting mode = *Inherit first parent velocity* |
| Per actor: Z position + Z velocity | 3 B × 21 = **63 B** | *Topdown Z axis* |
| Z scratch | **2 B** | *Topdown Z axis* |
| Slope scratch | **1 B** | *Slope collision* (with triangle/bounding-box model) |
| Per actor: last-trigger index | 1 B × 21 = **21 B** | *Actors activate triggers* |

**Totals:**

- **Default configuration** (Parent actors on, *delta* parenting mode, Z axis off, slope
  off, triggers on, 8 behavior slots, 2 platforms, 21 actors): **≈ 371 B**.
- **Everything enabled** (all components on, *delta* mode, Z axis on, slope on, 15
  behavior slots, 8 platforms, 21 actors): **≈ 601 B** — the plugin's maximum WRAM
  footprint.
- **Leanest** (all optional components off, 8 slots, 21 actors): **≈ 200 B**.

For reference, a stock GB Studio 4.3.0 project has roughly **850 B** of WRAM free, so even
the maximum configuration fits, though it leaves little headroom — trim behavior slots,
platforms, and unused components to reclaim space. Choosing the *static* or *velocity*
parenting mode instead of *delta* also drops the per-actor position snapshot (84 B, or
126 B with the Z axis). By far the biggest single lever, though, is **Max actors**: each
slot you cut off the top frees a whole `actor_t` (60-71 B) plus the per-actor rows above.

**ROM:** all of the plugin's code lives in banked ROM apart from the bank 0 figure below,
and scales with which components are enabled — see [Engine Settings](#engine-settings).

**SRAM:** not used. Save slots and cartridge requirements are unaffected.

---

<!-- BANK0:BEGIN -->
## Bank 0 (HOME) Usage

Bank 0 is the 16 KB non-switchable ROM bank that the GB Studio engine core,
the interrupt handlers and the GBDK runtime all share. Banked ROM is cheap
(add another bank), bank 0 is not, so it is usually the first thing a project
runs out of.

| | Bytes |
|---|---|
| Bank 0 used by this plugin | **+122** |
| Bank 0 free with this plugin installed | **1,329** of 16,384 (92% used) |

Everything else this plugin adds lives in banked ROM.

| Module | This plugin | Stock engine | Bank 0 cost |
|---|---|---|---|
| `actor.c` | 993 | 871 | +122 |

Modules that replace or patch a stock engine file only cost the *difference*:
the stock version's bank 0 bytes were being spent anyway.

<details><summary>How this was measured</summary>

GB Studio 4.3.2, DMG target, default engine settings. Each module's bank 0
contribution is the `A _HOME size` record that SDCC writes into its `.rel`
object, summed over the engine sources this plugin provides. Stock sizes come
from building projects whose only plugin ships no engine C, so every module in
them is the untouched engine; two such builds were compared and agreed on all
73 shared modules.

The "free" figure is a stock project with this plugin and nothing else. Your
own number will differ: other plugins, and any engine settings that change what
the core compiles, move it independently of this plugin.

</details>
<!-- BANK0:END -->
