#pragma once
#include <stdint.h>
typedef struct KinokoActor KinokoActor;
#include "kinoko/native_control.h"
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error The recovered Actor ABI requires MSVC Win32.
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Trivial externally owned SqPlus argument: the callee consumes its reference.
   Never replace this with SQObjectPtr or a nontrivial by-value C++ class. */
typedef struct KinokoOwnedObjectWords {
    int32_t vtable;
    int32_t type;
    int32_t value;
} KinokoOwnedObjectWords;
KinokoActor *kinoko_actor_construct(KinokoActor *actor);
KinokoActor *kinoko_actor_dispose(KinokoActor *actor);
int32_t kinoko_actor_set_step_owned(KinokoActor *actor, KinokoOwnedObjectWords *object);
int32_t kinoko_actor_reset(KinokoActor *actor);
KinokoActor *kinoko_actor_assign(KinokoActor *destination, KinokoActor *source);
/* Explicit SqPlus copy callback: integer signature belongs to the type registry. */
int32_t kinoko_actor_assign_instance(int32_t destination, int32_t source);
/* ECX receives this; EDX is unused; stack arguments are callee-popped. */
KinokoActor *__fastcall kinoko_actor_dispose_method(KinokoActor *actor, void* unused_edx);
KinokoActor *__fastcall kinoko_actor_delete_method(KinokoActor *actor, void* unused_edx, unsigned char flags);
int32_t __fastcall kinoko_actor_set_step_method(KinokoActor *actor, void* unused_edx, KinokoOwnedObjectWords object);
/* Narrow legacy-data accessors avoid conflicting external declarations. */
int32_t kinoko_actor_vtable(void);
int32_t kinoko_actor_step_key(void);
#ifdef __cplusplus
}
#endif
