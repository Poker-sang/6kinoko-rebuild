#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct KinokoInputKeyStorage KinokoInputKeyStorage;
/* Original 1044-byte tracker; the old vector slot owns a native key list. */
typedef struct KinokoKeyTracker {
    int32_t counts[256];
    KinokoInputKeyStorage *keys;
    uint8_t reserved1028[12];
    uint8_t shift, alt, control, reserved1043;
} KinokoKeyTracker;
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_keys_construct(KinokoKeyTracker *tracker);
void kinoko_input_keys_destroy(KinokoKeyTracker *tracker);
void kinoko_input_keys_clear(KinokoKeyTracker *tracker);
void kinoko_input_keys_add(KinokoKeyTracker *tracker, uint8_t scan);
void kinoko_input_keys_assign(KinokoKeyTracker *destination, const KinokoKeyTracker *source);
uint32_t kinoko_input_keys_size(const KinokoKeyTracker *tracker);
uint8_t kinoko_input_keys_at(const KinokoKeyTracker *tracker, uint32_t index);
int32_t kinoko_input_keys_update(KinokoKeyTracker *tracker);
int32_t kinoko_input_key_pressed(const KinokoKeyTracker *tracker,
    int32_t scan, int32_t shift, int32_t alt, int32_t control);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoKeyTracker)==1044);
static_assert(offsetof(KinokoKeyTracker,keys)==1024);
static_assert(offsetof(KinokoKeyTracker,shift)==1040);
static_assert(offsetof(KinokoKeyTracker,alt)==1041);
static_assert(offsetof(KinokoKeyTracker,control)==1042);
#endif
