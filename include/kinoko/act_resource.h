#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t function_44fde0(int32_t storage, int32_t source_holder);
int32_t function_450020(int32_t resource);
void retdec_destroy_act_runtime(int32_t resource);
int32_t kinoko_act_find_first(int32_t runtime, const char* pattern);
int32_t kinoko_act_find_next(int32_t runtime, int32_t id);
int32_t kinoko_act_find_close(int32_t runtime, int32_t id);
const char* kinoko_act_find_name(int32_t runtime, int32_t id);

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
