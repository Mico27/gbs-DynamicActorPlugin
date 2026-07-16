#pragma bank 255

#include <string.h>
#include <stdlib.h>
#include <gbdk/platform.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "math.h"
#include "actor.h"
#include "game_time.h"
#include "dynamic_actor.h"
#include "sincos.h"
#include "data/states_defines.h"
#include "collision.h"
#include "macro.h"

extern behavior_def_t behavior_defs[DYNAMIC_ACTOR_MAX_BEHAVIORS + 1];

#ifndef ACTOR_ATTR_H_FIRST
#define ACTOR_ATTR_H_FIRST            0x01
#define ACTOR_ATTR_CHECK_COLL_WALLS   0x02
#define ACTOR_ATTR_DIAGONAL           0x04
#define ACTOR_ATTR_RELATIVE_SNAP_PX   0x08
#define ACTOR_ATTR_RELATIVE_SNAP_TILE 0x10
#define ACTOR_ATTR_CHECK_COLL_ACTORS  0x20
#endif

#define MTPBV_ALLOW_H   0x01
#define MTPBV_ALLOW_V   0x02
#define MTPBV_DIR_H     0x04
#define MTPBV_DIR_V     0x08
#define MTPBV_NEEDED_H  0x10
#define MTPBV_NEEDED_V  0x20
#define MTPBV_H         (MTPBV_ALLOW_H | MTPBV_NEEDED_H)
#define MTPBV_V         (MTPBV_ALLOW_V | MTPBV_NEEDED_V)

inline uint8_t scale8(uint8_t i, uint8_t scale) {
    return (((uint16_t)i) * (1 + (uint16_t)scale)) >> 8;
}

inline uint8_t lerp8by8(uint8_t a, uint8_t b, uint8_t frac) {
    if (b > a) {
        uint8_t d = b - a;
        uint8_t s = scale8(d, frac);
        return a + s;
    } else {
        uint8_t d = a - b;
        uint8_t s = scale8(d, frac);
        return a - s;
    }
}

inline uint8_t quadratic_bezier(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t t) {
    p0 = lerp8by8(p0, p1, t);
    p1 = lerp8by8(p1, p2, t);
    return lerp8by8(p0, p1, t);
}

static uint8_t cubic_bezier(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t t) {
    p0 = lerp8by8(p0, p1, t);
    p1 = lerp8by8(p1, p2, t);
    p2 = lerp8by8(p2, p3, t);
    p0 = lerp8by8(p0, p1, t);
    p1 = lerp8by8(p1, p2, t);
    return lerp8by8(p0, p1, t);
}

void vm_define_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    UBYTE slot = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    if ((slot == 0) || (slot > DYNAMIC_ACTOR_MAX_BEHAVIORS)) return;
    behavior_def_t *def = &behavior_defs[slot];
    def->collision_type = *(uint8_t *)VM_REF_TO_PTR(FN_ARG6);
    def->flags        = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    def->flags2       = *(uint8_t *)VM_REF_TO_PTR(FN_ARG2);
    def->gravity      = *(uint8_t *)VM_REF_TO_PTR(FN_ARG3);
    def->max_fall_vel = *(uint8_t *)VM_REF_TO_PTR(FN_ARG4);
    def->bounce       = *(uint8_t *)VM_REF_TO_PTR(FN_ARG5);
}

void vm_set_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_behavior_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    UBYTE state = *(uint8_t *)VM_REF_TO_PTR(FN_ARG2);
    if (state != BHV_STATE_KEEP) {
        actor->actor_state = state;
    }
}

void vm_get_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 2; else A = script_memory + idx;
    *A = actor->actor_behavior_id;
}

void vm_set_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_state = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
}

void vm_get_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 2; else A = script_memory + idx;
    *A = actor->actor_state;
}

void vm_set_actor_velocity(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_vel_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor->actor_vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG2);
}

void vm_set_actor_velocity_x(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_vel_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
}

void vm_get_actor_velocity_x(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 2; else A = script_memory + idx;
    *A = actor->actor_vel_x;
}

void vm_set_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
}

void vm_get_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 2; else A = script_memory + idx;
    *A = actor->actor_vel_y;
}

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
void vm_set_actor_parent(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int8_t parent_actor_idx = *(int8_t *)VM_REF_TO_PTR(FN_ARG1);
    if (parent_actor_idx == -1) {
        actor->actor_parent = NULL;
    } else {
        actor->actor_parent = actors + parent_actor_idx;
    }
}

void vm_get_actor_parent(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 2; else A = script_memory + idx;
    if (!actor->actor_parent) {
        *A = -1;
    } else {
        *A = (int16_t)(actor->actor_parent - actors);
    }
}
#endif

void vm_get_tile_collision(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    uint8_t tile_x = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    uint8_t tile_y = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
    int16_t * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 3; else A = script_memory + idx;
    *A = tile_at(tile_x, tile_y);
}

void vm_get_actor_collision(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    uint16_t point_x = PX_TO_SUBPX(*(uint16_t *)VM_REF_TO_PTR(FN_ARG0));
    uint16_t point_y = PX_TO_SUBPX(*(uint16_t *)VM_REF_TO_PTR(FN_ARG1));
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
    int16_t * A;
    actor_t *actor = actors_active_tail;
    if (idx < 0) A = THIS->stack_ptr + idx - 3; else A = script_memory + idx;
    while (actor) {
        if (actor->flags & ACTOR_FLAG_COLLISION) {
            UWORD left = actor->pos.x + actor->bounds.left;
            UWORD right = actor->pos.x + actor->bounds.right;
            UWORD top = actor->pos.y + actor->bounds.top;
            UWORD bottom = actor->pos.y + actor->bounds.bottom;
            if ((point_x >= left) && (point_x <= right) &&
                (point_y >= top) && (point_y <= bottom)) {
                *A = (int16_t)(actor - actors);
                return;
            }
        }
        actor = actor->prev;
    }
    *A = -1;
}

#ifdef DYNAMIC_ACTOR_ENABLE_VM_MOTION_CHASE_ACTOR
UBYTE vm_actor_chase_actor(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t *actor = actors + (UBYTE)stack_frame[0];
    if (start){
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
    } else {
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }
    actor_t *target = actors + (UBYTE)stack_frame[1];
    UBYTE flee = (UBYTE)stack_frame[2];
    UWORD range = stack_frame[3];
    UBYTE interval = (UBYTE)stack_frame[4];
    WORD speed = actor->move_speed >> 1;
    UBYTE steer_y = 1;

    if (start || ((game_time & interval) == 0)) {
        stack_frame[5] = target->pos.x;
        stack_frame[6] = target->pos.y;
    }

    UWORD target_x = stack_frame[5];
    UWORD target_y = stack_frame[6];

    if (flee) {
        speed = -speed;
    }

#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
    if (behavior_defs[actor->actor_behavior_id].flags & BHV_GRAVITY) {
        steer_y = 0;
    }
#endif

    WORD dx = (WORD)(target_x - actor->pos.x);
    WORD dy = (WORD)(target_y - actor->pos.y);
    UWORD adx = (dx < 0) ? (UWORD)(-dx) : (UWORD)dx;
    UWORD ady = (dy < 0) ? (UWORD)(-dy) : (UWORD)dy;

    if (dx > speed) {
        actor->actor_vel_x = speed;
    } else if (dx < -speed) {
        actor->actor_vel_x = -speed;
    } else {
        actor->actor_vel_x = 0;
    }
    if (steer_y) {
        if (dy > speed) {
            actor->actor_vel_y = speed;
        } else if (dy < -speed) {
            actor->actor_vel_y = -speed;
        } else {
            actor->actor_vel_y = 0;
        }
    }

    if (range) {
        UBYTE done;
        if (flee) {
            done = (adx > range) || (steer_y && (ady > range));
        } else {
            done = (adx <= range) && (!steer_y || (ady <= range));
        }
        if (done) {
            actor->actor_vel_x = 0;
            if (steer_y) {
                actor->actor_vel_y = 0;
            }
            return TRUE;
        }
    }

    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_VM_MOTION_MOVE_TO_POS_BY_VELOCITY
UBYTE vm_actor_move_to_pos_by_velocity(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t *actor = actors + (UBYTE)stack_frame[0];
    UBYTE attr = (UBYTE)stack_frame[3];
    UBYTE direct_to_point = (UBYTE)stack_frame[4];
    UBYTE cancel_on_collision = (UBYTE)stack_frame[5];
    UBYTE flags;

    if (start) {
        UWORD target_x;
        UWORD target_y;

        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);

        target_x = PX_TO_SUBPX(stack_frame[1]);
        target_y = PX_TO_SUBPX(stack_frame[2]);

        if (CHK_FLAG(attr, ACTOR_ATTR_RELATIVE_SNAP_PX)) {
            target_x = SUBPX_SNAP_PX(target_x + actor->pos.x);
            target_y = SUBPX_SNAP_PX(target_y + actor->pos.y);
        } else if (CHK_FLAG(attr, ACTOR_ATTR_RELATIVE_SNAP_TILE)) {
            target_x = SUBPX_SNAP_TILE(target_x + actor->pos.x);
            target_y = SUBPX_SNAP_TILE(target_y + actor->pos.y);
        }

        stack_frame[1] = target_x;
        stack_frame[2] = target_y;

        flags = 0;
        if (CHK_FLAG(attr, ACTOR_ATTR_DIAGONAL)) {
            flags |= (MTPBV_ALLOW_H | MTPBV_ALLOW_V);
        } else if (CHK_FLAG(attr, ACTOR_ATTR_H_FIRST)) {
            flags |= MTPBV_ALLOW_H;
        } else {
            flags |= MTPBV_ALLOW_V;
        }

        if (actor->pos.x != target_x) {
            flags |= MTPBV_NEEDED_H;
        } else {
            flags |= MTPBV_ALLOW_V;
        }
        if (actor->pos.y != target_y) {
            flags |= MTPBV_NEEDED_V;
        } else {
            flags |= MTPBV_ALLOW_H;
        }

        if (actor->pos.x > target_x) {
            flags |= MTPBV_DIR_H;
        }
        if (actor->pos.y > target_y) {
            flags |= MTPBV_DIR_V;
        }

        stack_frame[6] = flags;
        stack_frame[7] = actor->pos.x;
        stack_frame[8] = actor->pos.y;
        stack_frame[9] = 0;
    } else {
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            actor->actor_vel_x = 0;
            actor->actor_vel_y = 0;
            return TRUE;
        }

        if (cancel_on_collision) {
            if ((((stack_frame[9] >> 8) != 0) && (actor->pos.x == stack_frame[7])) ||
             (((stack_frame[9] & 0xFF) != 0) && (actor->pos.y == stack_frame[8]))) {
                actor->actor_vel_x = 0;
                actor->actor_vel_y = 0;
                return TRUE;
            }
        }
    }

    UWORD target_x = stack_frame[1];
    UWORD target_y = stack_frame[2];
    flags = (UBYTE)stack_frame[6];
    WORD speed = actor->move_speed >> 1;
    UBYTE move_h;
    UBYTE move_v;

    actor->actor_vel_x = 0;
    actor->actor_vel_y = 0;

    if (direct_to_point) {
        WORD dx = (WORD)(target_x - actor->pos.x);
        WORD dy = (WORD)(target_y - actor->pos.y);
        UBYTE near_x = (dx <= speed) && (dx >= -speed);
        UBYTE near_y = (dy <= speed) && (dy >= -speed);

        if (near_x && near_y) {
            actor->actor_vel_x = 0;
            actor->actor_vel_y = 0;
            return TRUE;
        }

        WORD t_dx = (WORD)SUBPX_TO_TILE(target_x) - (WORD)SUBPX_TO_TILE(actor->pos.x);
        WORD t_dy = (WORD)SUBPX_TO_TILE(target_y) - (WORD)SUBPX_TO_TILE(actor->pos.y);
        if ((t_dx == 0) && (t_dy == 0)) {
            // closer than a tile: use the px delta for direction
            t_dx = (WORD)SUBPX_TO_PX(target_x) - (WORD)SUBPX_TO_PX(actor->pos.x);
            t_dy = (WORD)SUBPX_TO_PX(target_y) - (WORD)SUBPX_TO_PX(actor->pos.y);
        }
        UBYTE angle = atan2(t_dy, t_dx);
        if (!near_x) {
            actor->actor_vel_x = (WORD)(SIN(angle) * speed) >> 7;
            if ((actor->actor_vel_x == 0) && (dx != 0)) {
                actor->actor_vel_x = (dx > 0) ? speed : -speed;
            }
        } 
        if (!near_y) {
            actor->actor_vel_y = -((WORD)(COS(angle) * speed) >> 7);
            if ((actor->actor_vel_y == 0) && (dy != 0)) {
                actor->actor_vel_y = (dy > 0) ? speed : -speed;
            }
        } 
        

    } else {
        move_h = (CHK_FLAG(flags, MTPBV_H) == MTPBV_H);
        move_v = (CHK_FLAG(flags, MTPBV_V) == MTPBV_V);

        if (!CHK_FLAG(attr, ACTOR_ATTR_DIAGONAL)) {
            if (move_h) {
                move_v = FALSE;
            } else if (move_v) {
                move_h = FALSE;
            }
        }

        if (move_h) {
            WORD dx = (WORD)(target_x - actor->pos.x);
            if (dx > speed) {
                actor->actor_vel_x = speed;
            } else if (dx < -speed) {
                actor->actor_vel_x = -speed;
            } else {
                actor->actor_vel_x = dx;
                CLR_FLAG(flags, MTPBV_NEEDED_H);
                SET_FLAG(flags, MTPBV_ALLOW_V);
            }
        }

        if (move_v) {
            WORD dy = (WORD)(target_y - actor->pos.y);
            if (dy > speed) {
                actor->actor_vel_y = speed;
            } else if (dy < -speed) {
                actor->actor_vel_y = -speed;
            } else {
                actor->actor_vel_y = dy;
                CLR_FLAG(flags, MTPBV_NEEDED_V);
                SET_FLAG(flags, MTPBV_ALLOW_H);
            }
        }
    }

    stack_frame[6] = flags;
    stack_frame[7] = actor->pos.x;
    stack_frame[8] = actor->pos.y;
    stack_frame[9] = (actor->actor_vel_x << 8) | actor->actor_vel_y;

    if (!CHK_FLAG(flags, MTPBV_NEEDED_H | MTPBV_NEEDED_V)) {
        actor->actor_vel_x = 0;
        actor->actor_vel_y = 0;
        return TRUE;
    }

    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_VM_MOTION_CIRCLE_VARIABLE
// Waitable circle orbit at the actor's movement speed. The orbital angle is
// re-derived every update from the actor's actual position around the circle
// center (atan2 on the delta in tiles), and the actor steers toward the point
// on the circle a fixed lead ahead of that angle - so the orbit self-corrects
// instead of drifting, and physics (walls, being pushed) just bend the path.
// The aim radius is slightly enlarged to offset the chord cutting inward.
// stack_frame:
// [0] actor idx
// [1] radius in px (clamped 1-160 on start)
// [2] duration in frames (0 = forever), counts down every frame
// [3] ccw flag
// [4] start angle (0 = top of circle) - only used on start to place the center
// [5] update interval in frames (1-16)
// [6] circle center x in px (computed on start)
// [7] circle center y in px (computed on start)
// [8] frames left until the next steering update
#define CIRCLE_VAR_LEAD 32
UBYTE vm_actor_motion_circle_variable(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t *actor = actors + (UBYTE)stack_frame[0];

    if (start) {
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);

        UWORD radius = stack_frame[1];
        UBYTE angle = (UBYTE)stack_frame[4];
        UBYTE interval = (UBYTE)stack_frame[5];

        if (radius < 1) radius = 1;
        if (radius > 160) radius = 160;
        if (interval < 1) interval = 1;
        if (interval > 16) interval = 16;

        // the actor starts on the circle at the start angle
        stack_frame[6] = (UWORD)(SUBPX_TO_PX(actor->pos.x) - (((WORD)radius * SIN(angle)) >> 7));
        stack_frame[7] = (UWORD)(SUBPX_TO_PX(actor->pos.y) + (((WORD)radius * COS(angle)) >> 7));
        stack_frame[1] = radius;
        stack_frame[5] = interval;
        stack_frame[8] = 0;
    } else {
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }

    if (stack_frame[2]) {
        stack_frame[2] -= 1;
        if (!stack_frame[2]) {
            return TRUE;
        }
    }

    if (stack_frame[8]) {
        stack_frame[8] -= 1;
        ((SCRIPT_CTX *)THIS)->waitable = TRUE;
        return FALSE;
    }

    {
        WORD radius = (WORD)stack_frame[1];
        WORD center_x = (WORD)stack_frame[6];
        WORD center_y = (WORD)stack_frame[7];
        WORD speed = actor->move_speed >> 1;

        // current orbital angle from the center-to-actor delta in tiles
        UBYTE angle = atan2((WORD)SUBPX_TO_TILE(actor->pos.y) - (center_y >> 3),
                            (WORD)SUBPX_TO_TILE(actor->pos.x) - (center_x >> 3));

        if (stack_frame[3]) {
            angle -= CIRCLE_VAR_LEAD;
        } else {
            angle += CIRCLE_VAR_LEAD;
        }

        // aim slightly outside the true circle (~1/cos(lead/2)) so steering
        // along the chord settles on the requested radius
        WORD aim = radius + (radius >> 4);
        UWORD target_x = PX_TO_SUBPX((UWORD)(center_x + ((aim * SIN(angle)) >> 7)));
        UWORD target_y = PX_TO_SUBPX((UWORD)(center_y - ((aim * COS(angle)) >> 7)));

        WORD dx = (WORD)(target_x - actor->pos.x);
        WORD dy = (WORD)(target_y - actor->pos.y);

        WORD t_dx = (WORD)SUBPX_TO_TILE(target_x) - (WORD)SUBPX_TO_TILE(actor->pos.x);
        WORD t_dy = (WORD)SUBPX_TO_TILE(target_y) - (WORD)SUBPX_TO_TILE(actor->pos.y);
        if ((t_dx == 0) && (t_dy == 0)) {
            // circle smaller than a tile: use the px delta for direction
            t_dx = (WORD)SUBPX_TO_PX(target_x) - (WORD)SUBPX_TO_PX(actor->pos.x);
            t_dy = (WORD)SUBPX_TO_PX(target_y) - (WORD)SUBPX_TO_PX(actor->pos.y);
        }
        UBYTE move_angle = atan2(t_dy, t_dx);

        actor->actor_vel_x = (WORD)(SIN(move_angle) * speed) >> 7;
        if ((actor->actor_vel_x == 0) && (dx != 0)) {
            actor->actor_vel_x = (dx > 0) ? speed : -speed;
        }
        actor->actor_vel_y = -((WORD)(COS(move_angle) * speed) >> 7);
        if ((actor->actor_vel_y == 0) && (dy != 0)) {
            actor->actor_vel_y = (dy > 0) ? speed : -speed;
        }

        stack_frame[8] = (UWORD)((UBYTE)stack_frame[5] - 1);
    }

    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_VM_MOTION_BEZIER_TO
// Waitable bezier motion that drives actor velocity toward the sampled point
// each step instead of setting actor position directly.
// stack_frame:
// [0] actor idx
// [1] cached actor start x in px (set on start)
// [2] cached actor start y in px (set on start)
// [3] bezier type (0=quadratic, else cubic)
// [4] packed incremental/lerp (high byte = incremental, low byte = lerp)
// [5] point0 packed xy (low=x, high=y)
// [6] point1 packed xy
// [7] point2 packed xy
// [8] point3 packed xy (cubic only)
UBYTE vm_actor_move_bezier_to(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    
    UWORD target_x;
    UWORD target_y;
    WORD dx;
    WORD dy;
    UBYTE next_x = stack_frame[9];
    UBYTE next_y = stack_frame[10];
    UBYTE lerp = stack_frame[4] & 255;
    actor_t *actor = actors + stack_frame[0];

    if (start) {
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
        stack_frame[1] = SUBPX_TO_PX(actor->pos.x);
        stack_frame[2] = SUBPX_TO_PX(actor->pos.y);
    }

    if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
        return TRUE;
    }

    BYTE point0_x = stack_frame[5] & 255;
    BYTE point0_y = stack_frame[5] >> 8;
    WORD speed = actor->move_speed >> 1;
    // If the next bezier sample is still out of reach, keep waiting for actor motion to catch up.
    if (!start && lerp != 255) {
        target_x = PX_TO_SUBPX(stack_frame[1] - point0_x + next_x);
        target_y = PX_TO_SUBPX(stack_frame[2] - point0_y + next_y);
        dx = (WORD)(target_x - actor->pos.x);
        dy = (WORD)(target_y - actor->pos.y);
        if ((abs(dx) > speed) || (abs(dy) > speed)) {
            script_memory[0] = 255;
            ((SCRIPT_CTX *)THIS)->waitable = TRUE;
            return FALSE;
        }
    }

    {
        if (stack_frame[3] == 0) {
            stack_frame[9] = next_x = quadratic_bezier(point0_x, stack_frame[6] & 255, stack_frame[7] & 255, lerp);
            stack_frame[10] = next_y = quadratic_bezier(point0_y, stack_frame[6] >> 8, stack_frame[7] >> 8, lerp);
        } else {
            stack_frame[9] = next_x = cubic_bezier(point0_x, stack_frame[6] & 255, stack_frame[7] & 255, stack_frame[8] & 255, lerp);
            stack_frame[10] = next_y = cubic_bezier(point0_y, stack_frame[6] >> 8, stack_frame[7] >> 8, stack_frame[8] >> 8, lerp);
        }

        target_x = PX_TO_SUBPX(stack_frame[1] - point0_x + next_x);
        target_y = PX_TO_SUBPX(stack_frame[2] - point0_y + next_y);

        dx = (WORD)(target_x - actor->pos.x);
        dy = (WORD)(target_y - actor->pos.y);

        WORD t_dx = (WORD)SUBPX_TO_TILE(target_x) - (WORD)SUBPX_TO_TILE(actor->pos.x);
        WORD t_dy = (WORD)SUBPX_TO_TILE(target_y) - (WORD)SUBPX_TO_TILE(actor->pos.y);
        if ((t_dx == 0) && (t_dy == 0)) {
            // closer than a tile: use the px delta for direction
            t_dx = (WORD)SUBPX_TO_PX(target_x) - (WORD)SUBPX_TO_PX(actor->pos.x);
            t_dy = (WORD)SUBPX_TO_PX(target_y) - (WORD)SUBPX_TO_PX(actor->pos.y);
        }
        UBYTE angle = atan2(t_dy, t_dx);
        actor->actor_vel_x = (WORD)(SIN(angle) * speed) >> 7;
        if ((actor->actor_vel_x == 0) && (dx != 0)) {
            actor->actor_vel_x = (dx > 0) ? speed : -speed;
        }
        actor->actor_vel_y = -((WORD)(COS(angle) * speed) >> 7);
        if ((actor->actor_vel_y == 0) && (dy != 0)) {
            actor->actor_vel_y = (dy > 0) ? speed : -speed;
        }

        UBYTE incremental = stack_frame[4] >> 8;
        if (lerp != 255) {
            if ((255 - incremental) > lerp) {
                lerp += incremental;
            } else {
                lerp = 255;
            }
            stack_frame[4] = (incremental << 8) + lerp;
            ((SCRIPT_CTX *)THIS)->waitable = TRUE;
            return FALSE;
        }

    }
    return TRUE;
}
#endif
