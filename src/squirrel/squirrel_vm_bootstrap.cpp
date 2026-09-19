#include "kinoko/squirrel_vm_bootstrap.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include <squirrel.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
extern char g642;
extern int32_t g643;
extern char* g644;
extern int32_t g645;
extern int32_t unk_5149EC[3];
int32_t function_48a170(int32_t stack_size);
int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
}
namespace {
using kinoko::script::address;
using kinoko::script::pointer;
HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(g644); }
int32_t function_address(void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
}
extern "C" int32_t function_4a8c50(void) {
    if (g645 != 0) {
        function_4a9570_this(g645);
        std::free(pointer<void>(g645));
        g645 = 0;
    }
    if (g642 == 0) function_4a9570_this(address(unk_5149EC));
    g644 = nullptr;
    return 0;
}
extern "C" int32_t function_4a8c90(int32_t, const char* format, ...) {
    char message[4096]{};
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format ? format : "", arguments);
    va_end(arguments);
    return std::puts(message);
}
extern "C" int32_t function_4a8cc0(void) {
    if (g645 != 0) return g645;
    auto* vm = current_vm();
    if (!vm) return 0;
    sq_pushroottable(vm);
    const int32_t storage = _3f__3f_2_40_YAPAXI_40_Z(12);
    if (storage != 0) function_4a94e0_this(storage);
    g645 = storage;
    function_4a9660_this(storage, -1);
    sq_pop(vm, 1);
    return g645;
}
extern "C" int32_t function_4a8db0(int32_t requested_vm) {
    int32_t current = requested_vm;
    if (requested_vm != 0 && address(g644) == requested_vm) return 1;
    // Preserve the recovered boundary: the original virtual cleanup receiver
    // is not recovered here, so do not invent a destructor/free operation.
    if (g645 != 0) g645 = 0;
    if (g642 == 0) function_4a9570_this(address(unk_5149EC));
    g644 = nullptr;
    if (requested_vm == 0) {
        current = function_48a170(1024);
        if (current == 0) return 0;
        const int32_t node = _3f__3f_2_40_YAPAXI_40_Z(8);
        if (node != 0) {
            *pointer<int32_t>(node) = kinoko_sq_shared_state(current);
            *pointer<int32_t>(node + 4) = g643;
            g643 = node;
        }
        function_48b8b0(current, function_address(reinterpret_cast<void*>(&function_4a8c90)));
        function_48a670(current);
        function_4c7c90(current);
        function_4c73a0(current);
        function_4c6c20(current);
        function_4c6670(current);
        function_4c5c80(current);
        function_48aa30(current, 1);
    }
    g642 = 0;
    g644 = pointer<char>(current);
    const int32_t owner_result = function_4a9e30_this(address(unk_5149EC), current);
    return (owner_result & -256) | 1;
}
extern "C" int32_t function_4a90c0(int32_t* object, int32_t* klass) {
    return function_4a90c0_this(address(object), address(klass));
}
