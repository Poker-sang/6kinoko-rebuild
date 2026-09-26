#include "kinoko/legacy_abi.h"

static_assert(sizeof(void *) == 4, "The original method ABI requires Win32.");

namespace {
template <typename Result, typename... Args>
Result call_method(void *object, void *method, Args... args) {
    using Method = Result (__thiscall *)(void *, Args...);
    return reinterpret_cast<Method>(method)(object, args...);
}
}

extern "C" void kinoko_call_thiscall0(void *object, void *method) {
    call_method<void>(object, method);
}

extern "C" void kinoko_call_thiscall1(void *object, void *method, int32_t argument) {
    call_method<void>(object, method, argument);
}

extern "C" int32_t kinoko_call_thiscall0_result(void *object, void *method) {
    return call_method<int32_t>(object, method);
}

extern "C" int32_t kinoko_call_thiscall1_result(
    void *object, void *method, int32_t argument) {
    return call_method<int32_t>(object, method, argument);
}

extern "C" int32_t kinoko_call_thiscall2_result(
    void *object, void *method, int32_t argument1, int32_t argument2) {
    return call_method<int32_t>(object, method, argument1, argument2);
}

extern "C" int32_t kinoko_call_thiscall3_result(void *object, void *method,
    int32_t argument1, int32_t argument2, int32_t argument3) {
    return call_method<int32_t>(object, method, argument1, argument2, argument3);
}

extern "C" int32_t kinoko_call_thiscall4_result(void *object, void *method,
    int32_t argument1, int32_t argument2, int32_t argument3, int32_t argument4) {
    return call_method<int32_t>(object, method,
        argument1, argument2, argument3, argument4);
}

extern "C" int32_t kinoko_call_draw_method(void *object, void *method,
    int32_t x, int32_t y, int32_t width, int32_t height, struct KinokoActResource* resource,
    int32_t source_x, int32_t source_y, int32_t blend, float alpha) {
    return call_method<int32_t>(object, method,
        x, y, width, height, resource, source_x, source_y, blend, alpha);
}
