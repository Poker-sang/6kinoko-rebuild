#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct KinokoAudioHostSymbols {
    const void* critical_section_vtable;
    const void* handle_table_vtable;
    const void* decoder_vtable;
    const char* device_error_message;
};
const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void);
extern int32_t g637;
extern int32_t g765;
extern char g874;
extern int32_t g876;
extern char* g877;
extern int32_t g878;
int32_t function_407370(int32_t slot_address, const char* path);
int32_t function_407300(int32_t reader);
int32_t retdec_reader_read_exact(int32_t reader, void* buffer, uint32_t size);
void retdec_destroy_reader(int32_t* reader);
uint32_t retdec_safe_c_string_length(const char* source);
int32_t retdec_string_assign_n(int32_t* object, const char* source, uint32_t size);
void retdec_trace(const char* message);
void retdec_trace_i32(const char* label, int32_t value);
void retdec_trace_hresult(const char* label, long value);
void retdec_trace_squirrel_name(const char* label, int32_t name);
#ifdef __cplusplus
}
#endif
