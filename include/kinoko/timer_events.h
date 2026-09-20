#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_initialize_timer_events(void);
void kinoko_notify_timer_events(void);
int32_t kinoko_timer_events_identity(void);
int32_t kinoko_timer_events_first(void);
#ifdef __cplusplus
}
#endif
