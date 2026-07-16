#ifndef DYNAMIC_ACTOR_H
#define DYNAMIC_ACTOR_H

#include <gbdk/platform.h>
#include "actor.h"
#include "data/states_defines.h"

// Number of user-definable behavior slots (slot 0 is reserved = no behavior)
#ifndef DYNAMIC_ACTOR_MAX_BEHAVIORS
#define DYNAMIC_ACTOR_MAX_BEHAVIORS 8
#endif

// Physics component flags
#define BHV_GRAVITY     0x01u  // apply gravity to y velocity, clamped to max_fall_vel
#define BHV_MOVE_X      0x02u  // apply x velocity with horizontal tile collision
#define BHV_MOVE_Y      0x04u  // apply y velocity with vertical tile collision
#define BHV_LEDGE_STOP  0x08u  // while grounded, treat ledges/pits as walls
#define BHV_REFLECT_X   0x10u  // reverse x velocity on wall hit (otherwise stop)
#define BHV_REFLECT_Y   0x20u  // bounce y velocity on floor/ceiling hit (otherwise stop)
#define BHV_PLATFORM    0x40u  // moving platform: sets itself as the parent of every
                               // actor it intersects (unless that actor already has a
                               // different parent, or the platform has a collision
                               // group and the actor's group differs), and clears
                               // itself as parent when the actor stops intersecting.
                               // Parented actors inherit the platform's movement.
                               // 0x80 free

// Animation / option flags (flags2)
#define BHV2_ANIM_FACE  0x01u  // face x velocity direction (left/right)
#define BHV2_ANIM_IDLE  0x02u  // idle animation when x velocity is zero
#define BHV2_ANIM_JUMP  0x04u  // jump animation while airborne
#define BHV2_NO_TILE_COLLISION 0x08u  // move by velocity without tile collision
                                      // (passes through walls/floors; no wall turn,
                                      // bounce, ledge stop or landing)
#define BHV2_ANIM_FACE_4DIR    0x10u  // face dominant velocity axis (up/down/left/
                                      // right) - for top down / adventure actors
                                      // 0x20, 0x40 free
#define BHV2_ACTOR_COLLISION   0x80u  // collide with other actors (player excluded -
                                      // the engine already handles that): on overlap
                                      // the frame's movement is reverted and velocity
                                      // turns/bounces per the reflect settings

// Actor behavior states (actor_state)
#define BHV_STATE_PAUSED    0
#define BHV_STATE_GROUNDED  1
#define BHV_STATE_AIRBORNE  2
#define BHV_STATE_KEEP      255

typedef struct behavior_def_t {
    UBYTE flags;         // BHV_* physics components
    UBYTE flags2;        // BHV2_* animation/option flags
    UBYTE collision_type;// DYNAMIC_ACTOR_COLLISION_* collision model
    UBYTE gravity;       // subpixels/frame^2 added to y velocity
    BYTE max_fall_vel;  // max downward velocity in subpixels/frame
    UBYTE bounce;        // energy kept on bounce, 0..255 (255 = perfect reflect)
} behavior_def_t;

extern behavior_def_t behavior_defs[DYNAMIC_ACTOR_MAX_BEHAVIORS + 1];

void dynamic_actor_init(void) BANKED;
void dynamic_actor_update(void) BANKED;

#endif



