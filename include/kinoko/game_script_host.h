#pragma once
#include "kinoko/game_script_api.h"
#include "kinoko/script_callbacks.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow existing C-host storage; accesses use RecordView in the C++ layer. */
KinokoScriptCallback *kinoko_game_global_callback(void);
KinokoCollisionState *kinoko_game_collision_state(void);
void kinoko_game_initialize_callback(KinokoScriptCallback *callback);
int32_t kinoko_game_load_map_file(const char *path);
int32_t kinoko_game_release_map_state(void);
void kinoko_game_split_path(const char *path, char *directory);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoScriptCallback)==28);
#endif
