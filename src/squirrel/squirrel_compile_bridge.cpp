#include "kinoko/squirrel_compile_bridge.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclosure.h"
#include "sqfuncproto.h"
#include "sqcompiler.h"

extern "C" void kinoko_trace(const char *message);

namespace {
SQInteger write_bytecode(SQUserPointer context, SQUserPointer data, SQInteger size) {
    auto &bytes = *static_cast<std::vector<unsigned char> *>(context);
    const auto *begin = static_cast<const unsigned char *>(data);
    bytes.insert(bytes.end(), begin, begin + size);
    return size;
}

void compiler_error(HSQUIRRELVM, const SQChar *error, const SQChar *source,
                    SQInteger line, SQInteger column) {
    char message[1024];
    std::snprintf(message, sizeof(message), "stagevm:compile-error %s:%d:%d %s",
                  source, line, column, error);
    kinoko_trace(message);
}
}

extern "C" int32_t kinoko_squirrel_compile_source(const char *source,
    int32_t length, const char *name, unsigned char **bytecode, int32_t *bytecode_size) {
    *bytecode = nullptr;
    *bytecode_size = 0;
    HSQUIRRELVM compiler = sq_open(128);
    if (!compiler)
        return 0;
    sq_setcompilererrorhandler(compiler, compiler_error);
    sq_enabledebuginfo(compiler, SQTrue);
    std::vector<unsigned char> bytes;
    bool ok = SQ_SUCCEEDED(sq_compilebuffer(compiler, source, length, name, SQTrue)) &&
              SQ_SUCCEEDED(sq_writeclosure(compiler, write_bytecode, &bytes));
    if (ok && !bytes.empty()) {
        *bytecode = static_cast<unsigned char *>(std::malloc(bytes.size()));
        ok = *bytecode != nullptr;
        if (ok) {
            std::memcpy(*bytecode, bytes.data(), bytes.size());
            *bytecode_size = static_cast<int32_t>(bytes.size());
        }
    }
    sq_close(compiler);
    return ok ? 1 : 0;
}


namespace {
static_assert(offsetof(SQSharedState, _compilererrorhandler) == 160);
int32_t source_table_vtable;
template<class T> T *pointer(int32_t value) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
int32_t address(const void *value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
void recognize_compiler_tables(SQVM *vm) {
    if (!source_table_vtable) {
        SQObjectPtr table(SQTable::Create(_ss(vm), 0));
        std::memcpy(&source_table_vtable, _table(table), sizeof source_table_vtable);
    }
}
struct SourceBuffer { const char *text; SQInteger offset, length; };
SQInteger read_source(SQUserPointer context) {
    auto &buffer = *static_cast<SourceBuffer *>(context);
    return buffer.length < buffer.offset + 1 ? 0 : buffer.text[buffer.offset++];
}
}
extern "C" int32_t kinoko_sq_source_table_vtable(void) { return source_table_vtable; }

// Compile in the current VM, preserving constants, debug info and errors.
extern "C" int32_t kinoko_sq_compile_proto(int32_t vm, int32_t reader, int32_t context,
    const char *name, int32_t out[2], int32_t raiseerror, int32_t lineinfo) {
    auto *v = pointer<SQVM>(vm);
    recognize_compiler_tables(v);
    return Compile(v, reinterpret_cast<SQLEXREADFUNC>(pointer<void>(reader)),
        pointer<void>(context), name, *reinterpret_cast<SQObjectPtr *>(out),
        raiseerror != 0, lineinfo != 0);
}
extern "C" int32_t kinoko_sq_compile_reader(int32_t vm, int32_t reader, int32_t context,
    const char *name, int32_t raiseerror) {
    auto *v = pointer<SQVM>(vm);
    recognize_compiler_tables(v);
    return sq_compile(v, reinterpret_cast<SQLEXREADFUNC>(pointer<void>(reader)),
                      pointer<void>(context), name, raiseerror != 0);
}
extern "C" int32_t kinoko_sq_compile_buffer(int32_t vm, const char *text, int32_t length,
    const char *name, int32_t raiseerror) {
    return sq_compilebuffer(pointer<SQVM>(vm), text, length, name, raiseerror != 0);
}
// 4A1B90 returns -1 on compile failure, +1 when the new closure is on the stack.
extern "C" int32_t kinoko_sq_compilestring(int32_t vm) {
    auto *v = pointer<SQVM>(vm);
    const SQChar *source = nullptr, *name = _SC("unnamedbuffer");
    sq_getstring(v, 2, &source);
    if (sq_gettop(v) > 2) sq_getstring(v, 3, &name);
    return SQ_SUCCEEDED(kinoko_sq_compile_buffer(vm, source, sq_getsize(v, 2), name, SQFalse)) ? 1 : SQ_ERROR;
}
