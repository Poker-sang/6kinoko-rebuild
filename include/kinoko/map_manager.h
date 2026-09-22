#pragma once
#include "kinoko/act_types.h"
#include "kinoko/camera.h"
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
KinokoMapManager *kinoko_map_manager_construct(KinokoMapManager *manager);
void kinoko_map_manager_clear(KinokoMapManager *manager);
int32_t kinoko_map_manager_update(KinokoMapManager *manager);
KinokoActDocument *kinoko_map_manager_prepare(KinokoMapManager *manager, KinokoCamera *camera);
int32_t kinoko_map_manager_load(KinokoMapManager *manager, const char *path,
    struct SQVM *vm, void *map_class, void *root_object);
void kinoko_map_manager_assign(KinokoMapManager *destination, KinokoMapManager *source);
int32_t kinoko_map_manager_height(KinokoMapManager *manager);
void *kinoko_map_manager_containers(KinokoMapManager *manager);
#ifdef __cplusplus
}
#endif
