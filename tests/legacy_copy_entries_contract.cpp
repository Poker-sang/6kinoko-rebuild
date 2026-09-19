#include "kinoko/legacy_copy_entries.h"
#include <cstdint>
#include <cstdio>

namespace { int32_t observed_receiver, observed_source; constexpr int32_t answer = 0x12344321; }
extern "C" int32_t function_43d110_this(int32_t receiver, int32_t source) {
    observed_receiver = receiver; observed_source = source; return answer;
}
int main() {
    using Copy = int32_t(__thiscall*)(int32_t, int32_t);
    for (int i = 0; i < 10000; ++i) {
        const volatile uint32_t guard = 0xa5b6c7d8;
        const auto receiver = static_cast<int32_t>(0xfffffff0u + static_cast<uint32_t>(i));
        const int32_t source = i ^ 0x71231234;
        const Copy entries[] = {reinterpret_cast<Copy>(function_43d110),
            reinterpret_cast<Copy>(function_43cf20), reinterpret_cast<Copy>(function_43e860)};
        for (int entry = 0; entry < 3; ++entry) {
            const auto adjusted = static_cast<int32_t>(static_cast<uint32_t>(receiver) + (entry ? 4u : 0u));
            if (entries[entry](receiver, source) != answer || observed_receiver != adjusted || observed_source != source || guard != 0xa5b6c7d8)
                return 1;
        }
    }
    std::puts("PASS: 30000 real thiscall copies/adjustors, unsigned receiver wrapping, /RTC1 stack cleanup");
    return 0;
}
