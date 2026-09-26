#include "kinoko/file_io_legacy.h"
#pragma once
#include "kinoko/legacy_string.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct KinokoAudioHostSymbols {
    const void* critical_section_vtable;
    const char* device_error_message;
};
const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void);
extern int32_t g637;
extern char kinoko_packed_assets;
extern int32_t g876;
extern char* g877;
extern int32_t g878;
void retdec_trace(const char* message);
void retdec_trace_i32(const char* label, int32_t value);
void retdec_trace_hresult(const char* label, long value);
void retdec_trace_squirrel_name(const char* label, int32_t name);
#ifdef __cplusplus
}
#endif
