#include "kinoko/diagnostics.h"

#include <cstdio>
#include <cstring>
#include <dbghelp.h>

namespace {
SRWLOCK trace_lock = SRWLOCK_INIT;
HANDLE trace_file = INVALID_HANDLE_VALUE;
char trace_buffer[65536];
DWORD trace_used = 0;
bool trace_enabled = false;
bool dump_enabled = false;
volatile LONG dump_written = 0;

class TraceLock {
public:
    TraceLock() { AcquireSRWLockExclusive(&trace_lock); }
    ~TraceLock() { ReleaseSRWLockExclusive(&trace_lock); }
    TraceLock(const TraceLock &) = delete;
    TraceLock &operator=(const TraceLock &) = delete;
};

bool environment_switch(const char *name, bool fallback) {
    char value[8];
    const DWORD length = GetEnvironmentVariableA(name, value, sizeof(value));
    if (length == 1 && (value[0] == '0' || value[0] == '1'))
        return value[0] == '1';
    return fallback;
}

bool beside_executable(char (&path)[MAX_PATH], const char *name) {
    const DWORD size = GetModuleFileNameA(nullptr, path, sizeof(path));
    if (size == 0 || size >= sizeof(path))
        return false;
    char *separator = std::strrchr(path, '\\');
    if (!separator)
        return false;
    const size_t prefix = separator + 1 - path;
    if (prefix + std::strlen(name) + 1 > sizeof(path))
        return false;
    std::strcpy(separator + 1, name);
    return true;
}

bool starts_with(const char *message, const char *prefix) {
    return std::strncmp(message, prefix, std::strlen(prefix)) == 0;
}

bool selected_message(const char *message) {
#if defined(RETDEC_TRACE_STAR_FILTER)
    const char *prefixes[] = {"actor:star", "actor:invalid", "actor:update-failed",
        "stagevm:failure", "map:path", "veh:", "seh:"};
#elif defined(RETDEC_TRACE_FILTER)
    const char *prefixes[] = {"seh:", "veh:", "4011b0:", "4017b0:",
        "render-target:", "map:", "mcd:", "draw:", "act:", "actor:",
        "actor-create:", "actor-update:", "actor-manager:", "native-471df0:",
        "native-userdata:", "c2d:", "4525d0:", "40d790:", "408b30:",
        "411d80:", "40b520:", "4701e0:", "470220:", "470290:", "470300:",
        "470320:", "470360:", "470980:", "game:", "scene:", "stagevm:",
        "savedata:", "prepcall-beginstage-", "call-initstage-", "bgm:",
        "audio:", "input:"};
#else
    (void)message;
    return true;
#endif
#if defined(RETDEC_TRACE_STAR_FILTER) || defined(RETDEC_TRACE_FILTER)
    for (const char *prefix : prefixes)
        if (starts_with(message, prefix))
            return true;
    return false;
#endif
}

void flush_locked() {
    DWORD offset = 0;
    while (offset < trace_used) {
        DWORD written = 0;
        if (!WriteFile(trace_file, trace_buffer + offset,
                trace_used - offset, &written, nullptr) || written == 0)
            break;
        offset += written;
    }
    trace_used -= offset;
    std::memmove(trace_buffer, trace_buffer + offset, trace_used);
}

void write_dump(EXCEPTION_POINTERS *exception) {
    if (!dump_enabled || !exception || InterlockedExchange(&dump_written, 1))
        return;
    char path[MAX_PATH];
    if (!beside_executable(path, "retdec_crash.dmp"))
        return;
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception, FALSE};
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
        MiniDumpWithFullMemory, &info, nullptr, nullptr);
    CloseHandle(file);
}

LONG WINAPI unhandled_exception(EXCEPTION_POINTERS *exception) {
    kinoko_report_exception(exception);
    return EXCEPTION_CONTINUE_SEARCH;
}
}

extern "C" void kinoko_diagnostics_initialize() {
#if defined(RETDEC_DISABLE_TRACE) || defined(RETDEC_TRACE_OUTPUT_DISABLED)
    constexpr bool default_trace = false;
#else
    constexpr bool default_trace = true;
#endif
    trace_enabled = environment_switch("KINOKO_TRACE", default_trace);
    dump_enabled = environment_switch("KINOKO_CRASH_DUMP", false);
    if (trace_enabled || dump_enabled)
        SetUnhandledExceptionFilter(unhandled_exception);
}

extern "C" void kinoko_diagnostics_shutdown() {
    TraceLock lock;
    if (trace_file != INVALID_HANDLE_VALUE) {
        flush_locked();
        CloseHandle(trace_file);
        trace_file = INVALID_HANDLE_VALUE;
    }
}

// This remains an out-of-line sink in quiet builds. Removing VM call sites
// changes the stack shape of the remaining reconstructed C code.
extern "C" __declspec(noinline) void retdec_trace(const char *message) {
    if (!trace_enabled || !message || !selected_message(message))
        return;
    const size_t length = std::strlen(message);
    TraceLock lock;
    if (trace_file == INVALID_HANDLE_VALUE) {
        char path[MAX_PATH];
        if (!beside_executable(path, "retdec_trace.log"))
            return;
        trace_file = CreateFileA(path, FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (trace_file == INVALID_HANDLE_VALUE)
            return;
    }
    if (length + 2 > sizeof(trace_buffer) - trace_used)
        flush_locked();
    if (length + 2 <= sizeof(trace_buffer) - trace_used) {
        std::memcpy(trace_buffer + trace_used, message, length);
        trace_used += static_cast<DWORD>(length);
        std::memcpy(trace_buffer + trace_used, "\r\n", 2);
        trace_used += 2;
    } else {
        DWORD written;
        WriteFile(trace_file, message, static_cast<DWORD>(length), &written, nullptr);
        WriteFile(trace_file, "\r\n", 2, &written, nullptr);
    }
    if (starts_with(message, "game:frame") || starts_with(message, "actor:star") ||
        starts_with(message, "stagevm:failure") || starts_with(message, "seh:") ||
        starts_with(message, "veh:"))
        flush_locked();
}

extern "C" void retdec_trace_hresult(const char *label, long value) {
    if (!label)
        return;
    char message[128];
    std::snprintf(message, sizeof(message), "%s:0x%08lX", label,
        static_cast<unsigned long>(value));
    retdec_trace(message);
}

extern "C" int kinoko_report_exception(EXCEPTION_POINTERS *exception) {
    if (exception && exception->ExceptionRecord && exception->ContextRecord) {
        char message[256];
        const auto &context = *exception->ContextRecord;
        std::snprintf(message, sizeof(message),
            "seh:code=0x%08lX addr=%p eip=0x%08lX esp=0x%08lX ebp=0x%08lX",
            exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress,
            context.Eip, context.Esp, context.Ebp);
        retdec_trace(message);
        write_dump(exception);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
