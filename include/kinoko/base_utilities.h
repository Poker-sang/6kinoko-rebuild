#pragma once
#include <stdint.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One zlib 1.2.3 operation, returning bytes written or zero. */
int32_t kinoko_compress_buffer(const void* input, int32_t input_size,
    void* output, int32_t output_capacity);
int32_t kinoko_decompress_buffer(const void* input, int32_t input_size,
    void* output, int32_t output_capacity);

typedef struct KinokoProcessContext {
    HINSTANCE instance;
    HWND window;
    char executable_directory[MAX_PATH];
} KinokoProcessContext;
const KinokoProcessContext* kinoko_process_context(void);
int32_t kinoko_process_initialize(HINSTANCE instance, HWND window);
/* Both output buffers, when present, must hold MAX_PATH bytes. File is optional.
   Returns the CRT error code, zero on success; retains a trailing separator. */
int32_t kinoko_path_split(const char* path, char* directory, char* file);
#ifdef __cplusplus
}
#endif
