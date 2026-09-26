#include "kinoko/game_runtime.h"
#include "kinoko/game_host.h"
#include "kinoko/application_runtime.hpp"
#include "kinoko/actor_records.hpp"
#include "kinoko/audio_runtime.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/renderer.h"
#include "kinoko/render_queue.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/stage_cleanup.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <initializer_list>
namespace {
std::vector<int> calls;
kinoko::actor::ManagerPrefix actors{};
KinokoGameObjects objects{nullptr, reinterpret_cast<KinokoActorManager*>(&actors), nullptr, nullptr};
bool callback_changes_mask=false, camera_changes_mask=false, alpha_changes_mask=false;
int begin_result=1;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"game lifecycle line %d: %s\n",__LINE__,#x); std::abort(); } } while (0)
void expect(std::initializer_list<int> expected) { CHECK(calls==std::vector<int>(expected)); calls.clear(); }
}
extern "C" {
KinokoGraphics kinoko_graphics{};
void kinoko_trace(const char*) {}
void kinoko_trace_i32(const char*,int32_t) {}
const KinokoGameObjects* kinoko_game_objects() { return &objects; }
void kinoko_game_prepare_scripts() { calls.push_back(1); }
int32_t kinoko_audio_initialize_playback() { calls.push_back(2); return 0; }
void kinoko_game_register_scripts() { calls.push_back(3); }
int32_t kinoko_game_initialize_input(KinokoInputManager*) { calls.push_back(4); return 0; }
int32_t kinoko_actor_manager_initialize(KinokoActorManager* p) { CHECK(p==objects.actors); calls.push_back(5); return 0; }
int32_t kinoko_camera_initialize(KinokoCamera*) { calls.push_back(6); return 0; }
int32_t kinoko_game_load_boot_script() { calls.push_back(7); return 71; }
void kinoko_game_clear_map(KinokoMapManager*) { calls.push_back(10); }
void kinoko_game_clear_actors(KinokoActorManager*) { calls.push_back(11); }
int32_t kinoko_clear_global_stages() { calls.push_back(12); return 0; }
int32_t kinoko_clear_global_sound() { calls.push_back(13); return 0; }
void kinoko_game_release_script_reference(uint32_t index) { calls.push_back(14+index); }
int32_t kinoko_game_close_vm() { calls.push_back(18); return 81; }
int32_t kinoko_game_update_input(KinokoInputManager*) { calls.push_back(20); return 0; }
void kinoko_game_update_callback(int32_t) { calls.push_back(21); if(callback_changes_mask) kinoko_game_masks.update=-1; }
int32_t __fastcall kinoko_camera_update(KinokoCamera*,void*) { calls.push_back(22); if(camera_changes_mask) kinoko_game_masks.update=0; return 22; }
int32_t kinoko_actor_manager_update(KinokoActorManager*,KinokoCamera*) { calls.push_back(23); CHECK(actors.update_mask==-1); return 23; }
int32_t kinoko_game_update_map(KinokoMapManager*) { calls.push_back(24); return 24; }
int32_t kinoko_stages_update() { calls.push_back(25); return 25; }
void kinoko_game_trace_map(KinokoMapManager*,int32_t) {}
int32_t kinoko_render_set_alpha(int32_t a,int32_t b) { CHECK(a==1&&b==0); calls.push_back(30); if(alpha_changes_mask) kinoko_game_masks.render=0; return 0; }
int32_t kinoko_render_set_cull(int32_t a) { CHECK(a==1); calls.push_back(31); return 0; }
int32_t kinoko_render_set_blend(int32_t a) { CHECK(a==1); calls.push_back(32); return 0; }
int32_t kinoko_render_set_filter(int32_t a) { CHECK(a==1); calls.push_back(33); return 0; }
void kinoko_game_prepare_map(KinokoMapManager*,KinokoCamera*) { calls.push_back(34); }
void kinoko_draw_render_queue(struct KinokoCamera*) { calls.push_back(35); }
int32_t kinoko_stages_prepare_draw() { calls.push_back(36); return 0; }
int32_t kinoko_stages_draw() { calls.push_back(37); return 0; }
int32_t kinoko_render_queue_identity() { return 0; }
int32_t kinoko_render_queue_first() { return 0; }
int32_t kinoko_graphics_begin_scene() { calls.push_back(40); return begin_result; }
int32_t kinoko_graphics_clear() { calls.push_back(41); return 0; }
int32_t kinoko_graphics_end_scene() { calls.push_back(42); return 0; }
}
int main() {
    CHECK(kinoko_game_masks.update==-1 && kinoko_game_masks.render==-1);
    auto* manager=kinoko::game::create_manager(); CHECK(manager);
    CHECK(manager->methods->initialize(manager)==71); expect({30,31,32,33,1,2,3,4,5,6,7});
    CHECK(manager->methods->create_scene(manager,1)==nullptr);
    CHECK(manager->methods->create_scene(manager,-1)==nullptr);
    auto* scene=manager->methods->create_scene(manager,0); CHECK(scene);
    CHECK(scene->methods->enter(scene,0)==0 && scene->methods->leave(scene,0)==0);
    kinoko_game_masks.update=0; callback_changes_mask=camera_changes_mask=true;
    CHECK(kinoko_game_update()==25); expect({20,21,22,23,24,25});
    callback_changes_mask=camera_changes_mask=false;
    CHECK(kinoko_game_update()==0); expect({20,21});
    kinoko_game_masks.update=static_cast<int32_t>(KINOKO_GAME_MAP);
    CHECK(kinoko_game_update()==24); expect({20,21,24});
    kinoko_graphics.cooperative_status=D3DERR_DEVICELOST;
    CHECK(scene->methods->update(scene)==0); expect({});
    kinoko_graphics.cooperative_status=D3D_OK;
    CHECK(scene->methods->update(scene)==0); expect({20,21,24});
    alpha_changes_mask=true;
    CHECK(kinoko_game_draw()==1); expect({30,31,32,33,34,35,36,37});
    alpha_changes_mask=false;
    CHECK(kinoko_game_draw()==1); expect({30,31,32,33,34,35});
    begin_result=0; CHECK(scene->methods->draw(scene)==0); expect({40});
    begin_result=1; CHECK(scene->methods->draw(scene)==1); expect({40,41,30,31,32,33,34,35,42});
    const auto* methods=scene->methods;
    scene->methods->destroy(scene,0); CHECK(scene->methods!=methods); std::free(scene);
    scene=manager->methods->create_scene(manager,0); CHECK(scene); scene->methods->destroy(scene,1);
    CHECK(manager->methods->shutdown(manager)==81); expect({10,11,12,13,14,15,16,17,18});
    std::free(manager);
}
