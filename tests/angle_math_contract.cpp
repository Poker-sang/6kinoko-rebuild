#include "kinoko/angle_math.h"
#include <cmath>
#include <initializer_list>
#include <cstdio>
#include <float.h>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"angle line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main() {
    CHECK(kinoko_cos_degrees(0) == 1 && kinoko_cos_degrees(90) == 0);
    CHECK(kinoko_cos_degrees(180) == -1 && kinoko_cos_degrees(270) == 0);
    CHECK(kinoko_sin_degrees(0) == 0 && kinoko_sin_degrees(90) == 1);
    CHECK(kinoko_sin_degrees(180) == 0 && kinoko_sin_degrees(270) == -1);
    CHECK(kinoko_sin_degrees(-90) == -1 && kinoko_cos_degrees(-180) == -1);
    CHECK(kinoko_cos_degrees(60.04f) == kinoko_cos_degrees(60));
    CHECK(kinoko_cos_degrees(60.06f) == kinoko_cos_degrees(60.1f));
    CHECK(kinoko_sin_degrees(.04f) == 0 && kinoko_sin_degrees(.06f) > 0);
    CHECK(kinoko_sin_degrees(-.04f) == 0 && kinoko_sin_degrees(-.06f) < 0);
    CHECK(kinoko_cos_degrees(23.01f) == kinoko_cos_degrees(23.04f));
    for (int i = -7200; i <= 7200; ++i) {
        const float angle = static_cast<float>(i) / 10.0f;
        const double radians = static_cast<double>(i) / 10.0 * 3.1415926535 / 180.0;
        CHECK(std::fabs(kinoko_cos_degrees(angle) - std::cos(radians)) < 0.000001);
        CHECK(std::fabs(kinoko_sin_degrees(angle) - std::sin(radians)) < 0.000001);
    }
    unsigned previous = 0, current = 0;
    _controlfp_s(&previous, 0, 0);
    for (unsigned mode : {_RC_NEAR, _RC_UP, _RC_DOWN, _RC_CHOP}) {
        _controlfp_s(&current, mode, _MCW_RC);
        CHECK(kinoko_cos_degrees(60.04f) == kinoko_cos_degrees(60));
        CHECK(kinoko_cos_degrees(60.06f) == kinoko_cos_degrees(60.1f));
        CHECK(kinoko_sin_degrees(450) == 1 && kinoko_sin_degrees(-450) == -1);
        _controlfp_s(&current, 0, 0);
        CHECK((current & _MCW_RC) == mode);
    }
    _controlfp_s(&current, previous, _MCW_RC);
    std::puts("PASS: tenth-degree quantization, quadrants, wrapping and rounding-mode preservation");
}
