#pragma once
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
/* SqPlus native ReadCSV adapter: path and by-value object words. */
int32_t kinoko_script_read_csv(const char *path, const void* vtable, int32_t type, int32_t data);
int32_t kinoko_read_csv(struct SQVM* vm, void* window, const char *path,
    int32_t object[3], int32_t encoded);
/* 0 = success, 1 = non-table, 2 = mismatched definition rows. */
int32_t kinoko_csv_populate(struct SQVM* vm, const char *text, const int32_t table[2]);
#ifdef __cplusplus
}
#endif
