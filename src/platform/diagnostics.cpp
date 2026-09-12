#include "kinoko/diagnostics.h"

#include <cstdio>
#include <cstdlib>
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
volatile LONG first_fault_written = 0;
PVOID first_fault_handler = nullptr;
HANDLE fault_file = INVALID_HANDLE_VALUE;
char capture_stem[96] = {};

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

void write_fault_record(const char *phase, EXCEPTION_POINTERS *exception) {
    if (fault_file == INVALID_HANDLE_VALUE || !exception ||
        !exception->ExceptionRecord || !exception->ContextRecord)
        return;
    const auto &record = *exception->ExceptionRecord;
    const auto &context = *exception->ContextRecord;
    char message[768];
    const int length = std::snprintf(message, sizeof(message),
        "%s pid=%lu tid=%lu code=%08lX flags=%08lX address=%p module=%p "
        "eip=%08lX esp=%08lX ebp=%08lX eax=%08lX ebx=%08lX ecx=%08lX "
        "edx=%08lX esi=%08lX edi=%08lX access=%lu target=%08lX\r\n",
        phase, GetCurrentProcessId(), GetCurrentThreadId(), record.ExceptionCode,
        record.ExceptionFlags, record.ExceptionAddress, GetModuleHandleA(nullptr),
        context.Eip, context.Esp, context.Ebp, context.Eax, context.Ebx, context.Ecx,
        context.Edx, context.Esi, context.Edi,
        record.NumberParameters > 0 ? record.ExceptionInformation[0] : 0,
        record.NumberParameters > 1 ? record.ExceptionInformation[1] : 0);
    if (length > 0 && length < static_cast<int>(sizeof(message))) {
        DWORD written;
        WriteFile(fault_file, message, static_cast<DWORD>(length), &written, nullptr);
        FlushFileBuffers(fault_file);
    }
}

void write_dump(EXCEPTION_POINTERS *exception, bool first_chance = false) {
    if (!dump_enabled || !exception ||
        (!first_chance && InterlockedExchange(&dump_written, 1)))
        return;
    char path[MAX_PATH];
    char name[128];
    std::snprintf(name, sizeof(name), "%s-%s.dmp", capture_stem,
        first_chance ? "first" : "unhandled");
    if (!beside_executable(path, name))
        return;
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception, FALSE};
    const BOOL saved = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
        MiniDumpWithFullMemory, &info, nullptr, nullptr);
    const DWORD error = saved ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (fault_file != INVALID_HANDLE_VALUE) {
        char message[192];
        const int length = std::snprintf(message, sizeof(message),
            "dump=%s saved=%d error=%lu\r\n", name, saved ? 1 : 0, error);
        DWORD written;
        WriteFile(fault_file, message, static_cast<DWORD>(length), &written, nullptr);
        FlushFileBuffers(fault_file);
    }
}

LONG WINAPI first_chance_exception(EXCEPTION_POINTERS *exception) {
    if (!exception || !exception->ExceptionRecord || !exception->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = exception->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    if (!InterlockedCompareExchange(&first_fault_written, 1, 0)) {
        // No trace lock: the fault may have interrupted the trace sink itself.
        write_fault_record("first-chance", exception);
        if (code != EXCEPTION_STACK_OVERFLOW)
            write_dump(exception, true);
    }
    return EXCEPTION_CONTINUE_SEARCH;
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
#if defined(RETDEC_CAPTURE_FIRST_CHANCE)
    constexpr bool default_capture = true;
#else
    constexpr bool default_capture = false;
#endif
    const bool capture = environment_switch("KINOKO_CAPTURE_FIRST_CHANCE", default_capture);
    trace_enabled = environment_switch("KINOKO_TRACE", default_trace);
    dump_enabled = environment_switch("KINOKO_CRASH_DUMP", capture);
    if (capture || dump_enabled) {
        SYSTEMTIME now;
        GetSystemTime(&now);
        std::snprintf(capture_stem, sizeof(capture_stem),
            "fault-%04u%02u%02u-%02u%02u%02u-%03u-p%lu",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            now.wMilliseconds, GetCurrentProcessId());
        char name[128], path[MAX_PATH];
        std::snprintf(name, sizeof(name), "%s.log", capture_stem);
        if (beside_executable(path, name))
            fault_file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (capture)
            first_fault_handler = AddVectoredExceptionHandler(1, first_chance_exception);
        if (fault_file != INVALID_HANDLE_VALUE) {
            char message[192];
            const int length = std::snprintf(message, sizeof(message),
                "capture-start pid=%lu module=%p first_chance=%d dumps=%d handler=%p\r\n",
                GetCurrentProcessId(), GetModuleHandleA(nullptr), capture ? 1 : 0,
                dump_enabled ? 1 : 0, first_fault_handler);
            DWORD written;
            WriteFile(fault_file, message, static_cast<DWORD>(length), &written, nullptr);
            FlushFileBuffers(fault_file);
        }
    }
    std::atexit(kinoko_diagnostics_shutdown);
    if (trace_enabled || dump_enabled)
        SetUnhandledExceptionFilter(unhandled_exception);
}

extern "C" void kinoko_diagnostics_shutdown() {
    if (first_fault_handler) {
        RemoveVectoredExceptionHandler(first_fault_handler);
        first_fault_handler = nullptr;
    }
    if (fault_file != INVALID_HANDLE_VALUE) {
        const char message[] = "diagnostics-shutdown\r\n";
        DWORD written;
        WriteFile(fault_file, message, sizeof(message) - 1, &written, nullptr);
        CloseHandle(fault_file);
        fault_file = INVALID_HANDLE_VALUE;
    }
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
        write_fault_record("unhandled", exception);
        write_dump(exception);
        char message[256];
        const auto &context = *exception->ContextRecord;
        std::snprintf(message, sizeof(message),
            "seh:code=0x%08lX addr=%p eip=0x%08lX esp=0x%08lX ebp=0x%08lX",
            exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress,
            context.Eip, context.Esp, context.Ebp);
        retdec_trace(message);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
