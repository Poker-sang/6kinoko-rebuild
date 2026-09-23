#pragma once
#include <stdint.h>
typedef struct KinokoIntegerMap KinokoIntegerMap;
typedef struct KinokoIntegerMapIndex {
    uint32_t policy;
    KinokoIntegerMap* owner;
    int32_t count;
} KinokoIntegerMapIndex;
#ifdef __cplusplus
extern "C" {
#endif
KinokoIntegerMap* kinoko_integer_map_create(void);
void kinoko_integer_map_destroy(KinokoIntegerMap* map);
void kinoko_integer_map_clear(KinokoIntegerMap* map);
uint32_t kinoko_integer_map_size(const KinokoIntegerMap* map);
/* Values are integer slots. Returned pointers borrow nodes and survive insert,
   but not clear/destroy. Missing native lookup returns NULL. */
int32_t* kinoko_integer_map_put(KinokoIntegerMap* map,int32_t key,int32_t value);
int32_t* kinoko_integer_map_find(KinokoIntegerMap* map,int32_t key);
/* Explicit legacy iterator boundary: writes the old owner sentinel on miss. */
int32_t* kinoko_integer_map_lookup_index(const KinokoIntegerMapIndex* index,int32_t* entry,const int32_t* key);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoIntegerMapIndex)==12);
#endif
