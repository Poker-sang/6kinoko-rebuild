#include "kinoko/diagnostics.h"
#include <dbghelp.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

static bool handled_probe(bool final_report) {
    __try {
        if (final_report)
            RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION, 0, 0, nullptr);
        else {
            ULONG_PTR parameters[2] = {1, 0x12345678};
            RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 2, parameters);
        }
    } __except (final_report ? kinoko_report_exception(GetExceptionInformation()) :
                               EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
    return false;
}

static bool find_artifact(const std::string &directory, const char *suffix, std::string &path) {
    char pattern[128];
    std::snprintf(pattern, sizeof(pattern), "fault-*-p%lu%s", GetCurrentProcessId(), suffix);
    WIN32_FIND_DATAA data;
    HANDLE search = FindFirstFileA((directory + pattern).c_str(), &data);
    if (search == INVALID_HANDLE_VALUE)
        return false;
    path = directory + data.cFileName;
    const bool unique = !FindNextFileA(search, &data);
    FindClose(search);
    return unique;
}

static bool validate_dump(const std::string &path, DWORD code) {
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    void *base = mapping ? MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0) : nullptr;
    bool valid = false;
    if (base) {
        PMINIDUMP_DIRECTORY entry = nullptr;
        void *stream = nullptr;
        ULONG size = 0;
        if (MiniDumpReadDumpStream(base, ExceptionStream, &entry, &stream, &size) &&
            size >= sizeof(MINIDUMP_EXCEPTION_STREAM)) {
            const auto &exception = *static_cast<MINIDUMP_EXCEPTION_STREAM *>(stream);
            valid = exception.ThreadId == GetCurrentThreadId() &&
                exception.ExceptionRecord.ExceptionCode == code;
            if (code == EXCEPTION_ACCESS_VIOLATION)
                valid = valid && exception.ExceptionRecord.NumberParameters == 2 &&
                    exception.ExceptionRecord.ExceptionInformation[0] == 1 &&
                    exception.ExceptionRecord.ExceptionInformation[1] == 0x12345678;
        }
        UnmapViewOfFile(base);
    }
    if (mapping) CloseHandle(mapping);
    CloseHandle(file);
    return valid;
}

int main() {
    SetEnvironmentVariableA("KINOKO_TRACE", "0");
    SetEnvironmentVariableA("KINOKO_CAPTURE_FIRST_CHANCE", "1");
    SetEnvironmentVariableA("KINOKO_CRASH_DUMP", "1");
    kinoko_diagnostics_initialize();
    if (!handled_probe(false) || !handled_probe(false) || !handled_probe(true))
        return 1;
    kinoko_diagnostics_shutdown();
    char executable[MAX_PATH];
    GetModuleFileNameA(nullptr, executable, MAX_PATH);
    const std::string directory = std::string(executable).substr(0,
        std::string(executable).find_last_of('\\') + 1);
    std::string first, final, log;
    if (!find_artifact(directory, "-first.dmp", first) ||
        !find_artifact(directory, "-unhandled.dmp", final) ||
        !find_artifact(directory, ".log", log) ||
        !validate_dump(first, EXCEPTION_ACCESS_VIOLATION) ||
        !validate_dump(final, EXCEPTION_ILLEGAL_INSTRUCTION))
        return 2;
    std::ifstream input(log);
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    if (text.find("first-chance") == std::string::npos ||
        text.find("target=12345678") == std::string::npos ||
        text.find("unhandled") == std::string::npos ||
        text.find("diagnostics-shutdown") == std::string::npos)
        return 3;
    std::puts("PASS: first-fault capture preserves handling, records context, and keeps separate valid dumps");
    return 0;
}
