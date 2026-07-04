#pragma bank 255

#include <string.h>
#include <gbdk/platform.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "math.h"
#include "actor.h"
#include "game_time.h"
#include "dynamic_actor.h"
#include "data/states_defines.h"
#include "collision.h"
#include "macro.h"

#define DYNAMIC_ACTOR_COLLISION_SINGLE_POINT 0
#define DYNAMIC_ACTOR_COLLISION_TRIANGLE 1
#define DYNAMIC_ACTOR_COLLISION_BOUNDING_BOX 2
#ifndef DYNAMIC_ACTOR_COLLISION_TYPE
#define DYNAMIC_ACTOR_COLLISION_TYPE DYNAMIC_ACTOR_COLLISION_SINGLE_POINT
#endif

#define COLLISION_SLOPE_LEFT          0x10u
#define COLLISION_SLOPE_45            0x20u
#define COLLISION_SLOPE_225_BOT       0x40u
#define COLLISION_SLOPE_225_TOP       (COLLISION_SLOPE_45 | COLLISION_SLOPE_225_BOT)
#define COLLISION_SLOPE_45_RIGHT      COLLISION_SLOPE_45
#define COLLISION_SLOPE_225_RIGHT_BOT COLLISION_SLOPE_225_BOT
#define COLLISION_SLOPE_225_RIGHT_TOP COLLISION_SLOPE_225_TOP
#define COLLISION_SLOPE_45_LEFT       (COLLISION_SLOPE_LEFT | COLLISION_SLOPE_45)
#define COLLISION_SLOPE_225_LEFT_BOT  (COLLISION_SLOPE_LEFT | COLLISION_SLOPE_225_BOT)
#define COLLISION_SLOPE_225_LEFT_TOP  (COLLISION_SLOPE_LEFT | COLLISION_SLOPE_225_TOP)
#define COLLISION_SLOPE_ANY           (COLLISION_SLOPE_45 | COLLISION_SLOPE_225_BOT | COLLISION_SLOPE_225_TOP)
#define COLLISION_SLOPE               0x70u

#define IS_ON_SLOPE(t) ((t) & COLLISION_SLOPE_ANY)
#define IS_SLOPE_LEFT(t) ((t) & COLLISION_SLOPE_LEFT)
#define IS_SLOPE_RIGHT(t) (!((t) & COLLISION_SLOPE_LEFT))

behavior_def_t behavior_defs[DYNAMIC_ACTOR_MAX_BEHAVIORS + 1];

WORD new_actor_x;
WORD new_actor_y;
UBYTE col_tx;
UBYTE col_ty;

void dynamic_actor_init(void) BANKED {
    memset(behavior_defs, 0, sizeof(behavior_defs));
}

#if DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_SINGLE_POINT

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
static UWORD check_vertical_collision(UWORD start_x, UWORD start_y, UBYTE down) {
    col_ty = SUBPX_TO_TILE(start_y);
    col_tx = SUBPX_TO_TILE(start_x);
    if (down) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
        UBYTE tile = tile_at(col_tx, col_ty);
        if (tile & COLLISION_TOP) {
            start_y = TILE_TO_SUBPX(col_ty) - 1;
            col_ty = SUBPX_TO_TILE(start_y);
            tile = tile_at(col_tx, col_ty);
        }
        if (IS_ON_SLOPE(tile)){
            const UBYTE slope_type = (tile & COLLISION_SLOPE);
            UBYTE x_offset = SUBPX_TILE_REMAINDER(start_x);
            WORD offset = 0;

            switch (slope_type) {
                case COLLISION_SLOPE_45_RIGHT:
                    offset = (PX_TO_SUBPX(8) - x_offset);
                    break;
                case COLLISION_SLOPE_225_RIGHT_BOT:
                    offset = (PX_TO_SUBPX(8) - DIV_2(x_offset));
                    break;
                case COLLISION_SLOPE_225_RIGHT_TOP:
                    offset = (PX_TO_SUBPX(4) - DIV_2(x_offset));
                    break;
                case COLLISION_SLOPE_45_LEFT:
                    offset = x_offset;
                    break;
                case COLLISION_SLOPE_225_LEFT_BOT:
                    offset = DIV_2(x_offset) + PX_TO_SUBPX(4);
                    break;
                case COLLISION_SLOPE_225_LEFT_TOP:
                    offset = DIV_2(x_offset);
                    break;
            }
            UWORD slope_y_coord = TILE_TO_SUBPX(col_ty) + offset - 32;
            if (slope_y_coord < start_y){
                return slope_y_coord;
            }
            return start_y;
        }
        return start_y;
#else
        if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
            return TILE_TO_SUBPX(col_ty) - 1;
        }
        return start_y;
#endif
    }
    if (tile_at(col_tx, col_ty) & COLLISION_BOTTOM) {
        return TILE_TO_SUBPX(col_ty + 1);
    }
    return start_y;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
static UWORD check_horizontal_collision(UWORD start_x, UWORD start_y, UBYTE right) {
    col_ty = SUBPX_TO_TILE(start_y);
    col_tx = SUBPX_TO_TILE(start_x);
    if (right) {

        if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, col_ty))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - 1;
        }
        return start_x;
    }

    if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, col_ty))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1);
    }
    return start_x;
}
#endif

#if defined(DYNAMIC_ACTOR_ENABLE_MOVE_X) && defined(DYNAMIC_ACTOR_ENABLE_LEDGE_STOP)
static UWORD check_pit(UWORD start_x, UWORD start_y, UBYTE right) {
    col_ty = SUBPX_TO_TILE(start_y);
    col_tx = SUBPX_TO_TILE(start_x);
    if (right) {
        if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, col_ty))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - 1;
        }
        if (!(tile_at(col_tx, col_ty + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
            return TILE_TO_SUBPX(col_tx) - 1;
        }
        return start_x;
    }
    if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, col_ty))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1);
    }
    if (!(tile_at(col_tx, col_ty + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
        return TILE_TO_SUBPX(col_tx + 1);
    }
    return start_x;
}
#endif

#define CHECK_COL_H(x, y, actor, right) check_horizontal_collision((x), (y), (right))
#define CHECK_COL_V(x, y, actor, down)  check_vertical_collision((x), (y), (down))
#define CHECK_COL_PIT(x, y, actor, right) check_pit((x), (y), (right))

#else

#if defined(DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION) && defined(DYNAMIC_ACTOR_ENABLE_MOVE_Y)
static UBYTE on_slope;
static UWORD check_collision_slope(UWORD start_x, UWORD start_y, rect16_t *bounds){
    col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
    col_tx = SUBPX_TO_TILE(start_x);
    UBYTE tile = tile_at(col_tx, col_ty);
    if (tile & COLLISION_TOP) {
        start_y = (TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1));
        col_ty = SUBPX_TO_TILE(start_y + (bounds->bottom - 1));
        tile = tile_at(col_tx, col_ty);

    }
    on_slope = IS_ON_SLOPE(tile);
    if (on_slope){
        const UBYTE slope_type = (tile & COLLISION_SLOPE);
        UBYTE x_offset = SUBPX_TILE_REMAINDER(start_x);
        WORD offset = 0;
        switch (slope_type) {
            case COLLISION_SLOPE_45_RIGHT:
                offset = (PX_TO_SUBPX(8) - x_offset) - bounds->bottom;
                break;
            case COLLISION_SLOPE_225_RIGHT_BOT:
                offset = (PX_TO_SUBPX(8) - DIV_2(x_offset)) - bounds->bottom;
                break;
            case COLLISION_SLOPE_225_RIGHT_TOP:
                offset = (PX_TO_SUBPX(4) - DIV_2(x_offset)) - bounds->bottom;
                break;
            case COLLISION_SLOPE_45_LEFT:
                offset = x_offset - bounds->bottom;
                break;
            case COLLISION_SLOPE_225_LEFT_BOT:
                offset = DIV_2(x_offset) - bounds->bottom + PX_TO_SUBPX(4);
                break;
            case COLLISION_SLOPE_225_LEFT_TOP:
                offset = DIV_2(x_offset) - bounds->bottom;
                break;
        }
        UWORD slope_y_coord = TILE_TO_SUBPX(col_ty) + offset - 32;
        if (slope_y_coord < start_y){
            return slope_y_coord;
        }
    }
    return start_y;
}

#endif

#if DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_TRIANGLE

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
static UWORD check_vertical_collision(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE down) {
    if (down) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
        UWORD middle_pos = start_x + bounds->left + ((bounds->right - bounds->left) >> 1);
        return check_collision_slope(middle_pos, start_y, bounds);
#else
        col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
        col_tx = SUBPX_TO_TILE(start_x + bounds->left);
        if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
            return TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1);
        }
        col_tx = SUBPX_TO_TILE(start_x + bounds->right);
        if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
            return TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1);
        }
        return start_y;
#endif
    }
    col_ty = SUBPX_TO_TILE(start_y + bounds->top);
    col_tx = SUBPX_TO_TILE(start_x + bounds->left + ((bounds->right - bounds->left) >> 1));
    if (tile_at(col_tx, col_ty) & COLLISION_BOTTOM) {
        return TILE_TO_SUBPX(col_ty + 1) - bounds->top;
    }
    return start_y;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
static UWORD check_horizontal_collision(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE right) {
    if (right) {
        col_tx = SUBPX_TO_TILE(start_x + bounds->right);
        col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
        if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, col_ty))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        return start_x;
    }
    col_tx = SUBPX_TO_TILE(start_x + bounds->left);
    col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
    if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, col_ty))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1) - bounds->left;
    }
    return start_x;
}
#endif

#if defined(DYNAMIC_ACTOR_ENABLE_MOVE_X) && defined(DYNAMIC_ACTOR_ENABLE_LEDGE_STOP)
static UWORD check_pit(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE right) {
    if (right) {
        col_tx = SUBPX_TO_TILE(start_x + bounds->right);
        col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
        if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, col_ty))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        if (!(tile_at(col_tx, col_ty + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        return start_x;
    }
    col_tx = SUBPX_TO_TILE(start_x + bounds->left);
    col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
    if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, col_ty))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1) - bounds->left;
    }
    if (!(tile_at(col_tx, col_ty + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
        return TILE_TO_SUBPX(col_tx + 1)  - bounds->left;
    }
    return start_x;
}
#endif

#else //DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_BOUNDING_BOX

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
static UWORD check_vertical_collision(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE down) {
    UBYTE tile_x_start = SUBPX_TO_TILE(start_x + bounds->left);
    UBYTE tile_x_end = SUBPX_TO_TILE(start_x + bounds->right);
    if (down) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
        UWORD middle_pos = start_x + bounds->left + ((bounds->right - bounds->left) >> 1);
        start_y = check_collision_slope(middle_pos, start_y, bounds);
        if (on_slope){
            return start_y;
        }
#endif
        col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
        if (tile_col_test_range_x(COLLISION_TOP, col_ty, tile_x_start, tile_x_end)){
            return TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1);
        }
        return start_y;
    }
    col_ty = SUBPX_TO_TILE(start_y + bounds->top);
    if (tile_col_test_range_x(COLLISION_BOTTOM, col_ty, tile_x_start, tile_x_end)){
        return TILE_TO_SUBPX(col_ty + 1) - bounds->top;
    }
    return start_y;
}
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
static UWORD check_horizontal_collision(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE right) {
    UBYTE tile_y_start = SUBPX_TO_TILE(start_y + bounds->bottom);
    UBYTE tile_y_end = SUBPX_TO_TILE(start_y + bounds->top);
    if (right) {
        col_tx = SUBPX_TO_TILE(start_x + bounds->right);
        if (tile_col_test_range_y(COLLISION_LEFT, col_tx, tile_y_start, tile_y_end)) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, tile_y_start))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        return start_x;
    }
    col_tx = SUBPX_TO_TILE(start_x + bounds->left);
    if (tile_col_test_range_y(COLLISION_RIGHT, col_tx, tile_y_start, tile_y_end)) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, tile_y_start))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1) - bounds->left;
    }
    return start_x;
}
#endif

#if defined(DYNAMIC_ACTOR_ENABLE_MOVE_X) && defined(DYNAMIC_ACTOR_ENABLE_LEDGE_STOP)
static UWORD check_pit(UWORD start_x, UWORD start_y, rect16_t *bounds, UBYTE right) {
    UBYTE tile_y_start = SUBPX_TO_TILE(start_y + bounds->bottom);
    UBYTE tile_y_end = SUBPX_TO_TILE(start_y + bounds->top);
    if (right) {
        col_tx = SUBPX_TO_TILE(start_x + bounds->right);
        if (tile_col_test_range_y(COLLISION_LEFT, col_tx, tile_y_start, tile_y_end)) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx - 1, tile_y_start))){
                return start_x;
            }
#endif
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        if (!(tile_at(col_tx, tile_y_start + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
            return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
        }
        return start_x;
    }
    col_tx = SUBPX_TO_TILE(start_x + bounds->left);
    if (tile_col_test_range_y(COLLISION_RIGHT, col_tx, tile_y_start, tile_y_end)) {
#ifdef DYNAMIC_ACTOR_ENABLE_SLOPE_COLLISION
            if (IS_ON_SLOPE(tile_at(col_tx + 1, tile_y_start))){
                return start_x;
            }
#endif
        return TILE_TO_SUBPX(col_tx + 1) - bounds->left;
    }
    if (!(tile_at(col_tx, tile_y_start + 1) & (COLLISION_TOP | COLLISION_SLOPE_ANY))) {
        return TILE_TO_SUBPX(col_tx + 1)  - bounds->left;
    }
    return start_x;
}
#endif

#endif

#define CHECK_COL_H(x, y, actor, right) check_horizontal_collision((x), (y), &(actor)->bounds, (right))
#define CHECK_COL_V(x, y, actor, down)  check_vertical_collision((x), (y), &(actor)->bounds, (down))
#define CHECK_COL_PIT(x, y, actor, right) check_pit((x), (y), &(actor)->bounds, (right))

#endif

void dynamic_actor_update(void) BANKED {
    actor_t *actor = actors_active_tail;
    while (actor) {
        UBYTE behavior_id = actor->actor_behavior_id;
        UBYTE state = actor->actor_state;
        if ((behavior_id == 0) || (state == BHV_STATE_PAUSED)) {
            actor = actor->prev;
            continue;
        }
        behavior_def_t *def = &behavior_defs[behavior_id];
        UBYTE flags = def->flags;

#ifdef DYNAMIC_ACTOR_ENABLE_LINKED
        if (flags & BHV_LINKED) {
            actor_t *linked_actor = actors + actor->actor_linked_actor_idx;
            actor->pos.x = linked_actor->pos.x + actor->actor_vel_x;
            actor->pos.y = linked_actor->pos.y + actor->actor_vel_y;
            actor = actor->prev;
            continue;
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
        if (flags & BHV_GRAVITY) {
            actor->actor_vel_y += def->gravity;
            if (actor->actor_vel_y > def->max_fall_vel) {
                actor->actor_vel_y = def->max_fall_vel;
            }
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
        if (flags & BHV_MOVE_X) {
            new_actor_x = actor->pos.x + actor->actor_vel_x;
            UBYTE moving_right = (actor->pos.x < (UWORD)new_actor_x);
#ifdef DYNAMIC_ACTOR_ENABLE_LEDGE_STOP
            if ((flags & BHV_LEDGE_STOP) && (state == BHV_STATE_GROUNDED)) {
                actor->pos.x = CHECK_COL_PIT(new_actor_x, actor->pos.y, actor, moving_right);
            } else {
                actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, moving_right);
            }
#else
            actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, moving_right);
#endif
            if (actor->pos.x != (UWORD)new_actor_x) {
#ifdef DYNAMIC_ACTOR_ENABLE_REFLECT_X
                if (flags & BHV_REFLECT_X) {
                    actor->actor_vel_x = -actor->actor_vel_x;
                } else {
                    actor->actor_vel_x = 0;
                }
#else
                actor->actor_vel_x = 0;
#endif
            }
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
        if (flags & BHV_MOVE_Y) {
            new_actor_y = actor->pos.y + actor->actor_vel_y;
            UBYTE moving_down = (actor->pos.y <= (UWORD)new_actor_y);
            actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, moving_down);
            if (actor->pos.y != (UWORD)new_actor_y) {
                // Hit floor (moving down) or ceiling (moving up)
#ifdef DYNAMIC_ACTOR_ENABLE_BOUNCE
                if (flags & BHV_REFLECT_Y) {
                    if (def->bounce == 255) {
                        actor->actor_vel_y = -actor->actor_vel_y;
                    } else {
                        actor->actor_vel_y = -(WORD)(((int16_t)actor->actor_vel_y * def->bounce) >> 8);
                    }
                    // Kill micro-bounces caused by gravity pumping while resting
                    if (moving_down && (-actor->actor_vel_y <= def->gravity)) {
                        actor->actor_vel_y = 0;
                    }
                } else {
                    actor->actor_vel_y = 0;
                }
#else
                actor->actor_vel_y = 0;
#endif
                if (moving_down && (actor->actor_vel_y == 0)) {
#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
                    if (flags & BHV_GRAVITY){
                        //apply force to stick on ground to prevent bliping between grounded and airborne states on slopes
                        actor->actor_vel_y = 64;
                    }
#endif
                    state = BHV_STATE_GROUNDED;
                }
            }
#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
            else if (flags & BHV_GRAVITY) {
                state = BHV_STATE_AIRBORNE;
            }
#endif
            actor->actor_state = state;
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_ANIMATION
        UBYTE flags2 = def->flags2;
        if (flags2) {
            if (actor->actor_vel_x < 0) {
                if (flags2 & BHV2_ANIM_FACE) actor_set_dir(actor, DIR_LEFT, TRUE);
            } else if (actor->actor_vel_x > 0) {
                if (flags2 & BHV2_ANIM_FACE) actor_set_dir(actor, DIR_RIGHT, TRUE);
            } else if (flags2 & BHV2_ANIM_IDLE) {
                actor_set_anim_idle(actor);
            }
            if ((flags2 & BHV2_ANIM_JUMP) && (state == BHV_STATE_AIRBORNE)) {
                if (actor->dir == DIR_LEFT) {
                    actor_set_anim(actor, ANIM_JUMP_LEFT);
                } else {
                    actor_set_anim(actor, ANIM_JUMP_RIGHT);
                }
            }
        }
#endif

        actor = actor->prev;
    }
}

void vm_define_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    UBYTE slot = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    if ((slot == 0) || (slot > DYNAMIC_ACTOR_MAX_BEHAVIORS)) return;
    behavior_def_t *def = &behavior_defs[slot];
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
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor->actor_behavior_id;
}

void vm_set_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_state = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
}

void vm_get_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor->actor_state;
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
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor->actor_vel_x;
}

void vm_set_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
}

void vm_get_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor->actor_vel_y;
}

void vm_set_actor_linked_actor_idx(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    actor->actor_linked_actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor->actor_vel_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG2);
    actor->actor_vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG3);
}

void vm_get_actor_linked_actor_idx(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor->actor_linked_actor_idx;
}
