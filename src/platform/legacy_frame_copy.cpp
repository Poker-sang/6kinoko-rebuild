#include "kinoko/legacy_frame_copy.h"
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error The legacy frame-copy adapter is supported only by MSVC Win32.
#endif
// CMake also appends /Oy- and /GL- for THIS translation unit only. Never apply
// that setting to the callers: their incoming EBP register must not be changed.
#pragma optimize("y", off)
extern "C" int retdec_valid_range(const void* address, size_t size, int writeable);

namespace {
// Keep the 716-byte context OUT of the legacy [-0x800,+0x100] stack scan. The
// buffer has trivial TLS initialization and no cross-thread shared state.
__declspec(thread) CONTEXT saved_context;

uintptr_t read_frame_word(uintptr_t frame) noexcept {
    if (!retdec_valid_range(reinterpret_cast<const void*>(frame), sizeof(uintptr_t), 0))
        return 0;
    uintptr_t value = 0;
    std::memcpy(&value, reinterpret_cast<const void*>(frame), sizeof value);
    return value;
}
}

// Return the caller's EBP register value, not the address of a return address.
// The compiler owns this function's prologue. With /Oy- its saved frame word is
// the incoming EBP, including when the caller itself omits frame pointers.
extern "C" __declspec(noinline) uintptr_t kinoko_legacy_caller_ebp(void) {
    RtlCaptureContext(&saved_context);
    return read_frame_word(saved_context.Ebp);
}

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

extern "C" __declspec(noinline) int32_t _memcpy2(void) {
    // The helper returns OUR frame register. Its saved word is exactly the
    // incoming caller EBP that the former naked entry forwarded to the scan.
    const auto frame = kinoko_legacy_caller_ebp();
    return kinoko_legacy_copy_from_frame(read_frame_word(frame));
}
#pragma optimize("", on)
