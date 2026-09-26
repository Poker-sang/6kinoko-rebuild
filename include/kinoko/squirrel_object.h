#include "kinoko/squirrel_native_types.h"
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Original 403E50 scalar-deleting destructor: ECX holds the wrapper and the
   flags occupy one stack argument. EDX is unused, matching __thiscall. */
void* __fastcall kinoko_squirrel_object_delete(void* object, void *unused,
                                                int32_t flags);
int32_t kinoko_squirrel_object_size(void* object, SQVM* vm);
int32_t kinoko_squirrel_object_reverse(void* object, SQVM* vm);
void* kinoko_squirrel_object_destroy(void* object, SQVM* vm, const void* vtable);
int32_t kinoko_native_instance_create(SQVM* vm, const char* class_name,
    void* native_pointer, SQRELEASEHOOK release_hook);
#ifdef __cplusplus
}
#endif
