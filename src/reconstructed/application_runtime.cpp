#include "kinoko/base_utilities.h"
#include "kinoko/game_runtime.h"
#include "kinoko/direct_input.h"
#include "kinoko/timer_events.h"
#include "kinoko/application.h"
#include "kinoko/application_runtime.hpp"
#include "kinoko/audio_runtime.h"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/renderer.h"
#include "kinoko/render_target.h"
#include "kinoko/scene_queue.h"
#include "kinoko/ime_input.h"
#include "kinoko/archive_random.h"
#include "kinoko/game_math.h"
#include "kinoko/legacy_memory.hpp"
#include "../platform/resources/resource.h"
#include <mmsystem.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
void retdec_trace(const char*);
}

namespace kinoko::application {
State state;
namespace {
using kinoko::windows::CriticalLock;
using kinoko::windows::HandleOwner;
using kinoko::legacy::address;
struct ComScope {
    HRESULT status = CoInitialize(nullptr);
    ~ComScope() { if (SUCCEEDED(status)) CoUninitialize(); }
    explicit operator bool() const { return SUCCEEDED(status); }
};
void join(HandleOwner& thread) {
    if (!thread) return;
    WaitForSingleObject(thread.get(), INFINITE);
    thread.reset();
}
DWORD WINAPI load_scene_worker(void*) {
    ComScope com;
    if (com && state.config.manager) {
        auto* next = state.config.manager->methods->create_scene(
            state.config.manager, state.requested_scene);
        CriticalLock lock(&state.scene_lock);
        state.pending_scene = next;
    }
    return 0;
}
void update_statistics() {
    const DWORD now = timeGetTime();
    if (!state.config.show_fps || now - state.statistics_time < 1000) return;
    state.statistics_time += 1000;
    char title[256];
    std::snprintf(title, sizeof(title), "%s    Game:%uFPS    Draw:%u+%uFPS",
        kinoko_application_title(), state.frame_count, state.draw_count, state.present_count);
    SetWindowTextA(state.config.window, title);
    state.frame_count = state.draw_count = state.present_count = 0;
}
DWORD WINAPI game_loop(void*) {
    // This is a timer-registry borrow: unregister it, never close it separately.
    const HANDLE frame_event = kinoko_frame_timer_register();
    retdec_trace("game:entry");
    while (state.is_running()) {
        update_statistics();
        update_frame();
        if (!state.config.separate_draw) draw_frame();
        kinoko_math_checkpoint("render-done", 0);
        if (frame_event) kinoko_frame_timer_wait(frame_event);
        else Sleep(16); // Retained reconstruction fallback for event allocation failure.
    }
    if (frame_event) kinoko_frame_timer_unregister(frame_event);
    retdec_trace("game:exit");
    return 0;
}
DWORD WINAPI game_worker(void*) {
    ComScope com;
    if (com) kinoko_run_game_math(game_loop, nullptr);
    return 0;
}
DWORD WINAPI display_worker(void*) {
    ComScope com;
    if (!com) return 0;
    HandleOwner retry(CreateEventA(nullptr, FALSE, FALSE, nullptr));
    while (state.is_running()) {
        if (state.config.separate_draw) {
            if (TryEnterCriticalSection(&state.scene_lock)) {
                if (state.scene && draw_frame() && kinoko_graphics_present()) ++state.draw_count;
                LeaveCriticalSection(&state.scene_lock);
            }
            WaitForSingleObject(state.display_event.get(), 1);
        } else {
            if (WaitForSingleObject(state.display_event.get(), INFINITE) != WAIT_OBJECT_0 ||
                !state.is_running()) break;
            // 40E090..40E0E2: retry nonblocking presentation at most eight times.
            for (unsigned attempt = 0; attempt < 8 && state.is_running(); ++attempt) {
                if (kinoko_graphics_present()) { ++state.present_count; break; }
                if (retry) WaitForSingleObject(retry.get(), 1);
                else Sleep(1);
            }
        }
    }
    return 0;
}
DWORD WINAPI retire_worker(void*) {
    ComScope com;
    if (!com) return 0;
    while (state.is_running()) {
        if (WaitForSingleObject(state.retire_event.get(), INFINITE) != WAIT_OBJECT_0) break;
        kinoko_destroy_retired_scenes();
    }
    return 0;
}
bool initialize(const Configuration& configuration) {
    state.config = configuration;
    kinoko_application_set_archive_mode(configuration.archives != 0);
    state.timer_period = timeBeginPeriod(1) == TIMERR_NOERROR;
    kinoko_seed_random(timeGetTime());
    state.com_initialized = SUCCEEDED(CoInitialize(nullptr));
    if (!state.com_initialized) return false;
    kinoko_process_initialize(configuration.instance, configuration.window);
    if (!configuration.show_cursor) ShowCursor(FALSE);
    InterlockedExchange(&state.running, 1);
    if (configuration.graphics) {
        if (!kinoko_graphics_create(configuration.window, configuration.width, configuration.height)) return false;
        state.graphics_initialized = true;
        kinoko_renderer_initialize();
        state.renderer_initialized = true;
    }
    if (!initialize_input_audio(configuration)) return false;
    if (configuration.ime) { kinoko_ime_initialize(); state.ime_initialized = true; }
    state.statistics_time = timeGetTime();
    if (configuration.manager) configuration.manager->methods->initialize(configuration.manager);
    if (configuration.transition) configuration.transition->methods->initialize(configuration.transition);
    state.requested_scene = configuration.initial_scene;
    state.current_scene = -1;
    state.pending_scene = configuration.manager
        ? configuration.manager->methods->create_scene(configuration.manager, configuration.initial_scene) : nullptr;
    // Publish wake events before starting workers: shutdown must never signal a
    // null slot while its worker is just about to create and wait on that event.
    state.retire_event.reset(CreateEventA(nullptr, FALSE, FALSE, nullptr));
    if (configuration.graphics) state.display_event.reset(CreateEventA(nullptr, FALSE, FALSE, nullptr));
    if (!state.retire_event || (configuration.graphics && !state.display_event)) return false;
    state.retire_thread.reset(CreateThread(nullptr, 0, retire_worker, nullptr, 0, nullptr));
    if (state.retire_thread) SetThreadPriority(state.retire_thread.get(), THREAD_PRIORITY_IDLE);
    state.game_thread.reset(CreateThread(nullptr, 0, game_worker, nullptr, 0, nullptr));
    if (configuration.graphics) {
        state.display_thread.reset(CreateThread(nullptr, 0, display_worker, nullptr, 0, nullptr));
        if (state.display_thread) SetThreadPriority(state.display_thread.get(), THREAD_PRIORITY_IDLE);
    }
    return state.retire_thread && state.game_thread && (!configuration.graphics || state.display_thread);
}
void message_loop() {
    HandleOwner idle(CreateEventA(nullptr, FALSE, FALSE, nullptr));
    if (!idle) return;
    MSG message{};
    while (state.is_running()) {
        if (PeekMessageA(&message, nullptr, 0, 0, PM_NOREMOVE)) {
            if (GetMessageA(&message, nullptr, 0, 0) <= 0) break;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else {
            kinoko_graphics_poll();
            WaitForSingleObject(idle.get(), 16);
        }
    }
}
LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM key, LPARAM parameter) {
    return dispatch_message(window, message, key, parameter);
}
}

// 40D836..40D872: input service, keyboard and enumeration are required when
// enabled. Mouse creation is attempted, but its return does not gate startup.
bool initialize_input_audio(const Configuration& configuration) {
    if (configuration.input) {
        state.input_initialized = static_cast<uint8_t>(kinoko_input_initialize(
            configuration.window, configuration.instance)) != 0;
        if (!state.input_initialized) return false;
        if (!static_cast<uint8_t>(kinoko_input_open_keyboard())) return false;
        if (!static_cast<uint8_t>(kinoko_input_open_controllers())) return false;
        kinoko_input_open_mouse();
    }
    return !configuration.audio || static_cast<uint8_t>(kinoko_audio_initialize_device(
        configuration.window, configuration.audio_options)) != 0;
}

void update_frame() {
    if (state.config.input) kinoko_input_poll();
    kinoko_math_checkpoint("input-done", 0);
    auto* manager = state.config.manager;
    auto* transition = state.config.transition;
    if (manager) manager->methods->update(manager);
    if (state.current_scene != state.requested_scene) {
        if (!transition || transition->methods->ready(transition)) activate_pending_scene();
    } else {
        if (transition) transition->methods->update(transition);
        if (state.scene) {
            state.requested_scene = state.scene->methods->update(state.scene);
            if (state.requested_scene == -1) InterlockedExchange(&state.running, 0);
            else ++state.frame_count;
        }
        if (state.requested_scene != -1 && state.requested_scene != state.current_scene) {
            join(state.load_thread);
            state.load_thread.reset(CreateThread(nullptr, 0, load_scene_worker, nullptr, 0, nullptr));
        }
    }
    kinoko_math_checkpoint("scene-done", 0);
}
bool draw_frame() {
    { kinoko::graphics::Lock lock; kinoko_renderer.present_pending = 0; }
    int32_t ready = 1;
    if (state.scene) ready = state.scene->methods->draw(state.scene) & 1;
    if (state.config.manager) ready &= state.config.manager->methods->draw(state.config.manager);
    if (state.config.transition) ready &= state.config.transition->methods->draw(state.config.transition);
    if (ready) {
        { kinoko::graphics::Lock lock; kinoko_renderer.present_pending = 1; }
        if (!state.config.separate_draw && state.display_event) SetEvent(state.display_event.get());
    }
    return ready != 0;
}
LRESULT dispatch_message(HWND window, UINT message, WPARAM key, LPARAM parameter) {
    if (state.config.ime && static_cast<uint8_t>(kinoko_ime_dispatch(address(window), message,
            static_cast<uint32_t>(key), static_cast<int32_t>(parameter)))) return 0;
    switch (message) {
    case WM_SYSKEYDOWN:
        if (key == VK_RETURN) {
            const bool was_fullscreen = !kinoko_graphics.present.Windowed;
            { CriticalLock lock(&state.scene_lock); kinoko_graphics_toggle_window(); }
            if (state.config.show_cursor && was_fullscreen != !kinoko_graphics.present.Windowed)
                ShowCursor(kinoko_graphics.present.Windowed != 0);
            return 0;
        }
        if (key != VK_F4) return 0;
        break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_CREATE: case WM_QUIT: case WM_SYSKEYUP: case WM_IME_NOTIFY: return 0;
    case WM_SYSCOMMAND:
        if (key == SC_MONITORPOWER || key == SC_SCREENSAVE) return 1;
        break;
    }
    return DefWindowProcA(window, message, key, parameter);
}
}

extern "C" void kinoko_application_construct() {
    if (kinoko::application::state.constructed) return;
    kinoko_initialize_scene_queue();
    kinoko::application::state.constructed = true;
}
extern "C" int32_t kinoko_application_frame_count() {
    return static_cast<int32_t>(kinoko::application::state.frame_count);
}
extern "C" void kinoko_application_shutdown() {
    using namespace kinoko::application;
    InterlockedExchange(&state.running, 0);
    join(state.game_thread);
    join(state.load_thread);
    if (state.retire_event) SetEvent(state.retire_event.get());
    join(state.retire_thread);
    if (state.display_event) SetEvent(state.display_event.get());
    join(state.display_thread);
    kinoko_destroy_retired_scenes();
    state.retire_event.reset(); state.display_event.reset();
    {
        kinoko::windows::CriticalLock lock(&state.scene_lock);
        if (state.scene) state.scene->methods->destroy(state.scene, 1);
        state.scene = nullptr;
        if (state.pending_scene) state.pending_scene->methods->destroy(state.pending_scene, 1);
        state.pending_scene = nullptr;
    }
    if (auto* manager = state.config.manager) {
        manager->methods->shutdown(manager); std::free(manager); state.config.manager = nullptr;
    }
    if (auto* transition = state.config.transition) {
        transition->methods->shutdown(transition); std::free(transition); state.config.transition = nullptr;
    }
    if (state.ime_initialized) { kinoko_ime_release(state.config.window); state.ime_initialized = false; }
    kinoko_audio_shutdown_device();
    if (state.input_initialized) { kinoko_input_shutdown(); state.input_initialized = false; }
    if (state.renderer_initialized) {
        kinoko_renderer_before_reset(&kinoko_renderer, nullptr);
        kinoko_remove_device_listener(reinterpret_cast<KinokoDeviceListener*>(&kinoko_renderer));
        state.renderer_initialized = false;
    }
    if (state.graphics_initialized) { kinoko_graphics_release(); state.graphics_initialized = false; }
    if (state.com_initialized) { CoUninitialize(); state.com_initialized = false; }
    if (state.timer_period) { timeEndPeriod(1); state.timer_period = false; }
}
extern "C" int kinoko_application_run(HINSTANCE instance, int show_command) {
    using namespace kinoko::application;
    kinoko_application_initialize_host();
    kinoko::windows::HandleOwner singleton(CreateMutexA(nullptr, TRUE, kinoko_application_title()));
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 1;
    char executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, executable, MAX_PATH);
    if (length && length < MAX_PATH) {
        if (auto* slash = std::strrchr(executable, '\\')) { slash[1] = 0; SetCurrentDirectoryA(executable); }
    }
    WNDCLASSEXA cls{};
    cls.cbSize = sizeof(cls); cls.hInstance = instance; cls.lpszClassName = "Marisaland2";
    cls.lpfnWndProc = window_proc;
    cls.hIcon = cls.hIconSm = LoadIconA(instance, MAKEINTRESOURCEA(IDI_KINOKO));
    cls.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    cls.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if (!RegisterClassExA(&cls)) return 0;
    const int width = 640 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXDLGFRAME) + GetSystemMetrics(SM_CXBORDER);
    const int height = 480 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYCAPTION);
    HWND window = CreateWindowExA(WS_EX_APPWINDOW, cls.lpszClassName, kinoko_application_title(),
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
        width, height, nullptr, nullptr, instance, nullptr);
    if (!window) return 0;
    ShowWindow(window, show_command); UpdateWindow(window);
    Configuration config;
    config.window = window; config.instance = instance;
    config.manager = kinoko::game::create_manager();
    kinoko_application_open_archives();
    if (!config.manager || !initialize(config)) MessageBoxA(window, kinoko_application_error(), "Error", MB_OK);
    else message_loop();
    kinoko_application_shutdown();
    return 0;
}
