#include "kinoko/game_host.h"
#include "kinoko/base_utilities.h"
#include "kinoko/application.h"
#include "kinoko/file_io.h"
#include "kinoko/render_target.h"
#include "kinoko/timer_events.h"
#include "kinoko/map_manager.h"
#include "kinoko/scene_operations.h"
#include "kinoko/graphics_device.h"
#include "kinoko/renderer.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/legacy_memory.hpp"
extern "C" {
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
}
// The original CRT constructs these globals before WinMain; the replacement
// entry constructs them explicitly before opening the three archives. The
// borrowed identities remain in the legacy host's global storage.
extern "C" void kinoko_runtime_initialize_objects(const KinokoRuntimeBootSymbols* symbols) {
    static bool initialized = false;
    static bool map_constructed = false;
    if (initialized) return;
    kinoko_graphics_initialize_runtime();
    kinoko_renderer_construct(symbols->renderer_methods);
    kinoko_initialize_texture_cache();
    kinoko_archive_initialize();
    kinoko_register_stage_list_cleanup();
    kinoko_register_render_queue_cleanup();
    kinoko_register_sound_tree_cleanup();
    kinoko_application_construct();
    kinoko_frame_timer_initialize();
    if (!map_constructed) {
        if (!kinoko_map_manager_construct(symbols->map)) {
            retdec_trace("map-manager:init-failed");
        } else {
            map_constructed = true;
            retdec_trace_i32("map-manager:sentinel",
                kinoko::legacy::address(kinoko_map_manager_containers(symbols->map)));
        }
    }
    if (!kinoko_actor_manager_construct(symbols->actors))
        retdec_trace("actor-manager:construct-failed");
    *symbols->render_layer_owner_slot = kinoko::legacy::address(symbols->actors);
    kinoko_actor_trace_render_layers(symbols->actors);
    initialized = true;
}
