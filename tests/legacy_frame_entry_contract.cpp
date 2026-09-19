#include "kinoko/legacy_frame_copy.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <thread>
#include <vector>

// Link the actual production entry but replace ONLY its scanner. This proves
// what EBP it forwards independently of the historical candidate heuristic.
// Placing a memcpy triple somewhere on a real stack does not establish that
// it has the highest score; compiler scratch, canaries and enclosing frames
// can all be valid competing triples (including zero-byte candidates).
namespace {
thread_local uintptr_t observed_frame;
constexpr int32_t sentinel = 0x12345678;
void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

__declspec(noinline) void check_entry() {
    auto* context = static_cast<CONTEXT*>(_aligned_malloc(sizeof(CONTEXT), 16));
    require(context != nullptr, "allocate independent context");
    const volatile uint32_t canary = 0x13572468;
    observed_frame = 0;
    // Keep the two external calls adjacent. With /Oy the compiler can reuse
    // EBP for a TLS base; accessing observed_frame BETWEEN the calls changes
    // the very register being sampled and invalidates the reference capture.
    RtlCaptureContext(context);
    const auto result = _memcpy2();
    const auto expected = context->Ebp;
    if (observed_frame != expected)
        std::fprintf(stderr, "entry EBP expected=%08x forwarded=%08x\n",
            expected, static_cast<unsigned>(observed_frame));
    require(observed_frame == expected, "actual zero-argument entry forwards incoming EBP");
    require(result == sentinel && canary == 0x13572468, "result and caller stack preserved");
    _aligned_free(context);
}

__declspec(noinline) void check_nested(unsigned depth) {
    volatile uint32_t separation[1024]{};
    if (depth) check_nested(depth - 1);
    else check_entry();
    require(separation[0] == 0 && separation[1023] == 0, "nested frame canaries");
}
}

extern "C" int32_t kinoko_legacy_copy_from_frame(uintptr_t frame) {
    observed_frame = frame;
    return sentinel;
}

int main() {
    for (int i = 0; i < 100; ++i) check_nested(3);
    for (int i = 0; i < 10000; ++i) check_entry();
    std::vector<std::thread> threads;
    for (int n = 0; n < 4; ++n) threads.emplace_back([] {
        for (int i = 0; i < 1000; ++i) check_nested(2);
    });
    for (auto& thread : threads) thread.join();
    std::puts("PASS: production zero-argument entry, forwarded register, nested frames, independent threads");
}
