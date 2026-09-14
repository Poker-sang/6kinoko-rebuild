#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Original 403E50 scalar-deleting destructor: ECX holds the wrapper and the
   flags occupy one stack argument. EDX is unused, matching __thiscall. */
int32_t __fastcall kinoko_squirrel_object_delete(int32_t object, void *unused,
                                                int32_t flags);
int32_t kinoko_squirrel_object_size(int32_t object, int32_t vm);
int32_t kinoko_squirrel_object_clear(int32_t object, int32_t vm);
#ifdef __cplusplus
}
#endif
