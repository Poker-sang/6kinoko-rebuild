#include "kinoko/script_file.h"
#include "kinoko/file_io.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_vm_bootstrap.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

extern "C" {
extern char* g644;
extern char* g767;
extern char g874;
extern int32_t g664, g722[3];
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
void* kinoko_sqplus_object_assign(void*, const void*);
int32_t kinoko_squirrel_object_vtable(void);
}

namespace {
using namespace kinoko::script;
SQRESULT invoke(HSQUIRRELVM vm, SQInteger count, SQBool result, SQBool errors) {
    return kinoko_sq_call(address(vm), count, result, errors);
}
SQInteger read_bytecode(SQUserPointer stream, SQUserPointer destination, SQInteger count) {
    return kinoko_script_read_memory(stream, destination, count);
}
void trace_error(HSQUIRRELVM vm) {
    // Diagnostics must not retain the error on the execution stack.
    StackTop restore(vm);
    sq_getlasterror(vm);
    const SQChar* message = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &message)) && message)
        retdec_trace_squirrel_name("402d40:error", address(message));
}
class Reference final {
public:
    Reference(HSQUIRRELVM vm, HSQOBJECT value, bool retain = false) : vm_(vm), value_(value) {
        if (retain) sq_addref(vm_, &value_);
    }
    ~Reference() { sq_release(vm_, &value_); }
    HSQOBJECT value() const { return value_; }
    Reference(const Reference&) = delete;
private:
    HSQUIRRELVM vm_;
    HSQOBJECT value_;
};
}

extern "C" void* kinoko_script_initialize_root() {
    retdec_trace("402aa0:begin");
    kinoko_sqplus_select_vm(nullptr);
    retdec_trace("402aa0:after-debug");
    retdec_trace("402aa0:before-4a8cc0");
    auto source = kinoko_sqplus_root_object();
    retdec_trace_i32("402aa0:source", address(source));
    retdec_trace("402aa0:after-4a8cc0");
    auto result = kinoko_sqplus_object_assign(g722, source);
    retdec_trace("402aa0:done");
    return result;
}
extern "C" void* kinoko_script_root() { return g722; }
extern "C" int32_t kinoko_script_close_vm() {
    // 402AC0 CALL 4A8C50; 402AC5 JMP 4A8C50: two calls in the original.
    kinoko_sqplus_release_vm_wrappers();
    return kinoko_sqplus_release_vm_wrappers();
}

extern "C" int32_t kinoko_script_load_file(const char* path, const void* environment) {
    retdec_trace("402d40:entry");
    retdec_trace_i32("402d40:archives", kinoko_archive_count);
    if (!path) return 0;
    retdec_trace_squirrel_name("402d40:file", address(path));
    std::string lookup(path);
    if (g874) {
        // Existing reconstruction bounds checks; valid original names end .nut.
        if (lookup.size() >= MAX_PATH || lookup.size() < 4) return 0;
        lookup.replace(lookup.size() - 4, 4, ".cv4");
    }
    KinokoArchiveReader* opened = nullptr;
    if (!kinoko_reader_open(&opened, lookup.c_str()) || !opened) {
        retdec_trace("402d40:reader-failed");
        return 0;
    }
    std::unique_ptr<KinokoArchiveReader, decltype(&kinoko_reader_close)> reader(opened, kinoko_reader_close);
    const auto size = kinoko_reader_size(opened);
    retdec_trace_i32("402d40:size", static_cast<int32_t>(size));
    if (!size || size > 64u * 1024u * 1024u) return 0;
    std::unique_ptr<unsigned char, decltype(&std::free)> bytes(
        static_cast<unsigned char*>(std::calloc(size + 1u, 1)), std::free);
    if (!bytes || !kinoko_reader_read_exact(opened, bytes.get(), size)) return 0;
    uint16_t tag = 0;
    if (size >= sizeof(tag)) std::memcpy(&tag, bytes.get(), sizeof(tag));
    HSQOBJECT scope;
    sq_resetobject(&scope);
    if (environment) scope = ObjectView(environment).value();
    if (tag == 0xFAFAu) {
        // Original Sqrat VM (51620C), distinct from the SqPlus current VM.
        auto vm = pointer<SQVM>(g664);
        if (!vm) return 0;
        KinokoScriptMemoryReader stream{bytes.get(), static_cast<int32_t>(size), bytes.get()};
        const auto loaded = sq_readclosure(vm, read_bytecode, &stream);
        retdec_trace_i32("402d40:compiled-result", loaded);
        if (SQ_SUCCEEDED(loaded)) {
            HSQOBJECT closure;
            sq_resetobject(&closure);
            sq_getstackobj(vm, -1, &closure);
            if (!sq_isnull(closure)) {
                // LocalScript borrows the loaded closure; leave its stack slot.
                sq_pushobject(vm, closure);
                if (!environment || sq_isnull(scope)) sq_pushroottable(vm);
                else sq_pushobject(vm, scope);
                const auto executed = invoke(vm, 1, SQFalse, SQTrue);
                retdec_trace_i32("402d40:execute-result", executed);
                if (SQ_FAILED(executed)) trace_error(vm);
            }
        }
    } else {
        retdec_trace("402d40:plain-script");
        auto vm = reinterpret_cast<HSQUIRRELVM>(g644);
        if (!vm) return 0;
        // Unlike LocalScript, an explicitly supplied null value stays null.
        // SqPlus compile/call failures throw SquirrelError, as in the original.
        upstream::sqplus_compile_and_run(vm, reinterpret_cast<const char*>(bytes.get()),
            path, environment ? &scope : nullptr, invoke);
    }
    return 1; // Opened bytecode files report success even when load/run fails.
}

extern "C" int32_t kinoko_script_compile_file_argument(int32_t path, int32_t,
    int32_t argument_vm, int32_t type, int32_t data, char owns_reference) {
    const auto value = borrowed_value(type, data);
    // Destruction order: SqPlus temporary first, incoming Sqrat argument last.
    HSQOBJECT incoming;
    sq_resetobject(&incoming);
    if (owns_reference) incoming = value;
    Reference argument(pointer<SQVM>(argument_vm), incoming);
    Reference temporary(reinterpret_cast<HSQUIRRELVM>(g644), value, true);
    ObjectStorage environment{static_cast<uint32_t>(kinoko_squirrel_object_vtable()), value};
    return static_cast<unsigned char>(kinoko_script_load_file(pointer<const char>(path), &environment));
}

extern "C" int32_t kinoko_script_show_call_stack() {
    retdec_trace("402af0:entry");
    auto vm = reinterpret_cast<HSQUIRRELVM>(g644);
    const auto root = ObjectView(kinoko_sqplus_root_object()).value();
    Reference files(vm, upstream::sqplus_get_value(vm, root, "debug_call_stack_file"));
    Reference names(vm, upstream::sqplus_get_value(vm, root, "debug_call_stack_name"));
    Reference lines(vm, upstream::sqplus_get_value(vm, root, "debug_call_stack_line"));
    std::string text;
    const auto count = upstream::sqplus_length(vm, names.value());
    if (count > 0) {
        text = "================\nCallStack\n";
        for (int i = 0; i < count; ++i) {
            const auto file = upstream::sqplus_get_string(vm, files.value(), i);
            const auto line = upstream::sqplus_get_integer(vm, lines.value(), i);
            const auto name = upstream::sqplus_get_string(vm, names.value(), i);
            // Grow the string instead of overflowing the original 1024-byte row.
            text += "Func="; text += name ? name : "";
            text += " : Line="; text += std::to_string(line);
            text += " : File="; text += file ? file : ""; text += '\n';
        }
    }
    text += "================\n";
    MessageBoxA(reinterpret_cast<HWND>(g767), text.c_str(), "CallStack", 0);
    return 0;
}
