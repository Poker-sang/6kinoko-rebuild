#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_map_update(int32_t layout, int32_t left, int32_t top,
                         int32_t right, int32_t bottom);
int32_t kinoko_map_draw(int32_t layout, float x, float y);
int32_t __fastcall kinoko_map_entry_434b40(int32_t layout, void *unused);
int32_t __fastcall kinoko_map_entry_434b60(int32_t layout, void *unused,
    int32_t left, int32_t top, int32_t right, int32_t bottom);
int32_t __fastcall kinoko_map_entry_434f40(int32_t layout, void *unused,
    float x, float y);
int32_t __fastcall kinoko_map_entry_46eed0(int32_t layer, void *unused,
    int32_t camera);
#ifdef __cplusplus
}
#endif
