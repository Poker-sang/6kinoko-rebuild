#include "kinoko/squirrel_compile_bridge.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <squirrel.h>

extern "C" void retdec_trace(const char *message);

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
    retdec_trace(message);
}
}

extern "C" int32_t retdec_squirrel_compile_source(const char *source,
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
