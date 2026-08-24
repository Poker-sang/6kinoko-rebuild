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

static int retdec_capture_exception(EXCEPTION_POINTERS *exception_pointers)
{
    char message[256];
    EXCEPTION_RECORD *record;
    CONTEXT *context;

    if (exception_pointers == NULL || exception_pointers->ExceptionRecord == NULL) {
        retdec_trace("seh:unknown");
        return EXCEPTION_EXECUTE_HANDLER;
    }
    record = exception_pointers->ExceptionRecord;
    context = exception_pointers->ContextRecord;
    wsprintfA(message,
              "seh:code=0x%08lX addr=0x%08lX eip=0x%08lX esp=0x%08lX ebp=0x%08lX",
              (unsigned long)record->ExceptionCode,
              (unsigned long)(uintptr_t)record->ExceptionAddress,
              context != NULL ? (unsigned long)context->Eip : 0,
              context != NULL ? (unsigned long)context->Esp : 0,
              context != NULL ? (unsigned long)context->Ebp : 0);
    retdec_trace(message);
    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI retdec_vectored_exception_handler(EXCEPTION_POINTERS *exception_pointers)
{
    EXCEPTION_RECORD *record;
    CONTEXT *context;
    char message[384];
    ULONG_PTR fault_address;
    ULONG_PTR return_address;
    ULONG_PTR stack_word;

    if (exception_pointers == NULL || exception_pointers->ExceptionRecord == NULL) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    record = exception_pointers->ExceptionRecord;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION &&
        record->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION &&
        record->ExceptionCode != EXCEPTION_STACK_OVERFLOW &&
        record->ExceptionCode != STATUS_STACK_BUFFER_OVERRUN) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    context = exception_pointers->ContextRecord;
    fault_address = 0;
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        record->NumberParameters >= 2) {
        fault_address = record->ExceptionInformation[1];
    }
    return_address = 0;
    stack_word = 0;
    if (context != NULL) {
        __try {
            return_address = *(ULONG_PTR *)(uintptr_t)(context->Ebp + 4);
            stack_word = *(ULONG_PTR *)(uintptr_t)context->Esp;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return_address = 0;
            stack_word = 0;
        }
    }
    wsprintfA(message,
              "veh:code=0x%08lX addr=0x%08lX fault=0x%08lX eip=0x%08lX ret=0x%08lX esp=0x%08lX ebp=0x%08lX stack=0x%08lX eax=0x%08lX ebx=0x%08lX ecx=0x%08lX edx=0x%08lX esi=0x%08lX edi=0x%08lX",
              (unsigned long)record->ExceptionCode,
              (unsigned long)(uintptr_t)record->ExceptionAddress,
              (unsigned long)fault_address,
              context != NULL ? (unsigned long)context->Eip : 0,
              (unsigned long)return_address,
              context != NULL ? (unsigned long)context->Esp : 0,
              context != NULL ? (unsigned long)context->Ebp : 0,
              (unsigned long)stack_word,
              context != NULL ? (unsigned long)context->Eax : 0,
              context != NULL ? (unsigned long)context->Ebx : 0,
              context != NULL ? (unsigned long)context->Ecx : 0,
              context != NULL ? (unsigned long)context->Edx : 0,
              context != NULL ? (unsigned long)context->Esi : 0,
              context != NULL ? (unsigned long)context->Edi : 0);
    retdec_trace(message);
    return EXCEPTION_CONTINUE_SEARCH;
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
    int result;

    AddVectoredExceptionHandler(1, retdec_vectored_exception_handler);
    SetUnhandledExceptionFilter(retdec_unhandled_exception_filter);
    (void)previous;
    (void)command_line;
    (void)show_command;
    __try {
        result = (int)_WinMain_40_16((int32_t)(uintptr_t)instance,
                                     (int32_t)(uintptr_t)previous,
                                     (int32_t)(uintptr_t)command_line,
                                     show_command);
    } __except (retdec_capture_exception(GetExceptionInformation())) {
        result = -1;
    }
    return result;
}
