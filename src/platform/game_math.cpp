#include "kinoko/game_math.h"
#include <float.h>

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
    return update(argument);
}
