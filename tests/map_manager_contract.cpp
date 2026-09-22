#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_resource.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); std::abort(); } } while (0)
using namespace kinoko::map;
using kinoko::legacy::address;
namespace {
std::vector<int> calls;
KinokoMapManager *active;
KinokoActRuntime *expected_player;
int32_t __fastcall delete_document(KinokoActDocument *source, void *, int32_t flags) {
    CHECK(flags == 1);
    CHECK(!ManagerView(active).get(&ManagerRecord::player));
    CHECK(!ManagerView(active).get(&ManagerRecord::source_holder));
    calls.push_back(4); std::free(source); return 0;
}
}
extern "C" {
unsigned char g37;
void retdec_trace(const char *) {}
void retdec_trace_i32(const char *, int32_t) {}
void * kinoko_sqplus_object_initialize(void * ) { calls.push_back(0); return (void *)(intptr_t)(0); }
void * kinoko_sqplus_object_reset(void * ) { calls.push_back(1); return (void *)(intptr_t)(0); }
void * kinoko_sqplus_object_assign(void * , const void * ) { return (void *)(intptr_t)(0); }
void kinoko_act_runtime_dispose(KinokoActRuntime *player) {
    CHECK(player == expected_player);
    CHECK(kinoko_map_render_count(address(active)) == 0);
    CHECK(kinoko_map_event_count(address(active)) == 0);
    calls.push_back(2);
    // Original deletes the saved runtime, not this callback's replacement.
    ManagerView(active).set(&ManagerRecord::player, reinterpret_cast<KinokoActRuntime *>(1));
}
int32_t __fastcall kinoko_act_increment_frame(KinokoActRuntime *player, void *) {
    CHECK(player == expected_player); calls.push_back(5); return 0;
}
int32_t kinoko_act_update_frame(int32_t player) {
    CHECK(player == address(expected_player)); calls.push_back(6); return 17;
}
}
int main() {
    struct Storage { ManagerRecord manager; unsigned char tail[32]; } bytes{};
    std::memset(bytes.tail, 0x6d, sizeof(bytes.tail));
    active = reinterpret_cast<KinokoMapManager *>(&bytes.manager);
    bytes.manager.width = 777;
    CHECK(kinoko_map_manager_construct(active) == active);
    CHECK(bytes.manager.width == 777);
    for (auto byte : bytes.tail) CHECK(byte == 0x6d); // No old 512-byte memset.
    CHECK(kinoko_map_manager_update(active) == 0);
    CHECK(calls == std::vector<int>{0});
    kinoko::stage::DocumentVirtuals methods{};
    methods.deleting_destructor = reinterpret_cast<kinoko::stage::DeleteDocument>(delete_document);
    auto *document = static_cast<kinoko::act::DocumentRecord *>(std::calloc(1, sizeof(kinoko::act::DocumentRecord)));
    document->vtable = &methods;
    bytes.manager.source_act = reinterpret_cast<KinokoActDocument *>(document);
    bytes.manager.source_holder = static_cast<KinokoActSourceHolder *>(std::malloc(4));
    expected_player = static_cast<KinokoActRuntime *>(std::malloc(192));
    bytes.manager.player = expected_player;
    CHECK(kinoko_map_manager_update(active) == 17);
    CHECK((calls == std::vector<int>{0,5,6}));
    kinoko::camera::Record camera{};
    camera.x = 100; camera.y = 200; camera.center_x = 98.75f; camera.center_y = 203.5f;
    CHECK(kinoko_map_manager_prepare(active, reinterpret_cast<KinokoCamera *>(&camera)) == bytes.manager.source_act);
    CHECK(document->offset_x == -2 && document->offset_y == 3);
    kinoko_map_append_render(address(active), 0);
    kinoko_map_append_event(address(active), 0);
    const auto capacity = kinoko_map_event_capacity(address(active));
    kinoko_map_manager_clear(active);
    CHECK((calls == std::vector<int>{0,5,6,1,2,4}));
    CHECK(!bytes.manager.player && !bytes.manager.source_holder && !bytes.manager.source_act);
    CHECK(bytes.manager.width == 777 && kinoko_map_event_capacity(address(active)) == capacity);
    CHECK(kinoko_map_manager_update(active) == 0);
    kinoko_map_manager_clear(active);
    CHECK(calls.back() == 1);
    // Self-transfer must neither destroy nor lose the runtime.
    bytes.manager.player = expected_player = static_cast<KinokoActRuntime *>(std::malloc(192));
    const auto count = calls.size();
    kinoko_map_manager_assign(active, active);
    CHECK(bytes.manager.player == expected_player && calls.size() == count);
    std::free(expected_player); bytes.manager.player = nullptr;
    kinoko_map_containers_destroy(address(active));
    return 0;
}
