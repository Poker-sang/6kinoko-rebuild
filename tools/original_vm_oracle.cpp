// Offline oracle DLL, called by the debugger host before original WinMain.
// All original game and VM functions execute without patches.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "kinoko/squirrel_compile_bridge.h"

extern "C" void retdec_trace(const char *message) { std::fprintf(stderr, "%s\n", message); }

static uintptr_t location(uintptr_t address) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + address - 0x400000;
}
template <typename T> T original(uintptr_t address) { return reinterpret_cast<T>(location(address)); }
static unsigned char *probe_code;
static int32_t probe_size;
static int32_t vm;
static int32_t actor;
static int32_t __cdecl create_probe(int32_t machine) {
    int32_t initializer[3], argument[3];
    original<void(__thiscall *)(void *)>(0x4a94e0)(initializer);
    original<void(__thiscall *)(void *,int32_t)>(0x4a9660)(initializer,2);
    original<void(__thiscall *)(void *)>(0x4a94e0)(argument);
    actor=original<int32_t(__thiscall *)(void *,int32_t,int32_t,int32_t,
        float,float,float,int32_t,int32_t,int32_t,int32_t)>(0x463b40)(
        reinterpret_cast<void *>(location(0x5143e0)),initializer[0],initializer[1],initializer[2],
        100,160,-1,argument[0],argument[1],argument[2],0);
    std::printf("original actor=%08x instance=%08x\n",actor,
        *reinterpret_cast<uint32_t *>(actor+52)); std::fflush(stdout);
    original<void(__cdecl *)(int32_t,int32_t,int32_t)>(0x48ab90)(machine,
        *reinterpret_cast<int32_t *>(actor+48),*reinterpret_cast<int32_t *>(actor+52));
    return 1;
}
struct Stream { unsigned char *cursor; int32_t remaining; };
static int32_t read_stream(void *opaque, void *output, int32_t count) {
    auto &stream = *static_cast<Stream *>(opaque);
    if (count > stream.remaining) return -1;
    std::memcpy(output, stream.cursor, count);
    stream.cursor += count; stream.remaining -= count;
    return count;
}
static bool execute(unsigned char *code, int32_t size) {
    const auto top = original<int32_t(__cdecl *)(int32_t)>(0x48aa20)(vm);
    Stream stream{code, size};
    int32_t result = original<int32_t(__cdecl *)(int32_t, decltype(&read_stream), void *)>(
        0x48b050)(vm, read_stream, &stream);
    if (result >= 0) {
        original<void(__cdecl *)(int32_t)>(0x48a670)(vm);
        result = original<int32_t(__cdecl *)(int32_t,int32_t,int32_t,int32_t)>(
            0x48ace0)(vm,1,0,1);
    }
    std::printf("original script result=%d\n",result);
    if (result < 0) {
        auto *words = reinterpret_cast<uint32_t *>(vm);
        if (words[16] == 0x08000010)
            std::printf("original script error=%s\n",reinterpret_cast<char *>(words[17]+28));
    }
    original<void(__cdecl *)(int32_t,int32_t)>(0x48c910)(vm,top);
    std::fflush(stdout);
    return result >= 0;
}
static int WINAPI probe_main(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::puts("original CRT initialized; game WinMain not executed"); std::fflush(stdout);
    original<int32_t(__cdecl *)(int32_t)>(0x4a8db0)(0);
    vm = *reinterpret_cast<int32_t *>(location(0x5149dc));
    std::printf("original vm=%08x actor-manager-vtable=%08x\n",vm,
        *reinterpret_cast<uint32_t *>(location(0x5143e0))); std::fflush(stdout);
    original<int32_t(__cdecl *)()>(0x460e00)();
    original<int32_t(__thiscall *)(void *)>(0x463af0)(reinterpret_cast<void *>(location(0x5143e0)));
    original<void(__cdecl *)(int32_t)>(0x48a670)(vm);
    original<void(__cdecl *)(int32_t,const char *,int32_t)>(0x48a480)(vm,"CreateProbe",-1);
    original<void(__cdecl *)(int32_t,decltype(&create_probe),int32_t)>(0x48d850)(vm,create_probe,0);
    original<int32_t(__cdecl *)(int32_t,int32_t,int32_t)>(0x48c950)(vm,-3,0);
    original<void(__cdecl *)(int32_t)>(0x48aa50)(vm);
    const bool ok = execute(probe_code,probe_size);
    if(actor) std::printf("original final instance=%08x callback=%08x\n",
        *reinterpret_cast<uint32_t *>(actor+52),*reinterpret_cast<uint32_t *>(actor+112));
    std::fflush(stdout);
    // No game has started; do not run unrelated game shutdown through the CRT.
    return ok ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunOriginalProbe(void *argument) {
    HMODULE self=nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&RunOriginalProbe),&self);
    char path[MAX_PATH]; GetModuleFileNameA(self,path,MAX_PATH);
    strcat_s(path,".log");
    FILE *out=nullptr; freopen_s(&out,path,"w",stdout);
    std::ifstream source_file(static_cast<const char *>(argument),std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(source_file)),{});
    if (!retdec_squirrel_compile_source(source.data(),static_cast<int32_t>(source.size()),
        "original oracle",&probe_code,&probe_size)) return 3;
    return probe_main(nullptr,nullptr,nullptr,0);
}
