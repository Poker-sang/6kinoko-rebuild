#pragma once

#include <stdint.h>
#include "kinoko/act_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 44FDE0: initialize fresh caller-owned 192-byte Win32 runtime storage.
   The holder is borrowed; initialization neither clones nor loads a document.
   On C++ constructor failure the caller still owns the raw storage. */
KinokoActRuntime *kinoko_act_runtime_initialize(
    KinokoActRuntime *storage, KinokoActSourceHolder *source_holder);
/* Dispose members, but do not free the caller-owned runtime storage.
   Normal order follows 450020/4513F0. Null is accepted as by old 450020. */
void kinoko_act_runtime_dispose(KinokoActRuntime *runtime);
int32_t kinoko_act_find_first(KinokoActRuntime *runtime, const char* pattern);
int32_t kinoko_act_find_next(KinokoActRuntime *runtime, int32_t id);
int32_t kinoko_act_find_close(KinokoActRuntime *runtime, int32_t id);
const char* kinoko_act_find_name(KinokoActRuntime *runtime, int32_t id);

/* ECX is the resource, EDX is ignored; explicit arguments use the original
   callee-cleaned stack layout. */
int32_t __fastcall kinoko_act_set_current_time(KinokoActRuntime *resource, void *unused, int32_t time);
int32_t __fastcall kinoko_act_increment_frame(KinokoActRuntime *resource, void *unused);
int32_t __fastcall kinoko_act_get_current_time(KinokoActRuntime *resource, void *unused);
int32_t __fastcall kinoko_act_get_current_frame(KinokoActRuntime *resource, void *unused);
int32_t __fastcall kinoko_act_end_stage(KinokoActRuntime *resource, void *unused);
int32_t __fastcall kinoko_act_sleep(KinokoActRuntime *resource, void *unused, int32_t milliseconds);
int32_t __fastcall kinoko_act_sleep_to(KinokoActRuntime *resource, void *unused, int32_t milliseconds);
int32_t __fastcall kinoko_act_suspend(KinokoActRuntime *resource, void *unused);
int32_t __fastcall kinoko_act_resume(KinokoActRuntime *resource, void *unused);

#ifdef __cplusplus
}
#endif
