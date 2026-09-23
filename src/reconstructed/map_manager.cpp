#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_resource.h"
#include "kinoko/act_frame.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cmath>
#include <cstdlib>
#include <atomic>
extern "C" {
void retdec_trace(const char *);
void retdec_trace_i32(const char *, int32_t);
}
namespace {
using namespace kinoko::map;
using kinoko::legacy::address;
void delete_source(const ManagerView& manager) {
    auto *source = manager.get(&ManagerRecord::source_act);
    if (!source) return;
    const auto *methods = kinoko::legacy::load<kinoko::stage::DocumentPrefix>(source).vtable;
    if (methods && methods->deleting_destructor) methods->deleting_destructor(source, 1);
    manager.set(&ManagerRecord::source_act, static_cast<KinokoActDocument *>(nullptr));
}
}
extern "C" KinokoMapManager *kinoko_map_manager_construct(KinokoMapManager *storage) {
    if (!storage) return nullptr;
    const ManagerView manager(storage);
    // 46F4C0 initializes owners only; query results/dimensions survive clear.
    kinoko_sqplus_object_initialize((void *)(storage));
    manager.set(&ManagerRecord::player, static_cast<KinokoActRuntime *>(nullptr));
    kinoko_map_containers_construct(storage);
    manager.set(&ManagerRecord::source_act, static_cast<KinokoActDocument *>(nullptr));
    manager.set(&ManagerRecord::source_holder, static_cast<KinokoActSourceHolder *>(nullptr));
    return storage;
}
extern "C" void kinoko_map_manager_clear(KinokoMapManager *storage) {
    if (!storage) return;
    const ManagerView manager(storage);
    kinoko_sqplus_object_reset((void *)(storage));
    kinoko_map_containers_clear(storage);
    // 46F682 saves the runtime before its destructor; delete that same pointer.
    auto *player = manager.get(&ManagerRecord::player);
    if (player) { kinoko_act_runtime_dispose(player); std::free(player); }
    manager.set(&ManagerRecord::player, static_cast<KinokoActRuntime *>(nullptr));
    std::free(manager.get(&ManagerRecord::source_holder));
    manager.set(&ManagerRecord::source_holder, static_cast<KinokoActSourceHolder *>(nullptr));
    delete_source(manager);
}
extern "C" int32_t kinoko_map_manager_update(KinokoMapManager *storage) {
    static std::atomic<int32_t> trace_count{0};
    const auto trace = ++trace_count <= 16;
    if (trace) { retdec_trace("46f0b0:entry"); retdec_trace_i32("46f0b0:manager", address(storage)); }
    if (!storage) return 0;
    const ManagerView manager(storage);
    auto *player = manager.get(&ManagerRecord::player);
    if (trace) {
        retdec_trace_i32("46f0b0:act", address(manager.get(&ManagerRecord::source_act)));
        retdec_trace_i32("46f0b0:holder", address(manager.get(&ManagerRecord::source_holder)));
        retdec_trace_i32("46f0b0:resource", address(player));
    }
    if (!manager.get(&ManagerRecord::source_act) || !player) {
        if (trace) retdec_trace("46f0b0:skip");
        return 0;
    }
    kinoko_act_increment_frame(player, nullptr);
    // 46F0C5 reloads the receiver after IncrementFrame.
    const auto result = kinoko_act_update_frame(address(manager.get(&ManagerRecord::player)));
    if (trace) retdec_trace_i32("46f0b0:result", result);
    return result;
}
extern "C" KinokoActDocument *kinoko_map_manager_prepare(KinokoMapManager *storage, KinokoCamera *camera) {
    if (!storage || !camera) return nullptr;
    const ManagerView manager(storage);
    auto *source = manager.get(&ManagerRecord::source_act);
    if (!source) return nullptr;
    const kinoko::camera::View view(camera);
    using Camera = kinoko::camera::Record;
    using Document = kinoko::act::DocumentRecord;
    // 46EDDB/F8 explicitly round the subtraction to float before floor.
    const float x = view.get(&Camera::center_x) - view.get(&Camera::x);
    kinoko::act::DocumentView(source).set(&Document::offset_x, static_cast<float>(std::floor(static_cast<double>(x))));
    const float y = view.get(&Camera::center_y) - view.get(&Camera::y);
    source = manager.get(&ManagerRecord::source_act);
    kinoko::act::DocumentView(source).set(&Document::offset_y, static_cast<float>(std::floor(static_cast<double>(y))));
    return source;
}
extern "C" int32_t kinoko_map_manager_height(KinokoMapManager *manager) {
    return manager ? ManagerView(manager).get(&ManagerRecord::height) : 0;
}
extern "C" void *kinoko_map_manager_containers(KinokoMapManager *manager) {
    return manager ? ManagerView(manager).get(&ManagerRecord::containers) : nullptr;
}
