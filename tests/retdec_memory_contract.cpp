#include "kinoko/retdec_memory.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void contracts() {
    std::array<unsigned char, 128> source{}, expected{}, actual{};
    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<unsigned char>(i * 7 + 3);
    for (int offset = 0; offset < 4; ++offset) {
        for (int count = 1; count <= 12; ++count) {
            expected.fill(0xcd); actual = expected;
            std::memcpy(expected.data() + offset, source.data() + 3, count);
            require(__asm_rep_movsb_memcpy(actual.data() + offset, source.data() + 3, count) == 0,
                "MOVSB return value");
            require(actual == expected, "MOVSB bytes and canaries");
            expected.fill(0xcd); actual = expected;
            std::memcpy(expected.data() + offset, source.data() + 1, 4 * count);
            require(__asm_rep_movsd_memcpy(actual.data() + offset, source.data() + 1, count) == 0,
                "MOVSD return value");
            require(actual == expected, "MOVSD DWORD count and unaligned addresses");
            expected.fill(0xcd); actual = expected;
            std::memset(expected.data() + offset, 0x1234, count);
            require(__asm_rep_stosb_memset(actual.data() + offset, 0x1234, count) == 0,
                "STOSB return value");
            require(actual == expected, "STOSB low byte and canaries");
            for (int32_t value : {int32_t{0x12345678}, int32_t{-1}, int32_t{0}}) {
                expected.fill(0xcd); actual = expected;
                for (int i = 0; i < count; ++i)
                    std::memcpy(expected.data() + offset + i * 4, &value, sizeof(value));
                require(__asm_rep_stosd_memset(actual.data() + offset, value, count) == 0,
                    "STOSD return value");
                require(actual == expected, "STOSD complete words, unaligned writes and canaries");
            }
        }
    }
    for (int32_t count : {0, -1, std::numeric_limits<int32_t>::min()}) {
        require(__asm_rep_movsb_memcpy(nullptr, nullptr, count) == 0, "empty MOVSB");
        require(__asm_rep_movsd_memcpy(nullptr, nullptr, count) == 0, "empty MOVSD");
        require(__asm_rep_stosb_memset(nullptr, 42, count) == 0, "empty STOSB");
        require(__asm_rep_stosd_memset(nullptr, 42, count) == 0, "empty STOSD");
    }
    for (int i = 0; i < 1024; ++i)
        __frontend_reg_store_fpr(i, static_cast<long double>(i) + 0.25L);
    for (int i = 0; i < 1024; ++i) {
        require(__frontend_reg_load_fpr(i) == i + 0.25L, "FPR slots are independent");
        require(__frontend_reg_load_fpr(i - 1024) == i + 0.25L, "FPR unsigned negative-index mapping");
        require(__frontend_reg_load_fpr(i + 1024) == i + 0.25L, "FPR positive-index wrap");
    }
    __frontend_reg_store_fpr(std::numeric_limits<int32_t>::min(), -3.5L);
    require(__frontend_reg_load_fpr(0) == -3.5L, "INT_MIN maps to FPR zero");
}
}
int main() {
    try {
        contracts();
        std::puts("PASS: recovered memory operations and FPR scratch-bank contracts");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Memory contract failed: %s\n", error.what());
        return 1;
    }
}
