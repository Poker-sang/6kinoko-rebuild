#include "kinoko/legacy_frame_copy.h"
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <thread>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
uintptr_t address(const void* p) { return reinterpret_cast<uintptr_t>(p); }

// This comparison also runs in a target compiled /O2 /Oy: the incoming EBP
// need not be a valid frame pointer. Only the adapter's own frames are fixed.
__declspec(noinline) void check_register() {
    auto context = static_cast<CONTEXT*>(_aligned_malloc(sizeof(CONTEXT), 16));
    require(context != nullptr, "context allocation");
    const volatile uint32_t canary = 0xa135246bu;
    RtlCaptureContext(context);
    const auto expected = context->Ebp;
    const auto actual = kinoko_legacy_caller_ebp();
    if (actual != expected) std::fprintf(stderr, "EBP expected=%08x actual=%08x\n", expected, static_cast<unsigned>(actual));
    require(actual == expected && canary == 0xa135246bu, "incoming EBP and stack preserved");
    _aligned_free(context);
}

struct Fixture {
    // The scan is completely inside an initialized object, not a real stack.
    std::array<uint32_t, 0x920 / 4> words{};
    uintptr_t frame() { return address(words.data()) + 0x800; }
    void candidate(int offset, void* to, const void* from, uint32_t count) {
        const uint32_t triple[]{static_cast<uint32_t>(address(to)), static_cast<uint32_t>(address(from)), count};
        std::memcpy(reinterpret_cast<void*>(frame() + static_cast<uintptr_t>(offset)), triple, sizeof triple);
    }
};
void check_selection() {
    Fixture f;
    std::array<unsigned char, 128> source{}, first{}, second{};
    for (unsigned i = 0; i < source.size(); ++i) source[i] = static_cast<unsigned char>(i + 1);
    require(kinoko_legacy_copy_from_frame(0) == 0, "null frame");
    require(kinoko_legacy_copy_from_frame(f.frame()) == 0, "no candidate");
    f.candidate(-0x14, first.data(), source.data(), 128);
    require(static_cast<uintptr_t>(kinoko_legacy_copy_from_frame(f.frame())) == address(first.data()), "copy return");
    require(first == source, "copied explicit fixture");
    f = {}; first.fill(0);
    f.candidate(-0x28, first.data(), source.data(), 32);
    f.candidate(0, second.data(), source.data(), 32); // Equal score: earlier wins.
    kinoko_legacy_copy_from_frame(f.frame());
    require(first[31] == source[31] && second[0] == 0, "first-wins equal score");
    f = {}; first.fill(0);
    f.candidate(-0x14, first.data(), source.data(), 0);
    require(static_cast<uintptr_t>(kinoko_legacy_copy_from_frame(f.frame())) == address(first.data()) && first[0] == 0,
        "zero size returns destination without writing");
    f = {}; f.candidate(-0x14, first.data(), source.data(), 0x04000001u);
    require(kinoko_legacy_copy_from_frame(f.frame()) == 0, "size bound");
    f = {}; f.candidate(-0x14, source.data()+1, source.data(), 64);
    kinoko_legacy_copy_from_frame(f.frame());
    require(source[1] == 1 && source[64] == 64, "overlapping memmove");
    f = {}; f.candidate(-0x14, nullptr, source.data(), 1);
    require(kinoko_legacy_copy_from_frame(f.frame()) == 0, "null destination");
    auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE));
    require(page != nullptr, "protected page fixture");
    DWORD old = 0;
    require(VirtualProtect(page, 4096, PAGE_READONLY, &old) != 0, "read-only fixture");
    f = {}; f.candidate(-0x14, page, source.data(), 1);
    require(kinoko_legacy_copy_from_frame(f.frame()) == 0, "read-only destination rejected");
    require(VirtualProtect(page, 4096, PAGE_NOACCESS, &old) != 0, "unreadable fixture");
    require(kinoko_legacy_copy_from_frame(address(page)+0x800) == 0, "unreadable scan skipped");
    VirtualFree(page, 0, MEM_RELEASE);
}
}
int main() {
    check_selection();
    check_register();
    for (int i = 0; i < 10000; ++i) check_register();
    std::vector<std::thread> threads;
    for (int n = 0; n < 4; ++n) threads.emplace_back([] {
        for (int i = 0; i < 1000; ++i) check_register();
    });
    for (auto& thread : threads) thread.join();
    std::puts("PASS: context-derived incoming EBP, independent TLS, deterministic legacy selection/overlap/permissions");
}
