#include "kinoko/critical_section.h"
#include "kinoko/game_runtime.h"
#include "kinoko/direct_input.h"
#include "kinoko/timer_events.h"
#include "kinoko/application_runtime.hpp"
#include "kinoko/renderer.h"
#include "kinoko/scene_queue.h"
#include "kinoko/render_target.h"
#include "kinoko/game_math.h"
#include <array>
#include <cstdio>
#include <vector>

namespace {
std::vector<int> device_calls;
int input_result=1, keyboard_result=1, controllers_result=1, mouse_result=1, audio_result=1;
}
extern "C" {
KinokoRenderer kinoko_renderer{};
KinokoCriticalSection kinoko_graphics_lock{};
KinokoGraphics kinoko_graphics{};
int32_t g534 = 0;
char kinoko_packed_assets = 0;
// Isolated host/device ports. No game startup, timer or device is invoked by
// this contract; only the callback/state functions below are under examination.
void retdec_trace(const char*) {}
void kinoko_application_initialize_host() {}
void kinoko_application_open_archives() {}
void kinoko_application_set_archive_mode(int32_t enabled) { kinoko_packed_assets = enabled != 0; }
const char* kinoko_application_title() { return "fixture"; }
const char* kinoko_application_error() { return "fixture"; }
void kinoko_seed_random(uint32_t) {}
int32_t kinoko_audio_initialize_device(HWND, int32_t) { device_calls.push_back(5); return audio_result; }
int32_t kinoko_audio_shutdown_device() { return 0; }
int32_t kinoko_graphics_create(HWND, int32_t, int32_t) { return 0; }
int32_t kinoko_graphics_release() { return 0; }
int32_t kinoko_graphics_toggle_window() { return 0; }
HRESULT kinoko_graphics_poll() { return S_OK; }
int32_t kinoko_graphics_present() { return 0; }
int32_t kinoko_renderer_initialize() { return 0; }
int32_t __fastcall kinoko_renderer_before_reset(KinokoRenderer*, void*) { return 0; }
void kinoko_remove_device_listener(KinokoDeviceListener*) {}
int32_t kinoko_ime_dispatch(int32_t, uint32_t, uint32_t, int32_t) { return 0; }
unsigned long kinoko_run_game_math(unsigned long (__stdcall *)(void*), void*) noexcept(false) { return 0; }
int32_t kinoko_process_initialize(HINSTANCE, HWND) { return 0; }
int32_t kinoko_input_initialize(HWND, HINSTANCE) { device_calls.push_back(1); return input_result; }
int32_t kinoko_input_shutdown() { return 0; }
int32_t kinoko_input_open_keyboard() { device_calls.push_back(2); return keyboard_result; }
int32_t kinoko_input_open_controllers() { device_calls.push_back(3); return controllers_result; }
int32_t kinoko_input_open_mouse() { device_calls.push_back(4); return mouse_result; }
int32_t kinoko_input_poll() { return 0; }
int32_t kinoko_ime_initialize() { return 0; }
void kinoko_ime_release(HWND) {}
HANDLE kinoko_frame_timer_register() { return nullptr; }
void kinoko_frame_timer_wait(HANDLE) {}
int32_t kinoko_frame_timer_unregister(HANDLE) { return 0; }
}
namespace {
using namespace kinoko::application;
std::vector<int> calls;
int32_t next_scene = 7;
int32_t __fastcall manager_update(Manager*, void*) { calls.push_back(1); return 0; }
int32_t __fastcall transition_update(Transition*, void*) { calls.push_back(2); return 0; }
int32_t __fastcall scene_update(Scene*, void*) { calls.push_back(3); return next_scene; }
int32_t __fastcall scene_draw(Scene*, void*) { calls.push_back(4); return 0; }
int32_t __fastcall manager_draw(Manager*, void*) { calls.push_back(5); return 1; }
int32_t __fastcall transition_draw(Transition*, void*) { calls.push_back(6); return 1; }
int32_t __fastcall scene_leave(Scene*, void*, int32_t id) { calls.push_back(100 + id); return 0; }
int32_t __fastcall scene_enter(Scene*, void*, int32_t id) { calls.push_back(200 + id); return 0; }
void* __fastcall scene_destroy(Scene* scene, void*, unsigned flags) { calls.push_back(300 + flags); return scene; }
template<class T, class U> T method(U callback) { return reinterpret_cast<T>(callback); }
#define CHECK(value) do { if (!(value)) { std::fprintf(stderr, "application line %d: %s\n", __LINE__, #value); return 1; } } while (0)
}
int main() {
    using namespace kinoko::application;
    InitializeCriticalSection(&kinoko_graphics_lock.native);
    Configuration devices;
    for (int failure=1; failure<=5; ++failure) {
        input_result=keyboard_result=controllers_result=mouse_result=audio_result=1;
        int* results[]={&input_result,&keyboard_result,&controllers_result,&mouse_result,&audio_result};
        *results[failure-1]=0; device_calls.clear(); state.input_initialized=false;
        CHECK(initialize_input_audio(devices)==(failure==4));
        const int last=failure==4 ? 5 : failure;
        std::vector<int> expected;
        for(int i=1;i<=last;++i) expected.push_back(i);
        CHECK(device_calls==expected && state.input_initialized==(failure!=1));
    }
    input_result=0x100; device_calls.clear(); state.input_initialized=false;
    CHECK(!initialize_input_audio(devices) && device_calls==std::vector<int>{1});
    devices.input=devices.audio=0; device_calls.clear();
    CHECK(initialize_input_audio(devices) && device_calls.empty());
    devices.audio=1; audio_result=1;
    CHECK(initialize_input_audio(devices) && device_calls==std::vector<int>{5});
    state.input_initialized=false;
    ManagerMethods manager_methods{};
    manager_methods.update = method<decltype(manager_methods.update)>(manager_update);
    manager_methods.draw = method<decltype(manager_methods.draw)>(manager_draw);
    TransitionMethods transition_methods{};
    transition_methods.update = method<decltype(transition_methods.update)>(transition_update);
    transition_methods.draw = method<decltype(transition_methods.draw)>(transition_draw);
    SceneMethods scene_methods{};
    scene_methods.update = method<decltype(scene_methods.update)>(scene_update);
    scene_methods.draw = method<decltype(scene_methods.draw)>(scene_draw);
    scene_methods.enter = method<decltype(scene_methods.enter)>(scene_enter);
    scene_methods.leave = method<decltype(scene_methods.leave)>(scene_leave);
    scene_methods.destroy = method<decltype(scene_methods.destroy)>(scene_destroy);
    Manager manager{&manager_methods};
    Transition transition{&transition_methods};
    Scene first{&scene_methods}, second{&scene_methods};
    state.config.input = 0;
    state.config.manager = &manager; state.config.transition = &transition;
    state.current_scene = state.requested_scene = 7; state.scene = &first;
    InterlockedExchange(&state.running, 1);
    update_frame();
    CHECK((calls == std::vector<int>{1, 2, 3}));
    CHECK(state.frame_count == 1 && state.requested_scene == 7);
    calls.clear();
    CHECK(!draw_frame());
    CHECK((calls == std::vector<int>{4, 5, 6})); // No short-circuiting callbacks.
    CHECK(!kinoko_renderer.present_pending);
    calls.clear();
    state.pending_scene = &second; state.requested_scene = 9;
    activate_pending_scene();
    CHECK((calls == std::vector<int>{109, 207}));
    CHECK(state.scene == &second && !state.pending_scene && state.current_scene == 9);
    kinoko_destroy_retired_scenes();
    CHECK((calls == std::vector<int>{109, 207, 301}));
    next_scene = -1;
    update_frame();
    CHECK(!state.is_running() && state.requested_scene == -1 && state.frame_count == 1);
    state.scene = nullptr; state.config.manager = nullptr; state.config.transition = nullptr;
    DeleteCriticalSection(&kinoko_graphics_lock.native);
    std::puts("PASS: typed application callbacks, transition IDs, deferred destruction and scene exit");
}

namespace kinoko::game { application::Manager *create_manager() { return nullptr; } }
