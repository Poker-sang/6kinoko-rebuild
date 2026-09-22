#pragma once
#include "kinoko/windows_owner.hpp"
#include <cstddef>
#include <cstdint>

namespace kinoko::application {
struct Scene;
struct Manager;
struct Transition;
struct SceneMethods {
    void* (__thiscall *destroy)(Scene*, unsigned);
    int32_t (__thiscall *update)(Scene*);
    int32_t (__thiscall *draw)(Scene*);
    void *reserved;
    int32_t (__thiscall *enter)(Scene*, int32_t);
    int32_t (__thiscall *leave)(Scene*, int32_t);
};
struct Scene { const SceneMethods *methods; };
struct ManagerMethods {
    int32_t (__thiscall *initialize)(Manager*);
    int32_t (__thiscall *shutdown)(Manager*);
    int32_t (__thiscall *update)(Manager*);
    int32_t (__thiscall *draw)(Manager*);
    Scene* (__thiscall *create_scene)(Manager*, int32_t);
};
struct Manager { const ManagerMethods *methods; };
struct TransitionMethods {
    int32_t (__thiscall *initialize)(Transition*);
    int32_t (__thiscall *shutdown)(Transition*);
    int32_t (__thiscall *update)(Transition*);
    uint8_t (__thiscall *ready)(Transition*);
    int32_t (__thiscall *draw)(Transition*);
};
struct Transition { const TransitionMethods *methods; };
// Original 40D790 copies exactly 40 bytes to application+8.
struct Configuration {
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    int32_t width = 0, height = 0, audio_options = 0;
    Manager *manager = nullptr;
    int32_t initial_scene = 0;
    Transition *transition = nullptr;
    uint8_t graphics = 1, input = 1, audio = 1, ime = 0;
    uint8_t show_cursor = 1, separate_draw = 0, show_fps = 0, archives = 1;
};
static_assert(sizeof(Configuration) == 40);
static_assert(offsetof(Configuration, manager) == 20);
static_assert(offsetof(Configuration, graphics) == 32);
static_assert(offsetof(Configuration, archives) == 39);
static_assert(offsetof(SceneMethods, enter) == 16);
static_assert(offsetof(ManagerMethods, create_scene) == 16);

// Native owner, no longer overlaid on the split RetDec globals or byte block.
struct State {
    Configuration config;
    kinoko::windows::HandleOwner game_thread, display_thread, retire_thread, load_thread;
    kinoko::windows::HandleOwner display_event, retire_event;
    CRITICAL_SECTION scene_lock{};
    volatile LONG running = 0;
    Scene *scene = nullptr, *pending_scene = nullptr;
    int32_t requested_scene = 0, current_scene = -1;
    uint32_t frame_count = 0, draw_count = 0, present_count = 0;
    DWORD statistics_time = 0;
    bool com_initialized = false, timer_period = false, input_initialized = false;
    bool graphics_initialized = false, renderer_initialized = false, ime_initialized = false;
    bool constructed = false;
    State() { InitializeCriticalSection(&scene_lock); }
    ~State() { DeleteCriticalSection(&scene_lock); }
    bool is_running() { return InterlockedCompareExchange(&running, 0, 0) != 0; }
    State(const State&) = delete;
    State& operator=(const State&) = delete;
};
extern State state;
void activate_pending_scene();
void update_frame();
bool draw_frame();
LRESULT dispatch_message(HWND window, UINT message, WPARAM key, LPARAM parameter);
}
