#include "kinoko/game_math.h"
#include <float.h>
#ifdef KINOKO_MATH_AUDIT
#include <windows.h>
#include <cstdio>
#include <cstring>
static thread_local bool audit_active=false;
static thread_local unsigned audit_previous=0;
extern "C" void kinoko_math_checkpoint(const char *phase,uint32_t detail) {
    if(!audit_active)return;
    unsigned current=0;_controlfp_s(&current,0,0);
    if((current&_MCW_RC)==audit_previous)return;
    audit_active=false;
    char path[MAX_PATH];GetModuleFileNameA(nullptr,path,MAX_PATH);
    char *slash=std::strrchr(path,'\\');if(!slash)return;
    std::strcpy(slash+1,"fp-first-change.txt");
    FILE *file=nullptr;fopen_s(&file,path,"wx");if(!file)return;
    std::fprintf(file,"phase=%s detail=%08x previous=%08x current=%08x thread=%lu base=%p\n",phase,detail,audit_previous,current,GetCurrentThreadId(),GetModuleHandleA(nullptr));
    void *stack[32];auto count=CaptureStackBackTrace(0,32,stack,nullptr);
    for(USHORT i=0;i<count;++i)std::fprintf(file,"%p\n",stack[i]);
    std::fclose(file);
}
#endif

extern "C" uint32_t kinoko_enter_game_math() {
    unsigned int previous = 0, current = 0;
    _controlfp_s(&previous, 0, 0);
    _controlfp_s(&current, _RC_UP, _MCW_RC);
    return previous & _MCW_RC;
}

extern "C" void kinoko_leave_game_math(uint32_t previous_rounding) {
    unsigned int current = 0;
    _controlfp_s(&current, previous_rounding, _MCW_RC);
}

namespace {
class GameRoundingScope {
    const uint32_t previous = kinoko_enter_game_math();
public:
    ~GameRoundingScope() { kinoko_leave_game_math(previous); }
    GameRoundingScope() = default;
    GameRoundingScope(const GameRoundingScope &) = delete;
    GameRoundingScope &operator=(const GameRoundingScope &) = delete;
};
}

extern "C" unsigned long kinoko_run_game_math(unsigned long (__stdcall *update)(void *), void *argument) noexcept(false) {
    GameRoundingScope rounding;
#ifdef KINOKO_MATH_AUDIT
    audit_previous=_RC_UP; audit_active=true;
#endif
    return update(argument);
}
