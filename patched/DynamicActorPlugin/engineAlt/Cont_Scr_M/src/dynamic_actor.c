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

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
// Previous-frame player position, used to mirror the engine-controlled
// player's movement into its velocity fields (see dynamic_actor_update).
static UWORD player_prev_x;
static UWORD player_prev_y;
#endif

void dynamic_actor_init(void) BANKED {
    memset(behavior_defs, 0, sizeof(behavior_defs));
#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
    player_prev_x = PLAYER.pos.x;
    player_prev_y = PLAYER.pos.y;
#endif
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

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
static UBYTE actor_intersects_platform(actor_t *actor, actor_t *platform) {
#if DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_SINGLE_POINT
    return bb_contains(&platform->bounds, &platform->pos, &actor->pos);
#elif DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_TRIANGLE
    UWORD point_x = actor->pos.x + actor->bounds.left + ((actor->bounds.right - actor->bounds.left) >> 1);
    UWORD point_y = actor->pos.y + actor->bounds.bottom;
    UWORD left = platform->pos.x + platform->bounds.left;
    UWORD right = platform->pos.x + platform->bounds.right;
    UWORD top = platform->pos.y + platform->bounds.top;
    UWORD bottom = platform->pos.y + platform->bounds.bottom;
    return (point_x >= left) && (point_x <= right) && (point_y >= top) && (point_y <= bottom);
#else
    return bb_intersects(&actor->bounds, &actor->pos, &platform->bounds, &platform->pos);
#endif
}
#endif

void dynamic_actor_update(void) BANKED {

    actor_t *actor = actors_active_tail;
    while (actor) {
        UBYTE behavior_id = actor->actor_behavior_id;
        UBYTE state = actor->actor_state;
        behavior_def_t *def = &behavior_defs[behavior_id];
        UBYTE flags = def->flags;
#if defined(DYNAMIC_ACTOR_ENABLE_MOVE_X) || defined(DYNAMIC_ACTOR_ENABLE_MOVE_Y) || defined(DYNAMIC_ACTOR_ENABLE_ANIMATION) || defined(DYNAMIC_ACTOR_ENABLE_PARENT) || defined(DYNAMIC_ACTOR_ENABLE_ACTOR_COLLISION)
        UBYTE flags2 = def->flags2;
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
        // Parenting is not a behavior: every actor with a defined parent
        // inherits the parent actor's per-frame movement (tile-collision
        // checked, like riding a moving platform), then still runs its own
        // behavior physics below if it has any. Runs even for actors with no
        // behavior assigned (slot 0 is zeroed, so tile collision stays on) or
        // a paused one. Set the parent explicitly with the Set Actor Parent
        // Actor events, or automatically via a BHV_PLATFORM actor.
        if (actor->actor_parent) {
            actor_t *parent_actor = actor->actor_parent;
            // The displacement is tile-collision checked: a parented actor
            // normally only checks collision when it moves itself, so
            // without this the parent actor's movement could push this
            // actor through walls. Direction comes from the parent actor's
            // velocity. Disabled by the behavior's 'no tile collision' option.
            // The parent's velocity is its movement this frame; the engine-
            // controlled player doesn't set velocity, so its live position
            // delta is added when the player is the parent.
            WORD parent_actor_delta_x = parent_actor->actor_vel_x;
            WORD parent_actor_delta_y = parent_actor->actor_vel_y;
            if (parent_actor == &PLAYER) {
                parent_actor_delta_x += (WORD)(PLAYER.pos.x - player_prev_x);
                parent_actor_delta_y += (WORD)(PLAYER.pos.y - player_prev_y);
            }
            if (parent_actor_delta_x) {
                new_actor_x = actor->pos.x + parent_actor_delta_x;
#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
                if (flags2 & BHV2_NO_TILE_COLLISION) {
                    actor->pos.x = new_actor_x;
                } else {
                    actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, (parent_actor_delta_x > 0));
                }
#else
                actor->pos.x = new_actor_x;
#endif
            }
            if (parent_actor_delta_y) {
                new_actor_y = actor->pos.y + parent_actor_delta_y;
#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
                if (flags2 & BHV2_NO_TILE_COLLISION) {
                    actor->pos.y = new_actor_y;
                } else {
                    actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, (parent_actor_delta_y > 0));
                }
#else
                actor->pos.y = new_actor_y;
#endif
            }
        }
#endif

        if ((behavior_id == 0) || (state == BHV_STATE_PAUSED)) {
            actor = actor->prev;
            continue;
        }

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
        // Moving platform: claim every intersecting actor as a child (it then
        // inherits this platform's movement via the parenting above), unless
        // that actor already has a different parent, or this platform has a
        // collision group and the actor's group differs (a group-less platform
        // carries everything, including the player). Release actors that are
        // no longer intersecting.
        if (flags & BHV_PLATFORM) {
            actor_t *platform_actor = actor;
            UBYTE platform_group = actor->collision_group & COLLISION_GROUP_MASK;
            actor_t *other = actors_active_tail;
            while (other) {
                if (other != actor) {
                    if (actor_intersects_platform(other, actor)) {
                        if (!other->actor_parent) {
                            other->actor_parent = platform_actor;
                        }
                    } else if (other->actor_parent == platform_actor) {
                        other->actor_parent = NULL;
                    }
                }
                other = other->prev;
            }
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_ACTOR_COLLISION
        // Position before this frame's own movement (after any parent carry),
        // restored when the movement runs the actor into another actor.
        UWORD prev_x = actor->pos.x;
        UWORD prev_y = actor->pos.y;
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
            if (flags2 & BHV2_NO_TILE_COLLISION) {
                // Tile collision disabled: apply velocity directly
                actor->pos.x = new_actor_x;
            } else {
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
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
        if (flags & BHV_MOVE_Y) {
            new_actor_y = actor->pos.y + actor->actor_vel_y;
            if (flags2 & BHV2_NO_TILE_COLLISION) {
                // Tile collision disabled: apply velocity directly, never land
                actor->pos.y = new_actor_y;
#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
                if (flags & BHV_GRAVITY) {
                    state = BHV_STATE_AIRBORNE;
                }
#endif
                actor->actor_state = state;
            } else {
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
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_ACTOR_COLLISION
        // Actor-vs-actor collision (the engine already handles the player):
        // if this frame's movement ran into another collidable actor, restore
        // the pre-move position and turn/bounce per the reflect settings.
        if (flags2 & BHV2_ACTOR_COLLISION) {
            actor_t *other = actors_active_tail;
            while (other) {
                if ((other != actor) && (other != actors) &&
                    (other->flags & ACTOR_FLAG_COLLISION) &&
                    bb_intersects(&actor->bounds, &actor->pos, &other->bounds, &other->pos)) {
                    actor->pos.x = prev_x;
                    actor->pos.y = prev_y;
#ifdef DYNAMIC_ACTOR_ENABLE_REFLECT_X
                    if (flags & BHV_REFLECT_X) {
                        actor->actor_vel_x = -actor->actor_vel_x;
                    } else {
                        actor->actor_vel_x = 0;
                    }
#else
                    actor->actor_vel_x = 0;
#endif
#ifdef DYNAMIC_ACTOR_ENABLE_BOUNCE
                    if (flags & BHV_REFLECT_Y) {
                        actor->actor_vel_y = -actor->actor_vel_y;
                    } else
#endif
                    {
#ifdef DYNAMIC_ACTOR_ENABLE_GRAVITY
                        // Leave vertical velocity to gravity for side-view actors
                        if (!(flags & BHV_GRAVITY))
#endif
                        {
                            actor->actor_vel_y = 0;
                        }
                    }
                    break;
                }
                other = other->prev;
            }
        }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_ANIMATION
        if (flags2) {
            if (flags2 & BHV2_ANIM_FACE_4DIR) {
                // Face the dominant velocity axis (top down / adventure)
                WORD abs_vx = actor->actor_vel_x;
                if (abs_vx < 0) abs_vx = -abs_vx;
                WORD abs_vy = actor->actor_vel_y;
                if (abs_vy < 0) abs_vy = -abs_vy;
                if (abs_vx || abs_vy) {
                    if (abs_vy > abs_vx) {
                        actor_set_dir(actor, (actor->actor_vel_y < 0) ? DIR_UP : DIR_DOWN, TRUE);
                    } else {
                        actor_set_dir(actor, (actor->actor_vel_x < 0) ? DIR_LEFT : DIR_RIGHT, TRUE);
                    }
                } else if (flags2 & BHV2_ANIM_IDLE) {
                    actor_set_anim_idle(actor);
                }
            } else if (actor->actor_vel_x < 0) {
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

#ifdef DYNAMIC_ACTOR_ENABLE_PARENT
    // The scene-type movement code (platform, top down, adventure...) moves
    // the player directly without touching the plugin's velocity fields, so
    // actors parented to the player would see zero velocity and never follow.
    // Mirror the player's position change since last frame into its velocity
    // fields here.
    player_prev_x = PLAYER.pos.x;
    player_prev_y = PLAYER.pos.y;
#endif
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

void vm_set_actor_parent(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;
    actor_t * actor = actors + *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE parent_actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
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
    // Check for actor collision at the given pixel coordinates
    // And return the index of the first colliding actor, or -1 if none    
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

#define WAIT_COL_H     0x01u
#define WAIT_COL_V     0x02u
#define WAIT_COL_PIT   0x04u
#define WAIT_COL_ACTOR 0x08u

UBYTE vm_wait_for_collision(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t* actor = actors + stack_frame[0];
    if (start){
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
    } else {
        // Interrupt actor movement
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }
    UBYTE collision_flag = stack_frame[1]; //Horizontal or vertical or checkpit or another actor collision

    if (!collision_flag) {
        return TRUE;
    }

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
    if ((collision_flag & WAIT_COL_H) && actor->actor_vel_x) {
        new_actor_x = actor->pos.x + actor->actor_vel_x;
        if (CHECK_COL_H(new_actor_x, actor->pos.y, actor, (actor->actor_vel_x > 0)) != (UWORD)new_actor_x) {
            return TRUE;
        }
    }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
    if ((collision_flag & WAIT_COL_V) && actor->actor_vel_y) {
        new_actor_y = actor->pos.y + actor->actor_vel_y;
        if (CHECK_COL_V(actor->pos.x, new_actor_y, actor, (actor->actor_vel_y > 0)) != (UWORD)new_actor_y) {
            return TRUE;
        }
    }
#endif

#if defined(DYNAMIC_ACTOR_ENABLE_MOVE_X) && defined(DYNAMIC_ACTOR_ENABLE_LEDGE_STOP)
    if ((collision_flag & WAIT_COL_PIT) && actor->actor_vel_x) {
        new_actor_x = actor->pos.x + actor->actor_vel_x;
        if (CHECK_COL_PIT(new_actor_x, actor->pos.y, actor, (actor->actor_vel_x > 0)) != (UWORD)new_actor_x) {
            return TRUE;
        }
    }
#endif

#ifdef DYNAMIC_ACTOR_ENABLE_ACTOR_COLLISION
    if (collision_flag & WAIT_COL_ACTOR) {
        UWORD test_x = actor->pos.x;
        UWORD test_y = actor->pos.y;
        upoint16_t test_pos;
        actor_t *other = actors_active_tail;

#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_X
        test_x += actor->actor_vel_x;
#endif
#ifdef DYNAMIC_ACTOR_ENABLE_MOVE_Y
        test_y += actor->actor_vel_y;
#endif
    test_pos.x = test_x;
    test_pos.y = test_y;

        while (other) {
            if ((other != actor) &&
                (other->flags & ACTOR_FLAG_COLLISION) &&
        bb_intersects(&actor->bounds, &test_pos, &other->bounds, &other->pos)) {
                return TRUE;
            }
            other = other->prev;
        }
    }
#endif

    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}

// Waitable chase/flee steering: each frame, steers 'actor' toward (flee = 0)
// or away from (flee = 1) the target actor at 'speed' subpixels/frame, then
// yields. The actor needs a behavior with Move X/Y so the velocity gets
// applied; while the actor's behavior has gravity only the x axis is steered
// (ground pursuer), otherwise both axes (top down / flying). Speed doubles as
// the steering dead zone so the chaser doesn't oscillate on the target.
// stop_range (subpixels): a chase completes when the actor is within range of
// the target on all steered axes; a flee completes when it is beyond range on
// any steered axis; 0 = never completes (permanent behavior - put the event
// in an actor's update script or a looping thread).
// interval mask: power-of-two refresh period minus one for cached target
// position refresh from the target actor.
// stack_frame: [0] actor idx, [1] target idx, [2] flee,
//              [3] stop_range, [4] interval, [5] cached_target_x,
//              [6] cached_target_y
UBYTE vm_actor_chase_actor(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t *actor = actors + (UBYTE)stack_frame[0];
    if (start){
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
    } else {
        // Interrupt actor movement
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }
    actor_t *target = actors + (UBYTE)stack_frame[1];
    UBYTE flee = (UBYTE)stack_frame[2];
    UWORD range = stack_frame[3];
    UBYTE interval = (UBYTE)stack_frame[4];
    WORD speed = actor->move_speed >> 1; // player max velocity is 128, so divide by 2 to get a speed that lands on cell boundaries;
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

// Waitable move-to-point steering: each frame, steers 'actor' toward the
// target position at 'speed' subpixels/frame, then yields. The actor needs a
// behavior with Move X/Y so the velocity is applied; while the actor's
// behavior has gravity only the x axis is steered (ground mover), otherwise
// both axes (top down / flying). Speed doubles as the steering dead zone so
// the actor doesn't oscillate on the destination.
// target_x/target_y are in pixels; stop_range is in pixels (0 = exact match).
// stack_frame: [0] actor idx, [1] target_x_px, [2] target_y_px,
//              [3] stop_range_px
UBYTE vm_actor_move_to_pos_by_velocity(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
        
    actor_t *actor = actors + (UBYTE)stack_frame[0];
    if (start) {
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
    } else {
        // Interrupt actor movement
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }

    UWORD target_x = PX_TO_SUBPX(stack_frame[1]);
    UWORD target_y = PX_TO_SUBPX(stack_frame[2]);
    UWORD range = PX_TO_SUBPX(stack_frame[3]);
    WORD speed = actor->move_speed >> 1; // player max velocity is 128, so divide by 2 to get a speed that lands on cell boundaries;
    UBYTE steer_y = 1;

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

    if ((adx <= range) && (!steer_y || (ady <= range))) {
        actor->actor_vel_x = 0;
        if (steer_y) {
            actor->actor_vel_y = 0;
        }
        return TRUE;
    }
    

    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}

#define CRAWL_SOLID(tx, ty) ((tile_at((tx), (ty)) & COLLISION_ALL) == COLLISION_ALL)

#define DIR_XMOD(value, dir) (((dir) & 1) ? ((dir) == 1 ? (value) : -(value)) : 0)
#define DIR_YMOD(value, dir) (((dir) & 1) ? 0 : ((dir) == 0 ? -(value) : (value)))

#define DIR_BOUNDS_X(bounds, dir) (((dir) & 1) ? ((dir) == 1 ? (bounds).right : (bounds).left) : 0)
#define DIR_BOUNDS_Y(bounds, dir) (((dir) & 1) ? 0 : ((dir) == 0 ? (bounds).top : (bounds).bottom))


// One step of wall/ceiling crawling (right/left-hand wall follower).
// Call every frame from a looping script; the current direction lives in a
// script local owned by the caller, so every crawler keeps its own state and
// no per-actor engine RAM is needed. The behavior applies the velocity, so
// the actor needs Move X + Move Y (tile collision is not needed - the crawl
// logic already does the tile collision correction of the behavior). 
// A wall is a fully solid tile (all
// four collision bits); out-of-bounds reads count as solid, so map borders
// can be crawled. Stack frame slots [3] and [4] cache the last tile X/Y that
// was processed so the collision test only runs when the actor enters a new
// tile instead of depending on exact grid alignment.
UBYTE vm_actor_crawl_step(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
    actor_t * actor = actors + (UBYTE)stack_frame[0];
    if (start){
        CLR_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT);
    } else {
        // Interrupt actor movement
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_INTERRUPT)) {
            return TRUE;
        }
    }
    UBYTE dir = ((UBYTE)stack_frame[1]) & 3;
    UBYTE side = (UBYTE)stack_frame[2];   // 0 = wall on right hand (clockwise around blocks), 1 = left hand
    UBYTE speed = actor->move_speed >> 1; // player max velocity is 128, so divide by 2 to get a speed that lands on cell boundaries
#if DYNAMIC_ACTOR_COLLISION_TYPE == DYNAMIC_ACTOR_COLLISION_SINGLE_POINT
    UBYTE tile_x = SUBPX_TO_TILE(actor->pos.x + DIR_XMOD(speed, dir));
    UBYTE tile_y = SUBPX_TO_TILE(actor->pos.y + DIR_YMOD(speed, dir));
#else
    UBYTE tile_x = SUBPX_TO_TILE(actor->pos.x + DIR_BOUNDS_X(actor->bounds, dir) + DIR_XMOD(speed, dir));
    UBYTE tile_y = SUBPX_TO_TILE(actor->pos.y + DIR_BOUNDS_Y(actor->bounds, dir) + DIR_YMOD(speed, dir));
#endif
    if (start) {
        stack_frame[3] = tile_x;
        stack_frame[4] = tile_y;
    } else if ((tile_x != (UBYTE)stack_frame[3]) || (tile_y != (UBYTE)stack_frame[4])) {
#if DYNAMIC_ACTOR_COLLISION_TYPE != DYNAMIC_ACTOR_COLLISION_SINGLE_POINT
        if (dir & 1){
            UBYTE sdir = (dir + (side ? 3 : 1)) & 3; 
            UWORD new_actor_y = actor->pos.y + DIR_YMOD(speed, sdir);
            actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, sdir == 2);
            if (actor->pos.y == new_actor_y) {
                // Outer corner: the wall beside us ended - turn toward it to wrap around
                dir = sdir;
                //Adjust horizontal overshoot
                sdir = (dir + (side ? 3 : 1)) & 3;
                UWORD new_actor_x = actor->pos.x + DIR_XMOD(speed, sdir);
                actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, sdir == 1);

            } else {
                UWORD new_actor_x = actor->pos.x + DIR_XMOD(speed, dir);
                actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, dir == 1);
                if (new_actor_x != actor->pos.x) {
                    // Ran into a wall: turn away from the wall side
                    dir = (dir + (side ? 1 : 3)) & 3;
                }            
            }
        } else {
            UBYTE sdir = (dir + (side ? 3 : 1)) & 3; 
            UWORD new_actor_x = actor->pos.x + DIR_XMOD(speed, sdir);    
            actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, sdir == 1);   
            if (actor->pos.x == new_actor_x) {
                // Outer corner: the wall beside us ended - turn toward it to wrap around
                dir = sdir;
                //Adjust vertical overshoot
                sdir = (dir + (side ? 3 : 1)) & 3;
                UWORD new_actor_y = actor->pos.y + DIR_YMOD(speed, sdir);
                actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, sdir == 2);

            } else {
                UWORD new_actor_y = actor->pos.y + DIR_YMOD(speed, dir);
                actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, dir == 2);
                if (new_actor_y != actor->pos.y) {
                    // Ran into a wall: turn away from the wall side
                    dir = (dir + (side ? 1 : 3)) & 3;
                }
            }
        }
#else
        if (dir & 1){
            UWORD new_actor_x = actor->pos.x + DIR_XMOD(speed, dir);
            actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, dir == 1);
            if (new_actor_x != actor->pos.x) {
                // Ran into a wall: turn away from the wall side
                dir = (dir + (side ? 1 : 3)) & 3;
            } else {
                UBYTE sdir = (dir + (side ? 3 : 1)) & 3; 
                UWORD new_actor_y = actor->pos.y + DIR_YMOD(speed, sdir);
                actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, sdir == 2);
                if (actor->pos.y == new_actor_y) {
                    // Outer corner: the wall beside us ended - turn toward it to wrap around
                    dir = sdir;
                }
            }
        } else {
            UWORD new_actor_y = actor->pos.y + DIR_YMOD(speed, dir);
            actor->pos.y = CHECK_COL_V(actor->pos.x, new_actor_y, actor, dir == 2);
            if (new_actor_y != actor->pos.y) {
                // Ran into a wall: turn away from the wall side
                dir = (dir + (side ? 1 : 3)) & 3;
            } else {
                UBYTE sdir = (dir + (side ? 3 : 1)) & 3; 
                UWORD new_actor_x = actor->pos.x + DIR_XMOD(speed, sdir);    
                actor->pos.x = CHECK_COL_H(new_actor_x, actor->pos.y, actor, sdir == 1);   
                if (actor->pos.x == new_actor_x) {
                    // Outer corner: the wall beside us ended - turn toward it to wrap around
                    dir = sdir;
                }
            }
        }
#endif
        stack_frame[3] = tile_x;
        stack_frame[4] = tile_y;
    }

    switch (dir) {
        case 0:
            actor->actor_vel_x = 0;
            actor->actor_vel_y = -speed;
            break;
        case 1:
            actor->actor_vel_x = speed;
            actor->actor_vel_y = 0;
            break;
        case 2:
            actor->actor_vel_x = 0;
            actor->actor_vel_y = speed;
            break;
        default:
            actor->actor_vel_x = -speed;
            actor->actor_vel_y = 0;
            break;
    }
    stack_frame[1] = dir;
    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
    return FALSE;
}



