#include "kinoko/legacy_frame_copy.h"
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error The legacy frame-copy adapter is supported only by MSVC Win32.
#endif
// CMake appends /Oy- and /GL- for THIS translation unit only. Never apply
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
// /Oy- alone is insufficient for a zero-argument, register-only function:
// MSVC can still emit no frame at all. Disable optimization at the two tiny
// ABI entries and materialize a volatile stack slot; the scan stays optimized.
#pragma optimize("", off)
extern "C" __declspec(noinline) uintptr_t kinoko_legacy_caller_ebp(void) {
    RtlCaptureContext(&saved_context);
    volatile uintptr_t captured_frame = saved_context.Ebp;
    return read_frame_word(captured_frame);
}
#pragma optimize("", on)


#pragma optimize("", off)
extern "C" __declspec(noinline) int32_t _memcpy2(void) {
    // The helper returns OUR frame register. Its saved word is exactly the
    // incoming caller EBP that the former naked entry forwarded to the scan.
    volatile uintptr_t frame = kinoko_legacy_caller_ebp();
    return kinoko_legacy_copy_from_frame(read_frame_word(frame));
}
#pragma optimize("", on)
