#pragma once
#include "kinoko/actor_manager.h"
#include "kinoko/act_types.h"
typedef struct KinokoInputManager KinokoInputManager;
typedef struct KinokoGameObjects {
    KinokoInputManager *input;
    KinokoActorManager *actors;
    KinokoCamera *camera;
    KinokoMapManager *map;
} KinokoGameObjects;
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed global objects. Integer conversion remains only inside these
   ports to unconverted manager / Squirrel wrapper implementations. */
const KinokoGameObjects *kinoko_game_objects(void);
void kinoko_game_prepare_scripts(void);
void kinoko_game_register_scripts(void);
int32_t kinoko_game_initialize_input(KinokoInputManager *input);
int32_t kinoko_game_update_input(KinokoInputManager *input);
int32_t kinoko_game_load_boot_script(void);
void kinoko_game_update_callback(int32_t trace_index);
int32_t kinoko_game_update_map(KinokoMapManager *map);
void kinoko_game_prepare_map(KinokoMapManager *map, KinokoCamera *camera);
void kinoko_game_clear_map(KinokoMapManager *map);
void kinoko_game_clear_actors(KinokoActorManager *actors);
void kinoko_game_trace_map(KinokoMapManager *map, int32_t drawing);
void kinoko_game_release_script_reference(uint32_t index);
int32_t kinoko_game_close_vm(void);
#ifdef __cplusplus
}
#endif
