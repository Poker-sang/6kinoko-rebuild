#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoGameMasks { int32_t update, render; } KinokoGameMasks;
extern KinokoGameMasks kinoko_game_masks;
#define KINOKO_GAME_ACTOR_GROUPS 0x1fffffffu
#define KINOKO_GAME_CAMERA 0x20000000u
#define KINOKO_GAME_STAGES 0x40000000u
#define KINOKO_GAME_MAP 0x80000000u
int32_t kinoko_game_initialize(void);
int32_t kinoko_game_shutdown(void);
int32_t kinoko_game_update(void);
int32_t kinoko_game_draw(void);
int32_t kinoko_game_release_script_state(void);
#ifdef __cplusplus
}
namespace kinoko::application { struct Manager; }
namespace kinoko::game {
application::Manager *create_manager();
}
#endif
