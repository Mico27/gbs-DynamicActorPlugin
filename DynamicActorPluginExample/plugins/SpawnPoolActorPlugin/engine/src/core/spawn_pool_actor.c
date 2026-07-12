#pragma bank 255

#include <gbdk/platform.h>
#include "vm.h"
#include "gbs_types.h"
#include "math.h"
#include "actor.h"
#include "macro.h"

void vm_spawn_pool_actor(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE pool_start = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE pool_count = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    UWORD x = PX_TO_SUBPX(*(int16_t *)VM_REF_TO_PTR(FN_ARG2));
    UWORD y = PX_TO_SUBPX(*(int16_t *)VM_REF_TO_PTR(FN_ARG3));
    int16_t idx = *(int16_t*)VM_REF_TO_PTR(FN_ARG4);
    int16_t * A;
    UBYTE spawned_idx = 255;
    actor_t *pool_actor = actors + pool_start;
    for (UBYTE i = pool_count; i != 0; i--, pool_actor++) {
        if (CHK_FLAG(pool_actor->flags, ACTOR_FLAG_ACTIVE)) continue;
        pool_actor->pos.x = x;
        pool_actor->pos.y = y;
        CLR_FLAG(pool_actor->flags, ACTOR_FLAG_DISABLED);
        activate_actor(pool_actor);
        spawned_idx = (UBYTE)(pool_actor - actors);
        break;
    }
    if (idx < 0) A = THIS->stack_ptr + idx - 5; else A = script_memory + idx;
    *A = spawned_idx;
}
