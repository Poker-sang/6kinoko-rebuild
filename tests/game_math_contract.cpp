#include "kinoko/game_math.h"
#include <windows.h>
#include <float.h>
#include <cmath>
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static unsigned rounding() {
    unsigned control = 0;
    _controlfp_s(&control, 0, 0);
    return control & _MCW_RC;
}
static DWORD WINAPI calculate(void *argument) {
    unsigned x87 = 0, sse = 0;
    CHECK(__control87_2(0, 0, &x87, &sse));
    CHECK((x87 & _MCW_RC) == _RC_UP && (sse & _MCW_RC) == _RC_UP);
    volatile float one = 1, increment = 0x1p-25f;
    const float sum = one + increment;
    CHECK(sum == std::nextafterf(1.0f, 2.0f));
    CHECK(argument == reinterpret_cast<void *>(123));
    return 73;
}
static DWORD WINAPI fail(void *) { throw 42; }
struct ThreadGate { HANDLE ready, release; };
static DWORD WINAPI wait_inside_scope(void *argument) {
    auto &gate = *static_cast<ThreadGate *>(argument);
    CHECK(rounding() == _RC_UP);
    SetEvent(gate.ready);
    CHECK(WaitForSingleObject(gate.release, 5000) == WAIT_OBJECT_0);
    return 0;
}
static DWORD WINAPI worker(void *argument) {
    const auto before = rounding();
    CHECK(kinoko_run_game_math(wait_inside_scope, argument) == 0);
    CHECK(rounding() == before);
    return 0;
}
int main() {
    unsigned current = 0;
    _controlfp_s(&current, _RC_DOWN, _MCW_RC);
    CHECK(kinoko_run_game_math(calculate, reinterpret_cast<void *>(123)) == 73);
    CHECK(rounding() == _RC_DOWN);
    try { kinoko_run_game_math(fail, nullptr); CHECK(false); }
    catch (int value) { CHECK(value == 42); }
    CHECK(rounding() == _RC_DOWN);
    ThreadGate gate{CreateEventW(nullptr, FALSE, FALSE, nullptr), CreateEventW(nullptr, FALSE, FALSE, nullptr)};
    CHECK(gate.ready && gate.release);
    HANDLE thread = CreateThread(nullptr, 0, worker, &gate, 0, nullptr);
    CHECK(thread && WaitForSingleObject(gate.ready, 5000) == WAIT_OBJECT_0);
    CHECK(rounding() == _RC_DOWN);
    SetEvent(gate.release);
    CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0);
    DWORD result = 1; CHECK(GetExitCodeThread(thread, &result) && result == 0);
    CloseHandle(thread); CloseHandle(gate.ready); CloseHandle(gate.release);
    std::puts("PASS: game rounding affects x87/SSE, restores after return/exception, and remains thread-local");
    return 0;
}

