from pathlib import Path
import re,runpy
parser=runpy.run_path('tools/audio-migration-parser.py')
mask,matching=parser['mask'],parser['matching']
R=Path.cwd();p=R/'src/reconstructed/audio_runtime.cpp';s=p.read_text()
def replace(name,body):
 global s
 hits=list(re.finditer(r'(?m)^(?:static )?(?:\w+[ \t*]+)+'+name+r'\([^;{}]*\)\s*(?:noexcept\s*)?\{',s))
 assert len(hits)==1,(name,len(hits))
 h=hits[0];a=s.index('{',h.start());b=matching(mask(s),a,'{','}')+1
 s=s[:h.start()]+body.strip()+s[b:]
replace('retdec_se_entries_release','''
static void retdec_se_entries_release() {
    for (int index = 0; index < g_retdec_se_entry_count; ++index) {
        auto& entry = g_retdec_se_entries[index];
        if (entry.buffer) entry.buffer->Stop();
        entry.buffer.reset();
        entry.buffer_bytes = 0;
    }
    for (auto& entry : g_retdec_se_entries) entry = SoundEntry{};
    g_retdec_se_entry_count = 0;
}
''')
replace('retdec_se_pool_release','''
static void retdec_se_pool_release() {
    retdec_se_entries_release();
    for (auto& slot : g_retdec_se_pool.stream_slots) slot = SoundSlot{};
    g_retdec_se_pool.initialized = 0;
}
''')
s=s.replace('if (true)\n','').replace('if (true) {','{')
s=re.sub(r'(\b[\w.\[\]]+)\.reset\(\);\s*\1\.reset\(NULL\);',r'\1.reset();',s)
s=s.replace('g_retdec_se_entries[index].buffer.reset();\n            g_retdec_se_entries[index].buffer.reset(buffer);', 'g_retdec_se_entries[index].buffer.reset(buffer);')
assert '(LPTHREAD_START_ROUTINE)' not in s
cleanup='''
// The ABI records are table-owned allocations. Request/fade queues carry only
// handles; they must never free a BufferRecord or a decoder a second time.
static void retdec_audio_clear_queue(QueueNode*& head) noexcept {
    if (!head) return;
    while (head->next != head) {
        auto* node = head->next;
        head->next = node->next;
        std::free(node);
    }
    std::free(std::exchange(head, nullptr));
}
static void retdec_audio_clear_handle_allocations(HandleTable& table) noexcept {
    if (table.buffers_begin) {
        for (auto** it = table.buffers_begin; it != table.buffers_end; ++it) {
            auto* buffer = *it;
            if (!buffer) continue;
            if (buffer->path.capacity >= sizeof(buffer->path.storage.inline_text))
                std::free(buffer->path.storage.allocated_text);
            buffer->~BufferRecord();
            std::free(buffer);
        }
    }
    std::free(table.buffers_begin);
    std::free(table.generations_begin);
    table.buffers_begin = table.buffers_end = table.buffers_capacity = nullptr;
    table.generations_begin = table.generations_end = table.generations_capacity = nullptr;
    retdec_audio_clear_queue(table.live_handles);
}
static void retdec_audio_manager_destroy() noexcept {
    if (!g_retdec_audio_manager_initialized) return;
    auto& state = g_retdec_audio_manager_state;
    retdec_audio_clear_queue(state.active.head);
    retdec_audio_clear_queue(state.pending.head);
    retdec_audio_clear_queue(state.retired.head);
    retdec_audio_clear_handle_allocations(state.handles);
    DeleteCriticalSection(&state.handles.lock);
    DeleteCriticalSection(&state.lock);
    state = ManagerRecord{};
    g_retdec_audio_manager_initialized = 0;
}
'''
a=s.index('static void retdec_trace_audio_text(const char *label, const char *value)\n{')
s=s[:a]+cleanup+'\n'+s[a:]
replace('retdec_audio_manager_construct','''
static void retdec_audio_manager_construct() {
    if (g_retdec_audio_manager_initialized) return;
    auto& state = g_retdec_audio_manager_state;
    state = ManagerRecord{};
    // Allocate every list before initializing either OS synchronization object.
    // A failed construction owns nothing and can be retried without leaks.
    QueueRecord handles{};
    if (!retdec_audio_manager_list_init(handles) ||
        !retdec_audio_manager_list_init(state.active) ||
        !retdec_audio_manager_list_init(state.pending) ||
        !retdec_audio_manager_list_init(state.retired)) {
        retdec_audio_clear_queue(handles.head);
        retdec_audio_clear_queue(state.active.head);
        retdec_audio_clear_queue(state.pending.head);
        retdec_audio_clear_queue(state.retired.head);
        state = ManagerRecord{};
        return;
    }
    const auto* symbols = kinoko_audio_host_symbols();
    state.handles.live_handles = handles.head;
    state.handles.vtable = symbols->handle_table_vtable;
    state.handles.lock_vtable = symbols->critical_section_vtable;
    state.lock_vtable = symbols->critical_section_vtable;
    InitializeCriticalSection(&state.lock);
    InitializeCriticalSection(&state.handles.lock);
    state.master_gain = state.stream_gain = 1.0f;
    g_retdec_audio_manager_initialized = 1;
}
''')
replace('function_40b3a0','''
int32_t function_40b3a0(void) {
    function_40a460(); // join both workers before touching any owned resource
    retdec_bgm_release_all_tracks();
    retdec_se_pool_release();
    retdec_audio_manager_destroy();
    return 1;
}
''')
s=s.replace('const auto result = address(g_audio_device.device.get());\n    // Shutdown has already joined', 'const auto result = address(g_audio_device.device.get());\n    function_40b3a0();\n    // Shutdown has already joined')
s+='''
namespace {
// Constructed last and destroyed first: workers cannot race member destruction.
struct AudioRuntimeShutdown {
    ~AudioRuntimeShutdown() {
        function_40b3a0();
        if (g_retdec_audio_lock_initialized) {
            DeleteCriticalSection(&g_retdec_audio_lock);
            g_retdec_audio_lock_initialized = 0;
        }
    }
} audio_runtime_shutdown;
}
'''
s='\n'.join(line.rstrip() for line in s.splitlines())+'\n';p.write_text(s)
p=R/'CMakeLists.txt';s=p.read_text()
s=s.replace('add_library(kinoko_legacy_abi STATIC','''add_library(kinoko_audio_runtime STATIC src/reconstructed/audio_runtime.cpp)
target_include_directories(kinoko_audio_runtime PUBLIC include PRIVATE src/decompiled)
target_compile_definitions(kinoko_audio_runtime PRIVATE _CRT_SECURE_NO_WARNINGS
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,COMPILE_DEFINITIONS>)
target_compile_options(kinoko_audio_runtime PRIVATE /Gy)
target_link_libraries(kinoko_audio_runtime PUBLIC kinoko_actor_collision ole32 dxguid winmm user32)

add_library(kinoko_legacy_abi STATIC''',1)
s=s.replace('target_link_libraries(kinoko_retdec_rebuild PRIVATE\n','target_link_libraries(kinoko_retdec_rebuild PRIVATE\n    kinoko_audio_runtime\n',1)
s=s.replace('add_executable(kinoko_squirrel_c_api_contract', '''add_executable(kinoko_com_owner_contract tests/com_owner_contract.cpp)
target_include_directories(kinoko_com_owner_contract PRIVATE include)
add_test(NAME com_owner_contract COMMAND kinoko_com_owner_contract)

add_executable(kinoko_audio_runtime_contract tests/audio_runtime_contract.cpp)
target_include_directories(kinoko_audio_runtime_contract PRIVATE include src/decompiled)
target_compile_definitions(kinoko_audio_runtime_contract PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(kinoko_audio_runtime_contract PRIVATE kinoko_actor_collision ole32 dxguid winmm user32)
add_test(NAME audio_runtime_contract COMMAND kinoko_audio_runtime_contract)
set_tests_properties(audio_runtime_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_squirrel_c_api_contract''',1)
s=s.replace('set(KINOKO_TOOL_TARGETS\n','set(KINOKO_TOOL_TARGETS\n    kinoko_com_owner_contract\n    kinoko_audio_runtime_contract\n',1)
p.write_text(s)
p=R/'.github/workflows/windows-x86.yml';s=p.read_text().replace('squirrel_c_api_contract|','squirrel_c_api_contract|com_owner_contract|audio_runtime_contract|',1);p.write_text(s)
