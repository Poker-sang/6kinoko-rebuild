// Offline oracle: execute original PE code in a separate, fixed-base test host.
// Only WinMain is redirected to the probe; game/VM functions remain unchanged.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "kinoko/squirrel_compile_bridge.h"

extern "C" void retdec_trace(const char *message) { std::fprintf(stderr, "%s\n", message); }

template <typename T> T original(uintptr_t address) { return reinterpret_cast<T>(address); }
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
        float,float,float,int32_t,int32_t,int32_t)>(0x463b40)(
        reinterpret_cast<void *>(0x5143e0),initializer[0],initializer[1],initializer[2],
        100,160,-1,argument[0],argument[1],argument[2]);
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
    vm = *reinterpret_cast<int32_t *>(0x5149dc);
    std::printf("original vm=%08x actor-manager-vtable=%08x\n",vm,
        *reinterpret_cast<uint32_t *>(0x5143e0)); std::fflush(stdout);
    original<int32_t(__cdecl *)()>(0x460e00)();
    original<int32_t(__thiscall *)(void *)>(0x463af0)(reinterpret_cast<void *>(0x5143e0));
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
    ExitProcess(ok ? 0 : 1);
}
int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr,"usage: original_vm_oracle original.exe probe.nut\n"); return 2;
    }
    std::ifstream source_file(argv[2],std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(source_file)),{});
    if (!retdec_squirrel_compile_source(source.data(),static_cast<int32_t>(source.size()),
        "original oracle",&probe_code,&probe_size)) return 3;
    std::ifstream image_file(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(image_file)),{});
    if (bytes.size()<sizeof(IMAGE_DOS_HEADER)) return 4;
    auto *dos=reinterpret_cast<IMAGE_DOS_HEADER *>(bytes.data());
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 ||
        static_cast<size_t>(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size()) return 4;
    auto *pe=reinterpret_cast<IMAGE_NT_HEADERS32 *>(bytes.data()+dos->e_lfanew);
    if (pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
        pe->OptionalHeader.ImageBase!=0x400000 || pe->OptionalHeader.AddressOfEntryPoint!=0xaca23)
        return 4;
    auto *mapped=static_cast<unsigned char *>(VirtualAlloc(reinterpret_cast<void *>(0x400000),
        pe->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if (!mapped) { std::fprintf(stderr,"original image base unavailable: %lu\n",GetLastError()); return 5; }
    std::memcpy(mapped,bytes.data(),pe->OptionalHeader.SizeOfHeaders);
    auto *sections=IMAGE_FIRST_SECTION(pe);
    for (unsigned i=0;i<pe->FileHeader.NumberOfSections;++i) {
        const auto &section=sections[i];
        if (static_cast<uint64_t>(section.PointerToRawData)+section.SizeOfRawData>bytes.size() ||
            static_cast<uint64_t>(section.VirtualAddress)+section.SizeOfRawData>pe->OptionalHeader.SizeOfImage)
            return 4;
        std::memcpy(mapped+section.VirtualAddress,bytes.data()+section.PointerToRawData,section.SizeOfRawData);
    }
    auto *imports=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(mapped+
        pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (;imports->Name;++imports) {
        HMODULE module=LoadLibraryA(reinterpret_cast<char *>(mapped+imports->Name));
        if (!module) { std::fprintf(stderr,"missing import %s\n",mapped+imports->Name); return 6; }
        auto *lookup=reinterpret_cast<IMAGE_THUNK_DATA32 *>(mapped+
            (imports->OriginalFirstThunk ? imports->OriginalFirstThunk : imports->FirstThunk));
        auto *iat=reinterpret_cast<IMAGE_THUNK_DATA32 *>(mapped+imports->FirstThunk);
        for (;lookup->u1.AddressOfData;++lookup,++iat) {
            LPCSTR name=IMAGE_SNAP_BY_ORDINAL32(lookup->u1.Ordinal)
                ? MAKEINTRESOURCEA(IMAGE_ORDINAL32(lookup->u1.Ordinal))
                : reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(mapped+lookup->u1.AddressOfData)->Name;
            auto function=GetProcAddress(module,name);
            if (!function) return 6;
            iat->u1.Function=reinterpret_cast<uint32_t>(function);
        }
    }
    auto *entry=mapped+0x73b30;
    entry[0]=0xe9;
    const auto displacement=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&probe_main)-
        reinterpret_cast<uintptr_t>(entry+5));
    std::memcpy(entry+1,&displacement,4);
    FlushInstructionCache(GetCurrentProcess(),mapped,pe->OptionalHeader.SizeOfImage);
    original<void(__cdecl *)()>(0x4aca23)();
    return 7;
}
