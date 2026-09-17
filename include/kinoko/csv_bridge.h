#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_read_csv(int32_t vm, int32_t window, const char *path,
    int32_t object[3], int32_t encoded);
/* 0 = success, 1 = non-table, 2 = mismatched definition rows. */
int32_t kinoko_csv_populate(int32_t vm, const char *text, const int32_t table[2]);
#ifdef __cplusplus
}
#endif
