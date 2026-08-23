#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h>

void retdec_trace(const char *message)
{
    char path[MAX_PATH];
    HANDLE file;
    DWORD written;
    size_t length;

    if (message == NULL || GetModuleFileNameA(NULL, path, sizeof(path)) == 0) {
        return;
    }
    {
        char *separator = strrchr(path, '\\');
        if (separator != NULL) {
            separator[1] = '\0';
        } else {
            path[0] = '\0';
        }
    }
    lstrcatA(path, "retdec_trace.log");
    file = CreateFileA(path, FILE_APPEND_DATA,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    length = strlen(message);
    WriteFile(file, message, (DWORD)length, &written, NULL);
    WriteFile(file, "\r\n", 2, &written, NULL);
    CloseHandle(file);
}

void retdec_trace_hresult(const char *label, long value)
{
    char message[128];

    if (label == NULL) {
        return;
    }
    wsprintfA(message, "%s:0x%08lX", label, (unsigned long)value);
    retdec_trace(message);
}

static LONG WINAPI retdec_unhandled_exception_filter(EXCEPTION_POINTERS *exception_pointers)
{
    char dump_path[MAX_PATH];
    HANDLE dump_file;
    MINIDUMP_EXCEPTION_INFORMATION exception_info;

    if (GetModuleFileNameA(NULL, dump_path, sizeof(dump_path)) == 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    {
        char *separator = strrchr(dump_path, '\\');
        if (separator != NULL) {
            separator[1] = '\0';
        } else {
            dump_path[0] = '\0';
        }
    }
    lstrcatA(dump_path, "retdec_crash.dmp");
    dump_file = CreateFileA(dump_path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump_file == INVALID_HANDLE_VALUE) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    exception_info.ThreadId = GetCurrentThreadId();
    exception_info.ExceptionPointers = exception_pointers;
    exception_info.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump_file,
                      MiniDumpWithFullMemory, &exception_info, NULL, NULL);
    CloseHandle(dump_file);
    return EXCEPTION_CONTINUE_SEARCH;
}

extern int32_t _WinMain_40_16(int32_t instance, int32_t previous,
                              int32_t command_line, int32_t show_command);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command)
{
    SetUnhandledExceptionFilter(retdec_unhandled_exception_filter);
    (void)previous;
    (void)command_line;
    (void)show_command;
    return (int)_WinMain_40_16((int32_t)(uintptr_t)instance,
                               (int32_t)(uintptr_t)previous,
                               (int32_t)(uintptr_t)command_line,
                               show_command);
}
