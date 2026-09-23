#pragma once
#include "kinoko/input_manager.h"
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_devices_construct(KinokoInputManager *manager);
void kinoko_input_devices_destroy(KinokoInputManager *manager);
void kinoko_input_devices_resize(KinokoInputManager *manager, uint32_t count);
void kinoko_input_devices_assign(KinokoInputManager *destination, const KinokoInputManager *source);
uint32_t kinoko_input_devices_size(const KinokoInputManager *manager);
KinokoInputDevice *kinoko_input_devices_at(KinokoInputManager *manager, uint32_t index);
/* Borrowed Win32 contiguous record view. Invalidation follows vector resize. */
KinokoInputDevice *kinoko_input_devices_begin(KinokoInputManager *manager);
KinokoInputDevice *kinoko_input_devices_end(KinokoInputManager *manager);
#ifdef __cplusplus
}
#endif
