#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h>

#ifndef STATUS_HEAP_CORRUPTION
#define STATUS_HEAP_CORRUPTION ((DWORD)0xC0000374L)
#endif

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

static volatile LONG retdec_crash_dump_written;

static void retdec_write_crash_dump(EXCEPTION_POINTERS *exception_pointers)
{
    char dump_path[MAX_PATH];
    HANDLE dump_file;
    MINIDUMP_EXCEPTION_INFORMATION exception_info;

    if (exception_pointers == NULL ||
        InterlockedCompareExchange(&retdec_crash_dump_written, 1, 0) != 0) {
        return;
    }
    if (GetModuleFileNameA(NULL, dump_path, sizeof(dump_path)) == 0) {
        return;
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
        return;
    }
    exception_info.ThreadId = GetCurrentThreadId();
    exception_info.ExceptionPointers = exception_pointers;
    exception_info.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump_file,
                      MiniDumpWithFullMemory, &exception_info, NULL, NULL);
    CloseHandle(dump_file);
}

static LONG WINAPI retdec_vectored_exception_handler(EXCEPTION_POINTERS *exception_pointers)
{
    EXCEPTION_RECORD *record;
    CONTEXT *context;
    char message[768];
    ULONG_PTR fault_address;
    ULONG_PTR module_base;
    ULONG_PTR eip_rva;
    ULONG_PTR ret_rva;
    ULONG_PTR previous_frame;
    ULONG_PTR return_address;
    ULONG_PTR stack_words[4];

    if (exception_pointers == NULL || exception_pointers->ExceptionRecord == NULL) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    record = exception_pointers->ExceptionRecord;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION &&
        record->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION &&
        record->ExceptionCode != EXCEPTION_STACK_OVERFLOW &&
        record->ExceptionCode != STATUS_STACK_BUFFER_OVERRUN &&
        record->ExceptionCode != STATUS_HEAP_CORRUPTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    context = exception_pointers->ContextRecord;
    fault_address = 0;
    module_base = (ULONG_PTR)(uintptr_t)GetModuleHandleA(NULL);
    eip_rva = 0;
    ret_rva = 0;
    previous_frame = 0;
    return_address = 0;
    stack_words[0] = 0;
    stack_words[1] = 0;
    stack_words[2] = 0;
    stack_words[3] = 0;
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        record->NumberParameters >= 2) {
        fault_address = record->ExceptionInformation[1];
    }
    if (context != NULL) {
        if (module_base != 0 && context->Eip >= module_base) {
            eip_rva = context->Eip - module_base;
        }
        __try {
            previous_frame = *(ULONG_PTR *)(uintptr_t)context->Ebp;
            return_address = *(ULONG_PTR *)(uintptr_t)(context->Ebp + 4);
            stack_words[0] = *(ULONG_PTR *)(uintptr_t)context->Esp;
            stack_words[1] = *(ULONG_PTR *)(uintptr_t)(context->Esp + 4);
            stack_words[2] = *(ULONG_PTR *)(uintptr_t)(context->Esp + 8);
            stack_words[3] = *(ULONG_PTR *)(uintptr_t)(context->Esp + 12);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            previous_frame = 0;
            return_address = 0;
            stack_words[0] = 0;
            stack_words[1] = 0;
            stack_words[2] = 0;
            stack_words[3] = 0;
        }
        if (module_base != 0 && return_address >= module_base) {
            ret_rva = return_address - module_base;
        }
    }
    wsprintfA(message,
              "veh:code=0x%08lX addr=0x%08lX fault=0x%08lX base=0x%08lX eip=0x%08lX(rva=0x%08lX) ret=0x%08lX(rva=0x%08lX) esp=0x%08lX ebp=0x%08lX prev=0x%08lX stack=0x%08lX,0x%08lX,0x%08lX,0x%08lX eax=0x%08lX ebx=0x%08lX ecx=0x%08lX edx=0x%08lX esi=0x%08lX edi=0x%08lX",
              (unsigned long)record->ExceptionCode,
              (unsigned long)(uintptr_t)record->ExceptionAddress,
              (unsigned long)fault_address,
              (unsigned long)module_base,
              context != NULL ? (unsigned long)context->Eip : 0,
              (unsigned long)eip_rva,
              (unsigned long)return_address,
              (unsigned long)ret_rva,
              context != NULL ? (unsigned long)context->Esp : 0,
              context != NULL ? (unsigned long)context->Ebp : 0,
              (unsigned long)previous_frame,
              (unsigned long)stack_words[0],
              (unsigned long)stack_words[1],
              (unsigned long)stack_words[2],
              (unsigned long)stack_words[3],
              context != NULL ? (unsigned long)context->Eax : 0,
              context != NULL ? (unsigned long)context->Ebx : 0,
              context != NULL ? (unsigned long)context->Ecx : 0,
              context != NULL ? (unsigned long)context->Edx : 0,
              context != NULL ? (unsigned long)context->Esi : 0,
              context != NULL ? (unsigned long)context->Edi : 0);
    retdec_trace(message);
    if (record->ExceptionCode == STATUS_HEAP_CORRUPTION) {
        retdec_write_crash_dump(exception_pointers);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI retdec_unhandled_exception_filter(EXCEPTION_POINTERS *exception_pointers)
{
    retdec_write_crash_dump(exception_pointers);
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
