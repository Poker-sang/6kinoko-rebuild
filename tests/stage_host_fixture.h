#pragma once
/* Fixture-only legacy byte views of the actual linked production host objects. */
#include "../src/reconstructed/runtime_host_internal.h"
#define g_kinoko_actor_manager_state ((unsigned char *)kinoko_game_objects()->actors)
#define g_kinoko_map_manager_state ((unsigned char *)kinoko_game_objects()->map)
#define g_kinoko_camera_state ((unsigned char *)kinoko_game_objects()->camera)
#define g_kinoko_input_manager_state ((unsigned char *)kinoko_game_objects()->input)
#define g_514300_storage ((int32_t *)kinoko_game_collision_state())
#define g612 ((int32_t *)kinoko_game_global_callback())
