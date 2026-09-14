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
#include "kinoko/actor_collision.h"
#include <cmath>
#include <float.h>

extern "C" void retdec_trace(const char *message) { std::fprintf(stderr, "%s\n", message); }

static uintptr_t location(uintptr_t address) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + address - 0x400000;
}
template <typename T> T original(uintptr_t address) { return reinterpret_cast<T>(location(address)); }

static bool collision_mode;
template<class T> T &slot(void *p, int offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(p)+offset);
}
static int probe_collision() {
    using Move = int32_t(__thiscall *)(void *,void *,float *,int32_t *,float *,float *,float *,float *,float *);
    auto move=original<Move>(0x4689d0);
    unsigned int control=0; _controlfp_s(&control,0,0);
    std::printf("collision original control=%08x\n",control);
    alignas(8) unsigned char a[640]={}, b[640]={}, platform[640]={}, chip[48]={};
    float layout[8]={}; KinokoCollisionRecord record{chip,layout,0}, scratch[8]{};
    uintptr_t state[40]={}, candidates[]={reinterpret_cast<uintptr_t>(platform)};
    state[9]=reinterpret_cast<uintptr_t>(scratch); state[10]=state[11]=state[9]+sizeof(scratch);
    state[17]=reinterpret_cast<uintptr_t>(candidates); state[21]=1;
    slot<int32_t>(platform,312)=2;
    slot<float>(platform,440)=1952; slot<float>(platform,448)=2208;
    slot<KinokoCollisionRecord>(platform,328)=record;
    slot<int16_t>(chip,12)=256; slot<int16_t>(chip,14)=32;
    layout[3]=1952;
    int differences=0, original_misses=0, rebuilt_misses=0;
    float previous=48;
    for(int frame=1;frame<=300;++frame) {
        const float angle=static_cast<float>(static_cast<float>(frame/300.0f)*3.14159274f);
        const float offset=static_cast<float>(std::cos(static_cast<double>(angle)))*48.0f;
        layout[4]=464+offset;
        // Identical input to each solver, already standing on previous surface.
        std::memset(a,0,sizeof(a));
        slot<int32_t>(a,316)=2;
        slot<float>(a,240)=2000; slot<float>(a,244)=(464+previous)-1;
        slot<float>(a,440)=1989.5f; slot<float>(a,448)=2010.5f;
        slot<float>(a,444)=slot<float>(a,244)-30; slot<float>(a,452)=slot<float>(a,244)+1;
        slot<float>(a,268)=offset-previous;
        std::memcpy(b,a,sizeof(a));
        float dx=0,dy=offset-previous;
        move(state,a,reinterpret_cast<float *>(a+440),reinterpret_cast<int32_t *>(a+284),
            reinterpret_cast<float *>(a+240),reinterpret_cast<float *>(a+244),&dx,&dy,
            reinterpret_cast<float *>(a+276));
        kinoko_actor_collision_move(b,&record,1,0,offset-previous);
        const int ah=slot<int32_t>(a,296), bh=slot<int32_t>(b,296);
        if(!ah) ++original_misses; if(!bh) ++rebuilt_misses;
        if(ah!=bh || slot<float>(a,244)!=slot<float>(b,244)) {
            ++differences;
            std::printf("DIFF frame=%d offset=%.9g floor=%.9g original=(%.9g,%d) rebuilt=(%.9g,%d)\n",
                frame,offset,layout[4],slot<float>(a,244),ah,slot<float>(b,244),bh);
        }
        previous=offset;
    }
    std::printf("collision differences=%d original_misses=%d rebuilt_misses=%d\n",differences,original_misses,rebuilt_misses);
    std::fflush(stdout);
    return differences?1:0;
}

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
    if(collision_mode) return probe_collision();
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
    collision_mode=source.find("collision oracle")!=std::string::npos;
    if (!retdec_squirrel_compile_source(source.data(),static_cast<int32_t>(source.size()),
        "original oracle",&probe_code,&probe_size)) return 3;
    return probe_main(nullptr,nullptr,nullptr,0);
}
