#include "kinoko/retdec_memory.h"

#include <array>
#include <cstddef>
#include <cstring>

namespace {
// Preserve the existing process-wide bank; making it thread_local would
// change the recovered ABI's behavior rather than merely refactor it.
std::array<long double, 1024> floating_registers{};
std::size_t register_index(int32_t reg) noexcept {
    return static_cast<uint32_t>(reg) % floating_registers.size();
}
}

extern "C" long double __frontend_reg_load_fpr(int32_t reg) {
    return floating_registers[register_index(reg)];
}
extern "C" void __frontend_reg_store_fpr(int32_t reg, long double value) {
    floating_registers[register_index(reg)] = value;
}
extern "C" int64_t __asm_rep_movsb_memcpy(
    void *destination, const void *source, int32_t count) {
    if (count > 0)
        std::memcpy(destination, source, static_cast<std::size_t>(count));
    return 0;
}
extern "C" int64_t __asm_rep_movsd_memcpy(
    void *destination, const void *source, int32_t count) {
    if (count > 0)
        std::memcpy(destination, source,
                    static_cast<std::size_t>(count) * sizeof(uint32_t));
    return 0;
}
extern "C" int64_t __asm_rep_stosb_memset(
    void *destination, int32_t value, int32_t count) {
    if (count > 0)
        std::memset(destination, value & 0xff, static_cast<std::size_t>(count));
    return 0;
}
extern "C" int64_t __asm_rep_stosd_memset(
    void *destination, int32_t value, int32_t count) {
    auto *output = static_cast<unsigned char *>(destination);
    const auto word = static_cast<uint32_t>(value);
    // x86 STOSD permits an unaligned destination. A uint32_t* store has
    // alignment/aliasing requirements that the original instruction did not.
    for (int32_t i = 0; i < count; ++i, output += sizeof word)
        std::memcpy(output, &word, sizeof word);
    return 0;
}
