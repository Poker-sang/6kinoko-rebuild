#include "kinoko/game_runtime.h"
#include "kinoko/game_host.h"
#include "kinoko/base_utilities.h"
#include "kinoko/application.h"
#include "kinoko/file_io.h"
#include "kinoko/render_target.h"
#include "kinoko/timer_events.h"
#include "kinoko/map_manager.h"
#include "kinoko/scene_operations.h"
#include "kinoko/application_runtime.hpp"
#include "kinoko/actor_records.hpp"
#include "kinoko/audio_runtime.h"
#include "kinoko/camera.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/renderer.h"
#include "kinoko/render_queue.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
extern "C" {
void retdec_trace(const char*);
void retdec_trace_i32(const char*,int32_t);
int __cdecl _purecall(void);
KinokoGameMasks kinoko_game_masks{-1,-1};
}
namespace {
void render_defaults() {
    kinoko_render_set_alpha(1,0);
    kinoko_render_set_cull(1);
    kinoko_render_set_blend(1);
    kinoko_render_set_filter(1);
}
}
extern "C" int32_t kinoko_game_initialize(void) {
    const auto& objects=*kinoko_game_objects();
    retdec_trace("469640:enter");
    kinoko_game_prepare_scripts();
    retdec_trace("469640:audio-begin");
    kinoko_audio_initialize_playback();
    retdec_trace("469640:audio-done");
    kinoko_game_register_scripts();
    retdec_trace("469640:alloc-begin");
    kinoko_game_initialize_input(objects.input);
    retdec_trace("469640:alloc-done");
    retdec_trace("469640:actor-begin");
    kinoko_actor_manager_initialize(objects.actors);
    retdec_trace("469640:actor-done");
    retdec_trace("469640:global-begin");
    kinoko_camera_initialize(objects.camera);
    retdec_trace("469640:global-done");
    return kinoko_game_load_boot_script();
}
extern "C" int32_t kinoko_game_release_script_state(void) {
    // 470F30 releases four distinct roots before the existing double close port.
    for (uint32_t index=0; index<4; ++index) kinoko_game_release_script_reference(index);
    return kinoko_game_close_vm();
}
extern "C" int32_t kinoko_game_shutdown(void) {
    const auto& objects=*kinoko_game_objects();
    kinoko_game_clear_map(objects.map);
    kinoko_game_clear_actors(objects.actors);
    kinoko_clear_global_stages();
    kinoko_clear_global_sound();
    return kinoko_game_release_script_state();
}
extern "C" int32_t kinoko_game_update(void) {
    const auto& objects=*kinoko_game_objects();
    static volatile LONG trace_count;
    const LONG trace_index=InterlockedIncrement(&trace_count);
    if (trace_index<=16) {
        retdec_trace("469900:entry");
        retdec_trace_i32("469900:update-mask",kinoko_game_masks.update);
        retdec_trace_i32("469900:render-mask",kinoko_game_masks.render);
        kinoko_game_trace_map(objects.map,0);
    }
    kinoko_game_update_input(objects.input);
    kinoko_game_update_callback(trace_index);
    // 46995C snapshots AFTER the callback; later stages use this same mask.
    const uint32_t mask=static_cast<uint32_t>(kinoko_game_masks.update);
    int32_t result=static_cast<int32_t>(mask);
    if (mask & KINOKO_GAME_CAMERA) result=kinoko_camera_update(objects.camera,nullptr);
    if (mask & KINOKO_GAME_ACTOR_GROUPS) {
        kinoko::actor::ManagerView(objects.actors).set(&kinoko::actor::ManagerPrefix::update_mask,static_cast<int32_t>(mask));
        result=kinoko_actor_manager_update(objects.actors,objects.camera);
    }
    if (mask & KINOKO_GAME_MAP) {
        if (trace_index<=16) retdec_trace("469900:map-update");
        result=kinoko_game_update_map(objects.map);
    }
    if (mask & KINOKO_GAME_STAGES) {
        if (trace_index<=16) retdec_trace("469900:global-update");
        result=kinoko_stages_update();
    }
    if (trace_index<=16) retdec_trace_i32("469900:result",result);
    return result;
}
extern "C" int32_t kinoko_game_draw(void) {
    const auto& objects=*kinoko_game_objects();
    const uint32_t mask=static_cast<uint32_t>(kinoko_game_masks.render);
    static volatile LONG trace_count;
    const LONG trace_index=InterlockedIncrement(&trace_count);
    if (trace_index==1) {
        retdec_trace_i32("render:g613",kinoko_render_queue_identity());
        retdec_trace_i32("render:g613-first",kinoko_render_queue_first());
    }
    render_defaults();
    if (trace_index<=3) kinoko_game_trace_map(objects.map,1);
    kinoko_game_prepare_map(objects.map,objects.camera);
    // Legacy render queue API still accepts an address slot; no ownership transfer.
    kinoko_draw_render_queue(kinoko::legacy::address(objects.camera));
    if (mask & KINOKO_GAME_STAGES) { kinoko_stages_prepare_draw(); kinoko_stages_draw(); }
    return 1;
}
namespace kinoko::game {
namespace {
using namespace application;
extern const SceneMethods base_methods;
void* __fastcall destroy_scene(Scene* scene,void*,unsigned flags) {
    if (!scene) return nullptr;
    scene->methods=&base_methods;
    if (flags&1) std::free(scene);
    return scene;
}
void __fastcall scene_noop(Scene*,void*) {}
int32_t __fastcall scene_transition(Scene*,void*,int32_t) { return 0; }
int32_t __fastcall scene_update(Scene*,void*) {
    if (kinoko_graphics.cooperative_status==D3D_OK) kinoko_game_update();
    return 0; // original logo scene stays at ID zero
}
int32_t __fastcall scene_draw(Scene*,void*) {
    if (!kinoko_graphics_begin_scene()) return 0;
    kinoko_graphics_clear();
    kinoko_game_draw();
    kinoko_graphics_end_scene();
    return 1; // 45D9E8 ignores EndScene's return
}
#define SCENE_METHOD(field,fn) reinterpret_cast<decltype(SceneMethods::field)>(fn)
const SceneMethods base_methods{
    SCENE_METHOD(destroy,destroy_scene),SCENE_METHOD(update,_purecall),SCENE_METHOD(draw,_purecall),
    reinterpret_cast<void*>(scene_noop),SCENE_METHOD(enter,scene_transition),SCENE_METHOD(leave,scene_transition)};
const SceneMethods game_methods{
    SCENE_METHOD(destroy,destroy_scene),SCENE_METHOD(update,scene_update),SCENE_METHOD(draw,scene_draw),
    reinterpret_cast<void*>(scene_noop),SCENE_METHOD(enter,scene_transition),SCENE_METHOD(leave,scene_transition)};
#undef SCENE_METHOD
int32_t __fastcall initialize(Manager*,void*) {
    retdec_trace("45da00:enter"); render_defaults();
    retdec_trace("45da00:before-469640");
    const auto result=kinoko_game_initialize();
    retdec_trace_i32("45da00:after-469640",result); return result;
}
int32_t __fastcall shutdown(Manager*,void*) { return kinoko_game_shutdown(); }
int32_t __fastcall update(Manager*,void*) { return 0; }
int32_t __fastcall draw(Manager*,void*) { return 1; }
Scene* __fastcall create(Manager*,void*,int32_t id) {
    if (id!=0) return nullptr; // original checks before allocating
    auto* scene=static_cast<Scene*>(std::malloc(sizeof(Scene)));
    if (scene) scene->methods=&game_methods;
    return scene;
}
#define MANAGER_METHOD(field,fn) reinterpret_cast<decltype(ManagerMethods::field)>(fn)
const ManagerMethods manager_methods{
    MANAGER_METHOD(initialize,initialize),MANAGER_METHOD(shutdown,shutdown),
    MANAGER_METHOD(update,update),MANAGER_METHOD(draw,draw),MANAGER_METHOD(create_scene,create)};
#undef MANAGER_METHOD
}
application::Manager* create_manager() {
    auto* manager=static_cast<application::Manager*>(std::malloc(sizeof(application::Manager)));
    if (manager) manager->methods=&manager_methods;
    return manager;
}
}
