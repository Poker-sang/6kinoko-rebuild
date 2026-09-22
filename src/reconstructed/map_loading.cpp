#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/act_document.h"
#include "kinoko/act_source.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_runtime.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include <cstdlib>
extern "C" {
void retdec_trace(const char *);
void retdec_trace_i32(const char *, int32_t);
void retdec_trace_squirrel_name(const char *, int32_t);
}
namespace {
using namespace kinoko::map;
using namespace kinoko::script;
using kinoko::script::binding::Object;
using QueryLayout = uint8_t (__thiscall *)(KinokoActLayout *, const void *, KinokoActLayout **);
struct LayoutMethods { void *prefix[2]; QueryLayout query; };
struct MapType { void *table, *cache; char name[sizeof(".?AVC2DMapLayout@@")]; };
const MapType map_type{{}, {}, ".?AVC2DMapLayout@@"};
static_assert(offsetof(LayoutMethods, query) == 8);
KinokoActLayout *as_map(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    const auto *methods = LayoutView(layout).get(&LayoutRecord::methods);
    const auto query = kinoko::legacy::load<QueryLayout>(methods + offsetof(LayoutMethods, query));
    KinokoActLayout *result = nullptr;
    return query(layout, &map_type, &result) ? result : nullptr;
}
bool publish(KinokoMapManager *storage, SQVM *vm, void *map_class, void *root_storage) {
    const ManagerView manager(storage);
    {
        Object instance(vm);
        if (!ObjectView(map_class).value()._type ||
            !(int32_t)(intptr_t)(kinoko_sqplus_object_new_instance((void *)(intptr_t)(instance.location()), (const void *)(map_class)))) return false;
        kinoko_sqplus_object_assign((void *)(storage), (const void *)(intptr_t)(instance.location()));
    }
    kinoko_sqplus_object_set_instance((void *)(storage), (void *)(storage));
    kinoko_sqplus_object_raw_set_name((void *)(root_storage), "map", (const void *)(storage));
    Object names(vm);
    kinoko_sqplus_object_new_array((void *)(intptr_t)(pointer<int32_t>(names.location())), 0);
    // Original re-reads the holder/count and runtime on every iteration.
    for (int32_t index = 0; index < kinoko_act_source_layer_count(manager.get(&ManagerRecord::source_holder)); ++index) {
        auto *layout = as_map(kinoko_act_layer_layout(manager.get(&ManagerRecord::player), index));
        if (!layout) continue;
        auto *layer = LayoutView(layout).get(&LayoutRecord::owning_layer);
        if (!layer) continue;
        const kinoko::legacy::StringView name(LayerView(layer).bytes(&LayerRecord::name));
        Object text(vm);
        sq_pushstring(vm, name.data(), -1); // Empty names are also present in the original array.
        text.view().capture(vm, -1);
        sq_pop(vm, 1);
        kinoko_sqplus_object_append((void *)(intptr_t)(names.location()), (const void *)(intptr_t)(text.location()));
    }
    kinoko_sqplus_object_reverse((void *)(intptr_t)(names.location()));
    kinoko_sqplus_object_raw_set_name((void *)(storage), "layer_name", (const void *)(intptr_t)(names.location()));
    Object root(vm);
    kinoko_sqplus_object_assign((void *)(intptr_t)(root.location()), (const void *)(root_storage));
    Object current(vm);
    const auto *name = kinoko_act_document_name(manager.get(&ManagerRecord::source_act));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(root.location()), (void *)(intptr_t)(current.location()), name);
    // 46F99A -> 46F9AF always publishes the lookup result (including null).
    // The old reconstruction indexed current_map+4 in a three-word array.
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(root.location()), "currentMap", (const void *)(intptr_t)(current.location()));
    retdec_trace_squirrel_name("map:act-name", address(name));
    retdec_trace_i32("map:current-map-type", current.view().value()._type);
    retdec_trace_i32("map:current-map-data", data_bits(current.view().value()));
    return true; // current, root, names release in the original order.
}
}
extern "C" int32_t kinoko_map_manager_load(KinokoMapManager *storage, const char *path,
    SQVM *vm, void *map_class, void *root_object) {
    if (!storage || !path || !vm) return 0;
    const ManagerView manager(storage);
    kinoko_map_manager_clear(storage);
    auto *source = kinoko_act_document_create();
    manager.set(&ManagerRecord::source_act, source);
    if (!source) return 0;
    if (!kinoko_act_document_load(source, path)) {
        retdec_trace("map:act-load-failed");
        // No runtime or holder exists yet. Release only the failed document,
        // as 46F740 does, without resetting the script/containers a second time.
        source = manager.get(&ManagerRecord::source_act);
        if (source) {
            const auto *methods = kinoko::legacy::load<kinoko::stage::DocumentPrefix>(source).vtable;
            methods->deleting_destructor(source, 1);
        }
        manager.set(&ManagerRecord::source_act, static_cast<KinokoActDocument *>(nullptr));
        return 0;
    }
    source = manager.get(&ManagerRecord::source_act);
    kinoko_act_document_load_resources(source, ""); // original ignores result
    auto *holder = static_cast<KinokoActSourceHolder *>(std::malloc(sizeof(KinokoActSourceHolder)));
    if (!holder) { kinoko_map_manager_clear(storage); return 0; }
    kinoko_act_source_initialize(holder, manager.get(&ManagerRecord::source_act));
    manager.set(&ManagerRecord::source_holder, holder);
    auto *player = kinoko_act_source_create_runtime(holder);
    auto *previous = manager.get(&ManagerRecord::player);
    if (player != previous && previous) {
        kinoko_act_runtime_dispose(previous);
        std::free(previous);
    }
    manager.set(&ManagerRecord::player, player);
    // Retain existing reconstruction's failure guards at the VM boundary.
    if (!player || retdec_root_table_construct_this(address(player), address(vm), 0) < 0) {
        retdec_trace("map:450e30-failed");
        kinoko_map_manager_clear(storage); return 0;
    }
    if (retdec_begin_stage_this(address(manager.get(&ManagerRecord::player)), 0) < 0) {
        retdec_trace("map:450950-failed");
        kinoko_map_manager_clear(storage); return 0;
    }
    source = manager.get(&ManagerRecord::source_act);
    manager.set(&ManagerRecord::width, kinoko_act_document_screen_width(source));
    manager.set(&ManagerRecord::height, kinoko_act_document_screen_height(source));
    if (!map_class || !root_object || !publish(storage, vm, map_class, root_object)) {
        retdec_trace("map:instance-copy-failed");
        kinoko_map_manager_clear(storage); return 0;
    }
    retdec_trace_i32("map:instance", address(storage));
    retdec_trace_i32("map:act", address(source));
    retdec_trace_squirrel_name("map:path", address(path));
    return 1;
}
