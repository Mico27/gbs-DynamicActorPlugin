#ifndef DYNAMIC_ACTOR_H
#define DYNAMIC_ACTOR_H

#include <gbdk/platform.h>
#include "actor.h"
#include "data/states_defines.h"

// Number of user-definable behavior slots (slot 0 is reserved = no behavior)
#ifndef DYNAMIC_ACTOR_MAX_BEHAVIORS
#define DYNAMIC_ACTOR_MAX_BEHAVIORS 8
#endif

// Physics component flags (flags)
#define BHV_GRAVITY_Y   0x01u  // apply gravity to y velocity, clamped to max_fall_vel
#define BHV_GRAVITY_Z   0x02u  // apply gravity to z velocity, clamped to max_fall_vel
#define BHV_LEDGE_STOP  0x04u  // while grounded, treat ledges/pits as walls
#define BHV_REFLECT_X   0x08u  // reverse x velocity on wall hit (otherwise stop)
#define BHV_REFLECT_Y   0x10u  // bounce y velocity on floor/ceiling hit (otherwise stop)
#define BHV_REFLECT_Z   0x20u  // bounce z velocity when z position hits the ground
#define BHV_PLATFORM    0x40u  // moving platform: sets itself as the parent of every
                               // actor it intersects (unless that actor already has a
                               // different parent, or the platform has a collision
                               // group and the actor's group differs), and clears
                               // itself as parent when the actor stops intersecting.
                               // Parented actors inherit the platform's movement.
#define BHV2_NO_TILE_COLLISION 0x80u  // move by velocity without tile collision
                                      // (passes through walls/floors; no wall turn,
                                      // bounce, ledge stop or landing)

// Lock / animation / secondary behavior flags (flags2)
#define BHV3_LOCK_POS_X        0x01u  // freeze x position updates
#define BHV3_LOCK_POS_Y        0x02u  // freeze y position updates
#define BHV3_LOCK_POS_Z        0x04u  // freeze z position updates
#define BHV3_LOCK_DIR_H        0x08u  // prevent behavior from changing left/right dir
#define BHV3_LOCK_DIR_V        0x10u  // prevent behavior from changing up/down dir
#define BHV2_ANIM_IDLE         0x20u  // idle animation when x velocity is zero
#define BHV2_ANIM_JUMP         0x40u  // jump animation while airborne
#define BHV2_ACTOR_COLLISION   0x80u  // collide with other actors (player excluded -
                                      // the engine already handles that): on overlap
                                      // the frame's movement is reverted and velocity
                                      // turns/bounces per the reflect settings

// Actor behavior states (actor_state)
#define BHV_STATE_PAUSED    0
#define BHV_STATE_GROUNDED  1
#define BHV_STATE_AIBORNE_Y 2
#define BHV_STATE_ARIBORNE_Z 3
#define BHV_STATE_KEEP      255

typedef struct behavior_def_t {
    UBYTE flags;         // BHV_* physics components
    UBYTE flags2;        // BHV2_* secondary behavior/animation + BHV3_* lock flags
    UBYTE collision_type;// DYNAMIC_ACTOR_COLLISION_* collision model
    UBYTE gravity;       // subpixels/frame^2 added to y velocity
    BYTE max_fall_vel;  // max downward velocity in subpixels/frame
    UBYTE bounce;        // energy kept on bounce, 0..255 (255 = perfect reflect)
} behavior_def_t;

extern behavior_def_t behavior_defs[DYNAMIC_ACTOR_MAX_BEHAVIORS + 1];

void dynamic_actor_init(void) BANKED;
void dynamic_actor_update(void) BANKED;

#endif



