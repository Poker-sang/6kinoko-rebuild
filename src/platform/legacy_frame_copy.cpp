#include "kinoko/legacy_frame_copy.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" int retdec_valid_range(const void* address, size_t size, int writeable);

// Isolated from the register-ABI entry so its heuristic can be tested without
// reading arbitrary compiler-generated stack temporaries. New code must pass
// explicit operands; this is not a recovered memcpy implementation.
extern "C" __declspec(noinline) int32_t kinoko_legacy_copy_from_frame(uintptr_t caller_frame) {
    if (!caller_frame) return 0;
    int best_score = -1;
    void* destination = nullptr;
    const void* source = nullptr;
    size_t size = 0;
    // This deliberately preserves the pre-existing heuristic, including its
    // scan limits, size bound, scoring, zero-byte rule and first-wins ties.
    // It does NOT claim that the unresolved RetDec operands are reconstructed.
    for (int offset = -0x800; offset <= 0x100; offset += 4) {
        const auto slot = caller_frame + static_cast<uintptr_t>(offset);
        if (!retdec_valid_range(reinterpret_cast<const void*>(slot), 12, 0)) continue;
        uint32_t words[3];
        std::memcpy(words, reinterpret_cast<const void*>(slot), sizeof words);
        const auto destination_value = static_cast<uintptr_t>(words[0]);
        const auto source_value = static_cast<uintptr_t>(words[1]);
        const size_t count = words[2];
        if (!destination_value || !source_value || count > 0x04000000u) continue;
        const auto* candidate_source = reinterpret_cast<const void*>(source_value);
        auto* candidate_destination = reinterpret_cast<void*>(destination_value);
        if (!retdec_valid_range(candidate_source, count ? count : 1, 0) ||
            !retdec_valid_range(candidate_destination, count ? count : 1, 1)) continue;
        int score = 1000 - std::abs(offset + 0x14);
        if (count <= 0x1000u) score += 30;
        if (destination_value == source_value) score -= 10;
        if (score > best_score) {
            best_score = score;
            destination = candidate_destination;
            source = candidate_source;
            size = count;
        }
    }
    if (!destination) return 0;
    if (size) std::memmove(destination, source, size);
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(destination));
}
