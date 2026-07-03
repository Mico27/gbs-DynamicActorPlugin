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
#define BHV_LINKED      0x40u  // follow linked actor at (vel_x, vel_y) offset; skips physics

// Animation component flags
#define BHV2_ANIM_FACE  0x01u  // face x velocity direction
#define BHV2_ANIM_IDLE  0x02u  // idle animation when x velocity is zero
#define BHV2_ANIM_JUMP  0x04u  // jump animation while airborne

// Actor behavior states (actor_state)
#define BHV_STATE_PAUSED    0
#define BHV_STATE_GROUNDED  1
#define BHV_STATE_AIRBORNE  2
#define BHV_STATE_KEEP      255

typedef struct behavior_def_t {
    UBYTE flags;         // BHV_* physics components
    UBYTE flags2;        // BHV2_* animation components
    UBYTE gravity;       // subpixels/frame^2 added to y velocity
    UBYTE max_fall_vel;  // max downward velocity in subpixels/frame
    UBYTE bounce;        // energy kept on bounce, 0..255 (255 = perfect reflect)
} behavior_def_t;

extern behavior_def_t behavior_defs[DYNAMIC_ACTOR_MAX_BEHAVIORS + 1];

void dynamic_actor_init(void) BANKED;
void dynamic_actor_update(void) BANKED;

#endif
