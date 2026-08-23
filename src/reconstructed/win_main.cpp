#include "kinoko/archive.hpp"
#include "kinoko/renderer.hpp"

#include <windows.h>

#include <filesystem>
#include <string>

namespace {

constexpr char kWindowClass[] = "Marisaland2";
constexpr char kWindowTitle[] =
    "\x96\x82\x97\x9d\x8d\xb9\x82\xc6\x82\x55\x82\xc2\x82\xcc\x83\x4c\x83\x6d\x83\x52";

struct AppState {
    kinoko::AssetStore assets;
    kinoko::GdiRenderer renderer;
    kinoko::RuntimeView view;
    std::filesystem::path data_directory;
};

std::filesystem::path executable_directory() {
    char buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(std::string(buffer, length)).parent_path();
}

std::filesystem::path command_line_data_directory() {
    for (int i = 1; i < __argc; ++i) {
        const std::string argument = __argv[i] != nullptr ? __argv[i] : "";
        constexpr const char prefix[] = "--data-dir=";
        if (argument.rfind(prefix, 0) == 0) {
            return std::filesystem::path(argument.substr(sizeof(prefix) - 1));
        }
        if (argument == "--data-dir" && i + 1 < __argc) {
            return std::filesystem::path(__argv[++i]);
        }
    }
    char environment[MAX_PATH]{};
    const DWORD environment_length = GetEnvironmentVariableA(
        "KINOKO_DATA_DIR", environment, static_cast<DWORD>(sizeof(environment)));
    if (environment_length != 0 && environment_length < sizeof(environment)) {
        return std::filesystem::path(std::string(environment, environment_length));
    }
    return executable_directory();
}

void load_assets(AppState& app) {
    app.data_directory = command_line_data_directory();
    std::string error;
    app.view.mounted = app.assets.mount_standard_set(app.data_directory, error);
    app.view.archive_count = app.assets.archive_count();
    app.view.entry_count = app.assets.entry_count();
    app.view.data_directory = app.data_directory.string();
    app.view.error = error;
    if (!error.empty()) {
        OutputDebugStringA((error + "\n").c_str());
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* app = reinterpret_cast<AppState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTA*>(lparam);
        app = static_cast<AppState*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    switch (message) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (wparam == VK_F5 && app != nullptr) {
            app->assets = kinoko::AssetStore{};
            load_assets(*app);
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if (app != nullptr) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            RECT client{};
            GetClientRect(hwnd, &client);
            app->renderer.paint(dc, client, app->view);
            EndPaint(hwnd, &paint);
            return 0;
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

} // namespace

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show_command) {
    HANDLE mutex = CreateMutexA(nullptr, TRUE, "Local\\6kinoko-rebuild");
    if (mutex == nullptr || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex != nullptr) {
            CloseHandle(mutex);
        }
        return 1;
    }

    WNDCLASSEXA window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.hInstance = instance;
    window_class.lpfnWndProc = window_proc;
    window_class.lpszClassName = kWindowClass;
    window_class.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    window_class.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    window_class.hIconSm = window_class.hIcon;
    window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    if (RegisterClassExA(&window_class) == 0) {
        CloseHandle(mutex);
        return 1;
    }

    RECT desired{0, 0, 640, 480};
    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&desired, style, FALSE, WS_EX_APPWINDOW);

    AppState app;
    load_assets(app);
    HWND window = CreateWindowExA(
        WS_EX_APPWINDOW,
        kWindowClass,
        kWindowTitle,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        desired.right - desired.left,
        desired.bottom - desired.top,
        nullptr,
        nullptr,
        instance,
        &app);
    if (window == nullptr) {
        CloseHandle(mutex);
        return 1;
    }

    ShowWindow(window, show_command);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    CloseHandle(mutex);
    return static_cast<int>(message.wParam);
}
