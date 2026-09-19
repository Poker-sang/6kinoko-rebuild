# Reviewed PR4 batch: separate production ABI entry from deterministic scanner tests.
from pathlib import Path
r=Path('.')
p=r/'src/platform/legacy_frame_copy.cpp'; s=p.read_text()
a=s.index('extern "C" __declspec(noinline) int32_t kinoko_legacy_copy_from_frame')
b=s.index('\n#pragma optimize("", off)',a)
scanner=s[a:b]
entry=s[:a]+s[b:]
entry=entry.replace('// CMake also appends /Oy- and /GL- for THIS translation unit only.', '// CMake appends /Oy- and /GL- for THIS translation unit only.')
(r/'src/platform/legacy_frame_entry.cpp').write_text(entry)
p.write_text('''#include "kinoko/legacy_frame_copy.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" int retdec_valid_range(const void* address, size_t size, int writeable);

// Isolated from the register-ABI entry so its heuristic can be tested without
// reading arbitrary compiler-generated stack temporaries. New code must pass
// explicit operands; this is not a recovered memcpy implementation.
'''+scanner)
p=r/'tests/legacy_frame_copy_contract.cpp'; s=p.read_text()
a=s.index('#pragma optimize("y", off)'); b=s.index('void check_selection()',a)
s=s[:a]+s[b:]
s=s.replace('    for (int i = 0; i < 100; ++i) check_nested_entry(3);\n','')
s=s.replace('context-derived incoming EBP, caller stack, independent TLS, legacy selection/overlap/permissions', 'context-derived incoming EBP, independent TLS, deterministic legacy selection/overlap/permissions')
p.write_text(s)
(r/'tests/legacy_frame_entry_contract.cpp').write_text('''#include "kinoko/legacy_frame_copy.h"
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
    if (!condition) { std::fprintf(stderr, "FAIL: %s\\n", message); std::exit(1); }
}

__declspec(noinline) void check_entry() {
    auto* context = static_cast<CONTEXT*>(_aligned_malloc(sizeof(CONTEXT), 16));
    require(context != nullptr, "allocate independent context");
    const volatile uint32_t canary = 0x13572468;
    RtlCaptureContext(context);
    const auto expected = context->Ebp;
    observed_frame = 0;
    const auto result = _memcpy2();
    if (observed_frame != expected)
        std::fprintf(stderr, "entry EBP expected=%08x forwarded=%08x\\n",
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
''')
p=r/'CMakeLists.txt';s=p.read_text().replace('    src/platform/legacy_frame_copy.cpp\n','    src/platform/legacy_frame_copy.cpp\n    src/platform/legacy_frame_entry.cpp\n')
s=s.replace('set_source_files_properties(src/platform/legacy_frame_copy.cpp','set_source_files_properties(src/platform/legacy_frame_entry.cpp')
a=s.index('# Both frame-pointer policies');b=s.index('\nset(KINOKO_TOOL_TARGETS',a)
s=s[:a]+'''# Test actual register forwarding separately from deterministic candidate selection.
# The entry contract supplies a scanner spy; the copy contract links the real one.
foreach(policy IN ITEMS checked optimized)
    foreach(boundary IN ITEMS copy entry)
        set(contract kinoko_legacy_frame_${boundary}_${policy}_contract)
        add_executable(${contract} tests/legacy_frame_${boundary}_contract.cpp)
        target_include_directories(${contract} PRIVATE include)
        target_link_libraries(${contract} PRIVATE kinoko_retdec_support)
        if(policy STREQUAL "checked")
            target_compile_options(${contract} PRIVATE /Od /RTC1 /Oy-)
        else()
            target_compile_options(${contract} PRIVATE /O2 /Oy)
        endif()
        target_link_options(${contract} PRIVATE /OPT:REF /OPT:ICF)
        add_test(NAME legacy_frame_${boundary}_${policy}_contract COMMAND ${contract})
        set_tests_properties(legacy_frame_${boundary}_${policy}_contract PROPERTIES TIMEOUT 120)
    endforeach()
endforeach()
'''+s[b:]
s=s.replace('    kinoko_legacy_frame_copy_checked_contract kinoko_legacy_frame_copy_optimized_contract\n','    kinoko_legacy_frame_copy_checked_contract kinoko_legacy_frame_copy_optimized_contract\n    kinoko_legacy_frame_entry_checked_contract kinoko_legacy_frame_entry_optimized_contract\n')
p.write_text(s)
p=r/'tools/check_migration_boundaries.py';s=p.read_text().replace('legacy_frame_copy\\.cpp','legacy_frame_entry\\.cpp');p.write_text(s)
Path(__file__).unlink()
