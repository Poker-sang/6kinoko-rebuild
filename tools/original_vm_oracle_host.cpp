// Start the original PE under the debugger API, stop before WinMain, and run
// the offline oracle DLL while the original main thread remains suspended.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>

static bool write_memory(HANDLE process, void *address, const void *bytes, SIZE_T size) {
    SIZE_T written=0;
    return WriteProcessMemory(process,address,bytes,size,&written) && written==size;
}
static void *remote_string(HANDLE process, const char *text) {
    const SIZE_T size=std::strlen(text)+1;
    void *remote=VirtualAllocEx(process,nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    return remote && write_memory(process,remote,text,size) ? remote : nullptr;
}
static bool wait_thread(PROCESS_INFORMATION &process, HANDLE thread, DWORD &result) {
    const DWORD id=GetThreadId(thread);
    const ULONGLONG deadline=GetTickCount64()+60000;
    while(GetTickCount64()<deadline) {
        DEBUG_EVENT event{};
        if(!WaitForDebugEvent(&event,1000)) continue;
        DWORD status=DBG_CONTINUE;
        if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
            CloseHandle(event.u.LoadDll.hFile);
        if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            const auto code=event.u.Exception.ExceptionRecord.ExceptionCode;
            if(code!=EXCEPTION_BREAKPOINT) {
                std::fprintf(stderr,"oracle exception=%08lx at %p first=%lu\n",code,
                    event.u.Exception.ExceptionRecord.ExceptionAddress,event.u.Exception.dwFirstChance);
                status=DBG_EXCEPTION_NOT_HANDLED;
            }
        }
        const bool done=event.dwDebugEventCode==EXIT_THREAD_DEBUG_EVENT && event.dwThreadId==id;
        if(done) result=event.u.ExitThread.dwExitCode;
        const bool exited=event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT;
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
        if(done) return true;
        if(exited) return false;
    }
    return false;
}
int main(int argc,char **argv) {
    if(argc!=4) { std::fprintf(stderr,"usage: original_vm_oracle original.exe oracle.dll probe.nut\n"); return 2; }
    HMODULE local=LoadLibraryExA(argv[2],nullptr,DONT_RESOLVE_DLL_REFERENCES);
    if(!local) return 3;
    auto function=GetProcAddress(local,"_RunOriginalProbe@4");
    if(!function) return 3;
    const uintptr_t offset=reinterpret_cast<uintptr_t>(function)-reinterpret_cast<uintptr_t>(local);
    FreeLibrary(local);
    STARTUPINFOA startup{sizeof(startup)}; PROCESS_INFORMATION process{};
    std::string command='"'+std::string(argv[1])+'"';
    if(!CreateProcessA(argv[1],command.data(),nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS,
        nullptr,nullptr,&startup,&process)) return 4;
    unsigned char original_byte=0; void *entry=nullptr; bool ready=false;
    const ULONGLONG deadline=GetTickCount64()+60000;
    while(GetTickCount64()<deadline && !ready) {
        DEBUG_EVENT event{};
        if(!WaitForDebugEvent(&event,1000)) continue;
        DWORD status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            entry=static_cast<unsigned char *>(event.u.CreateProcessInfo.lpBaseOfImage)+0x73b30;
            SIZE_T read=0;
            ReadProcessMemory(process.hProcess,entry,&original_byte,1,&read);
            const unsigned char breakpoint=0xcc;
            if(read!=1 || !write_memory(process.hProcess,entry,&breakpoint,1)) break;
            FlushInstructionCache(process.hProcess,entry,1);
            if(event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        }
        if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
            CloseHandle(event.u.LoadDll.hFile);
        if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            if(event.u.Exception.ExceptionRecord.ExceptionAddress==entry) {
                CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
                GetThreadContext(process.hThread,&context);
                context.Eip=reinterpret_cast<DWORD>(entry);
                write_memory(process.hProcess,entry,&original_byte,1);
                FlushInstructionCache(process.hProcess,entry,1);
                SetThreadContext(process.hThread,&context);
                SuspendThread(process.hThread);
                ready=true;
            } else if(event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_BREAKPOINT)
                status=DBG_EXCEPTION_NOT_HANDLED;
        }
        const bool exited=event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT;
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
        if(exited) break;
    }
    DWORD result=5;
    if(ready) {
        void *dll_path=remote_string(process.hProcess,argv[2]);
        HANDLE loader=CreateRemoteThread(process.hProcess,nullptr,0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA")),
            dll_path,0,nullptr);
        DWORD module=0;
        if(loader && wait_thread(process,loader,module) && module) {
            void *script_path=remote_string(process.hProcess,argv[3]);
            HANDLE probe=CreateRemoteThread(process.hProcess,nullptr,0,
                reinterpret_cast<LPTHREAD_START_ROUTINE>(module+offset),script_path,0,nullptr);
            if(!probe || !wait_thread(process,probe,result)) result=6;
            if(probe) CloseHandle(probe);
        }
        if(loader) CloseHandle(loader);
    }
    TerminateProcess(process.hProcess,result);
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    std::printf("offline original oracle result=%lu\n",result);
    return static_cast<int>(result);
}
