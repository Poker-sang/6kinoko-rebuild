#pragma once
#include <stdint.h>
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
int32_t function_45e300_this(int32_t actor);
int32_t function_45e460_this(int32_t actor);
int32_t function_4606d0_this(int32_t actor, int32_t owned_object);
void retdec_actor_release_weak(int32_t control);
void retdec_release_squirrel_object(int32_t control);
/* ECX receives this; EDX is unused; stack arguments are callee-popped. */
int32_t __fastcall function_45e460(int32_t actor, void* unused_edx);
int32_t __fastcall function_45f0c0(int32_t actor, void* unused_edx, unsigned char flags);
int32_t __fastcall function_4606d0(int32_t actor, void* unused_edx, KinokoOwnedObjectWords object);
/* Narrow legacy-data accessors avoid conflicting external declarations. */
int32_t kinoko_actor_vtable(void);
int32_t kinoko_actor_control_vtable(void);
int32_t kinoko_actor_step_key(void);
#ifdef __cplusplus
}
#endif
