from pathlib import Path
import re, subprocess

repo = Path(".")
source = repo / "src/decompiled/6kinoko_rebuilt.c"
text = source.read_text()

def remove_definition(text, signature):
    start = text.find(signature)
    assert start >= 0, signature
    brace = text.find("{", start)
    semi = text.find(";", start)
    assert brace >= 0 and (semi < 0 or brace < semi), ("prototype selected", signature)
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{": depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                name = signature.split("(")[0].split()[-1]
                return text[:start] + "/* " + name + " is implemented in native C++ (squirrel_vm_bootstrap.cpp). */" + text[i+1:]
    raise AssertionError(("unbalanced", signature))

for signature in [
    "int32_t function_4a8c50(void)",
    "int32_t function_4a8c90(int32_t vm, const char *format, ...)",
    "int32_t function_4a8cc0(void)",
    "int32_t function_4a8db0(int32_t a1)",
    "int32_t function_4a90c0(int32_t * a1, int32_t * a2)",
    "int32_t function_4a9570(void)",
]:
    # Definitions occur after prototypes; find the last occurrence.
    pos = text.rfind(signature)
    assert pos >= 0, signature
    head, tail = text[:pos], text[pos:]
    text = head + remove_definition(tail, signature)

source.write_text(text)

bootstrap = repo / "src/squirrel/squirrel_vm_bootstrap.cpp"
assert not bootstrap.exists()
bootstrap.write_text(r'''#include "kinoko/squirrel_vm_bootstrap.h"
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
using kinoko::script::ObjectView;
using kinoko::script::address;
using kinoko::script::pointer;

HSQUIRRELVM current_vm() noexcept {
    return reinterpret_cast<HSQUIRRELVM>(g644);
}

int32_t function_address(void* pointer_value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(pointer_value));
}
}

extern "C" int32_t function_4a8c50(void) {
    if (g645 != 0) {
        function_4a9570_this(g645);
        std::free(pointer<void>(g645));
        g645 = 0;
    }
    if (g642 == 0)
        function_4a9570_this(address(unk_5149EC));
    g644 = nullptr;
    return 0;
}

extern "C" int32_t function_4a8c90(int32_t /*vm*/, const char* format, ...) {
    // The original VM print callback formats one line and writes it. The
    // recovered host had accumulated diagnostic prefixes/trace routing here;
    // keep the observable print behavior without the extra debug noise.
    char message[4096]{};
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format ? format : "", arguments);
    va_end(arguments);
    return std::puts(message);
}

extern "C" int32_t function_4a8cc0(void) {
    if (g645 != 0)
        return g645;

    auto* vm = current_vm();
    if (!vm)
        return 0;

    sq_pushroottable(vm);
    const int32_t storage = _3f__3f_2_40_YAPAXI_40_Z(12);
    if (storage != 0)
        function_4a94e0_this(storage);
    g645 = storage;
    function_4a9660_this(storage, -1);
    sq_pop(vm, 1);
    return g645;
}

extern "C" int32_t function_4a8db0(int32_t requested_vm) {
    int32_t current = requested_vm;

    if (requested_vm != 0 && address(g644) == requested_vm)
        return 1;

    // Preserve the recovered ownership boundary exactly. The original has a
    // virtual cleanup here, but its receiver type is not yet recovered; do
    // not invent a destructor/free operation for this path.
    if (g645 != 0)
        g645 = 0;

    if (g642 == 0)
        function_4a9570_this(address(unk_5149EC));
    g644 = nullptr;

    if (requested_vm == 0) {
        current = function_48a170(1024);
        if (current == 0)
            return 0;

        const int32_t node = _3f__3f_2_40_YAPAXI_40_Z(8);
        if (node != 0) {
            *pointer<int32_t>(node) = kinoko_sq_shared_state(current);
            *pointer<int32_t>(node + 4) = g643;
            g643 = node;
        }

        function_48b8b0(current,
            function_address(reinterpret_cast<void*>(&function_4a8c90)));
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
    const int32_t owner_result =
        function_4a9e30_this(address(unk_5149EC), current);
    return (owner_result & -256) | 1;
}

extern "C" int32_t function_4a90c0(int32_t* object, int32_t* klass) {
    return function_4a90c0_this(address(object), address(klass));
}
''')

header = repo / "include/kinoko/squirrel_vm_bootstrap.h"
assert not header.exists()
header.write_text(r'''#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compatibility names retained at the decompiled host boundary. The
// implementation is source-backed C++ and owns no alternate resource path.
int32_t function_4a8c50(void);
int32_t function_4a8c90(int32_t vm, const char* format, ...);
int32_t function_4a8cc0(void);
int32_t function_4a8db0(int32_t vm);
int32_t function_4a90c0(int32_t* object, int32_t* klass);

#ifdef __cplusplus
}
#endif
''')

detail = repo / "include/kinoko/squirrel_binding_detail.hpp"
d = detail.read_text()
d = d.replace('#include "kinoko/squirrel_host_compat.h"\n',
              '#include "kinoko/squirrel_host_compat.h"\n#include "kinoko/squirrel_vm_bootstrap.h"\n', 1)
d = d.replace('extern "C" {\nextern char* g644;\nint32_t function_4a8cc0(void);\nint32_t function_4a8db0(int32_t vm);\n}\n',
              'extern "C" { extern char* g644; }\n', 1)
detail.write_text(d)

cm = repo / "CMakeLists.txt"
c = cm.read_text()
needle = "    src/squirrel/squirrel_host_compat.cpp\n"
assert c.count(needle) == 1
c = c.replace(needle, needle + "    src/squirrel/squirrel_vm_bootstrap.cpp\n", 1)
cm.write_text(c)

subprocess.run(["python3", "tools/check_migration_boundaries.py"], check=True)
Path(__file__).unlink()
