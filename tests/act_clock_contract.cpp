// Actual typed clock/EndStage implementation with only the clock and unrelated
// legacy container ports and virtual document callbacks controlled. Raw fixtures are independent of
// the production schema offsets; no original game assets are synthesized.
#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include "kinoko/act_resource.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/legacy_memory.hpp"
static DWORD test_clock;
static DWORD WINAPI clock_now() { return test_clock; }
#define timeGetTime clock_now
#include "../src/reconstructed/act_resource.cpp"
#undef timeGetTime

namespace test {
int32_t cleared_slot;
void *source_receiver,*active_receiver;
int callback_count;
std::array<int,3> callbacks{};
int32_t __fastcall suspend_document(void *document,void *) {
    callbacks[callback_count++]=document==active_receiver?1:2;
    return document==source_receiver?17:91;
}
int32_t __fastcall resume_document(void *document,void *) {
    callbacks[callback_count++]=document==active_receiver?3:99;return 18;
}
int clears, command_clears;
int32_t command_runtime;
void put(void *p, uint32_t value) { std::memcpy(p, &value, sizeof(value)); }
uint32_t word(const void *p) { uint32_t value; std::memcpy(&value, p, sizeof(value)); return value; }
}
extern "C" {
void kinoko_act_commands_clear(int32_t runtime) { test::command_runtime = runtime; ++test::command_clears; }
void retdec_trace_i32(const char *, int32_t) {}
int32_t retdec_act_clear_layout_vector(int32_t slot) { test::cleared_slot = slot; ++test::clears; return 0; }
}
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr,"clock line %d: %s\n",__LINE__,#condition); return 1; } } while (0)
static_assert(std::is_same_v<decltype(kinoko::act::RuntimeRecord::active_document), KinokoActDocument *>);
static_assert(std::is_same_v<decltype(kinoko::act::RuntimeRecord::active_holder), KinokoActSourceHolder *>);
static_assert(std::is_same_v<decltype(kinoko::act::RuntimeRecord::vm), SQVM *>);

int main() {
    using namespace test;
    // Guarded, deliberately unaligned runtime for scalar operations.
    std::array<unsigned char, 194> bytes;
    bytes.fill(0xa5);
    auto *raw = bytes.data() + 1;
    auto *runtime = reinterpret_cast<KinokoActRuntime *>(raw);
    int32_t source[2] = {0, 10};
    auto *document = reinterpret_cast<KinokoActDocument *>(source);
    auto *holder = reinterpret_cast<KinokoActSourceHolder *>(&document);
    kinoko::legacy::store(raw, holder);
    auto expected = bytes;
    CHECK(kinoko_act_set_current_time(runtime, nullptr, 37) == 0);
    put(expected.data()+5, 37);
    CHECK(bytes == expected);
    CHECK(kinoko_act_get_current_time(runtime, nullptr) == 37);
    CHECK(kinoko_act_get_current_frame(runtime, nullptr) == 3);
    CHECK(kinoko_act_increment_frame(runtime, nullptr) == 0);
    CHECK(word(raw+4) == 47);
    kinoko_act_set_current_time(runtime, nullptr, -31);
    CHECK(kinoko_act_get_current_frame(runtime, nullptr) == -3);
    put(raw+4, 0x7fffffffu); source[1] = 1;
    kinoko_act_increment_frame(runtime, nullptr);
    CHECK(word(raw+4) == 0x80000000u);
    put(raw+4, 0xffffffffu);
    kinoko_act_increment_frame(runtime, nullptr);
    CHECK(word(raw+4) == 0);
    source[1] = 0;
    CHECK(kinoko_act_get_current_frame(runtime, nullptr) == 0);
    document = nullptr; put(raw+4, 123);
    kinoko_act_increment_frame(runtime, nullptr);
    CHECK(word(raw+4) == 123);
    test_clock = 0xfffffff0u;
    CHECK(kinoko_act_sleep_to(runtime, nullptr, 32) == 16);
    CHECK(word(raw+100) == 16);
    CHECK(kinoko_act_sleep_to(nullptr, nullptr, -1) == static_cast<int32_t>(0xffffffefu));
    CHECK(kinoko_act_set_current_time(nullptr, nullptr, 55) == 0);
    CHECK(kinoko_act_get_current_time(nullptr, nullptr) == 0);
    CHECK(kinoko_act_increment_frame(nullptr, nullptr) == 0);
    void *methods[9]{};
    methods[7]=reinterpret_cast<void*>(suspend_document);
    methods[8]=reinterpret_cast<void*>(resume_document);
    void *active[1]={methods};
    put(source,static_cast<uint32_t>(reinterpret_cast<uintptr_t>(methods)));
    document=reinterpret_cast<KinokoActDocument*>(source);
    source_receiver=document;active_receiver=active;
    kinoko::legacy::store(raw+12,reinterpret_cast<KinokoActDocument*>(active));
    CHECK(kinoko_act_suspend(runtime, nullptr) == 17 && raw[104]==1);
    CHECK(kinoko_act_resume(runtime, nullptr) == 18);
    CHECK((callback_count==3 && callbacks==std::array<int,3>{1,2,3}));
    CHECK(word(raw+100)==0 && raw[104]==0 && raw[105]==0xa5);
    CHECK(bytes.front() == 0xa5 && bytes.back() == 0xa5);

    // Real Win32 lock on aligned storage. Preserve every byte outside the
    // documented EndStage fields; mock the separate sprite container boundary.
    alignas(CRITICAL_SECTION) unsigned char aligned[192];
    std::memset(aligned, 0xa5, sizeof(aligned));
    runtime = reinterpret_cast<KinokoActRuntime *>(aligned);
    auto *lock = reinterpret_cast<CRITICAL_SECTION *>(aligned+20);
    InitializeCriticalSection(lock);
    aligned[8] = 1; put(aligned+44, 0x1234); put(aligned+48, 0x5678);
    CHECK(kinoko_act_end_stage(runtime, nullptr) == 0);
    CHECK(aligned[8] == 0 && aligned[9] == 0xa5);
    for (int i=108; i<152; ++i) CHECK(aligned[i] == 0);
    CHECK(command_clears == 1 && command_runtime == kinoko::legacy::address(runtime));
    CHECK(word(aligned+48) == 0x5678);
    CHECK(word(aligned+52) == 0xa5a5a5a5u);
    CHECK(clears == 1 && cleared_slot == kinoko::legacy::address(aligned+60));
    CHECK(lock->RecursionCount == 0);
    CHECK(kinoko_act_end_stage(runtime, nullptr) == E_FAIL && clears == 1);
    CHECK(kinoko_act_end_stage(nullptr, nullptr) == E_FAIL);
    DeleteCriticalSection(lock);
    std::puts("PASS: typed ACT clock, DWORD wrap, source borrow, guarded records and EndStage fields");
}
