#include <windows.h>
#include <dbghelp.h>

#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: capture_process_dump <pid> <output.dmp>\n");
        return 2;
    }
    DWORD pid = static_cast<DWORD>(std::strtoul(argv[1], nullptr, 0));
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
                                     PROCESS_DUP_HANDLE,
                                 FALSE, pid);
    if (process == nullptr) {
        std::fprintf(stderr, "OpenProcess failed: %lu\n", GetLastError());
        return 1;
    }
    HANDLE file = CreateFileA(argv[2], GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "CreateFile failed: %lu\n", GetLastError());
        CloseHandle(process);
        return 1;
    }
    BOOL ok = MiniDumpWriteDump(process, pid, file, MiniDumpWithFullMemory,
                                nullptr, nullptr, nullptr);
    DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    CloseHandle(process);
    if (!ok) {
        std::fprintf(stderr, "MiniDumpWriteDump failed: %lu\n", error);
        return 1;
    }
    return 0;
}
