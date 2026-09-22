#include "kinoko/angle_math.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace {
constexpr int table_size = 3600;
// 404180 builds this before the update thread changes its rounding mode.
std::array<float, table_size> make_cosine_table() {
    std::array<float, table_size> values{};
    const double tenths = 10.0, pi = 3.1415926535, half_turn = 180.0;
    for (int index = 0; index < table_size; ++index) {
        double radians;
        // Preserve the original x87 expression and its final double store.
        __asm {
            fild index
            fdiv tenths
            fmul pi
            fdiv half_turn
            fstp radians
        }
        values[index] = static_cast<float>(std::cos(radians));
    }
    values[0] = 1.0f;
    values[900] = 0.0f;
    values[1800] = -1.0f;
    values[2700] = 0.0f;
    return values;
}
const auto cosine_table = make_cosine_table();

float lookup(float degrees, float phase) {
    const float tenths = 10.0f;
    float scaled;
    // There is ONE float store after multiply/subtract in 4040D0.
    // SSE float multiply followed by subtract would add a rounding step.
    __asm {
        fld degrees
        fmul tenths
        fsub phase
        fstp scaled
    }
    // __ftol2_sse truncates after adding 0.5 in extended precision. Converting
    // float to double here preserves that addition without a float store.
    const auto index = static_cast<int32_t>(std::fabs(static_cast<double>(scaled)) + 0.5);
    return cosine_table[index % table_size];
}
}
extern "C" float kinoko_sin_degrees(float degrees) { return lookup(degrees, 900.0f); }
extern "C" float kinoko_cos_degrees(float degrees) { return lookup(degrees, 0.0f); }
