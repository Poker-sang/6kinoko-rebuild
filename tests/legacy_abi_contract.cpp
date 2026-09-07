#include "kinoko/legacy_abi.h"

#include <cstdio>

namespace {
class Probe {
public:
    virtual void touch() { ++calls; }
    virtual void store(int32_t value) { stored = value; }
    virtual int32_t get() { return stored; }
    virtual int32_t one(int32_t a) { return stored + a; }
    virtual int32_t two(int32_t a, int32_t b) { return one(a) + 2 * b; }
    virtual int32_t three(int32_t a, int32_t b, int32_t c) {
        return two(a, b) + 3 * c;
    }
    virtual int32_t four(int32_t a, int32_t b, int32_t c, int32_t d) {
        return three(a, b, c) + 4 * d;
    }
    virtual int32_t draw(int32_t x, int32_t y, int32_t width, int32_t height,
        int32_t resource, int32_t sx, int32_t sy, int32_t blend, float alpha) {
        return stored + x + 2*y + 3*width + 4*height + 5*resource + 6*sx +
            7*sy + 8*blend + static_cast<int32_t>(16*alpha);
    }
    int32_t calls = 0;
    int32_t stored = 0;
};
}

int main() {
    Probe object;
    void **methods = *reinterpret_cast<void ***>(&object);
    for (int i = 0; i < 10000; ++i) {
        retdec_call_thiscall0(&object, methods[0]);
        retdec_call_thiscall1(&object, methods[1], 17);
        if (retdec_call_thiscall0_result(&object, methods[2]) != 17 ||
            retdec_call_thiscall1_result(&object, methods[3], -3) != 14 ||
            retdec_call_thiscall2_result(&object, methods[4], -3, 5) != 24 ||
            retdec_call_thiscall3_result(&object, methods[5], -3, 5, 7) != 45 ||
            retdec_call_thiscall4_result(&object, methods[6], -3, 5, 7, 11) != 89 ||
            kinoko_call_draw_method(&object, methods[7], 1, 2, 3, 4, 5, 6, 7, 8,
                0.75f) != 233 || object.calls != i + 1)
            return 1;
    }
    std::puts("PASS: x86 virtual receiver, argument order, results and stack cleanup");
}
