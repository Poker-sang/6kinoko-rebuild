#pragma once
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int kinoko_application_run(HINSTANCE instance, int show_command);
void kinoko_application_construct(void);
void kinoko_application_shutdown(void);
int32_t kinoko_application_frame_count(void);
/* Narrow ports to the not-yet-migrated boot script and scene implementations. */
void kinoko_application_initialize_host(void);
void kinoko_application_open_archives(void);
void kinoko_application_set_archive_mode(int32_t enabled);
const char *kinoko_application_title(void);
const char *kinoko_application_error(void);
#ifdef __cplusplus
}
#endif
