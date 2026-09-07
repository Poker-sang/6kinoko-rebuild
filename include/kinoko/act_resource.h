#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ECX is the resource, EDX is ignored; explicit arguments use the original
   callee-cleaned stack layout. */
int32_t __fastcall kinoko_act_set_current_time(int32_t resource, void *unused, int32_t time);
int32_t __fastcall kinoko_act_increment_frame(int32_t resource, void *unused);
int32_t __fastcall kinoko_act_get_current_time(int32_t resource, void *unused);
int32_t __fastcall kinoko_act_get_current_frame(int32_t resource, void *unused);
int32_t __fastcall kinoko_act_end_stage(int32_t resource, void *unused);
int32_t __fastcall kinoko_act_sleep(int32_t resource, void *unused, int32_t milliseconds);
int32_t __fastcall kinoko_act_sleep_to(int32_t resource, void *unused, int32_t milliseconds);
int32_t __fastcall kinoko_act_suspend(int32_t resource, void *unused);
int32_t __fastcall kinoko_act_resume(int32_t resource, void *unused);

#ifdef __cplusplus
}
#endif
