#include "kinoko/map_activation.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/map_render.h"
#include "kinoko/map_containers.h"
#include "kinoko/act_runtime.h"
#include "kinoko/diagnostics.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_source_runtime.h"
#include <cstdio>

extern "C" void retdec_trace_i32(const char *label, int32_t value);

namespace {
using namespace kinoko::map;
using kinoko::legacy::address;
using kinoko::legacy::load;
using kinoko::script::ObjectStorage;

// Lookup captures one external SqPlus reference. Keep its lifetime identical
// to the original local SquirrelObject; never overlay an SQObjectPtr here.
class InitCallback final {
    ObjectStorage object_{};
public:
    InitCallback(const KinokoSquirrelObject *environment, const char *name) {
        kinoko_sqplus_object_get_value((void *)(environment), (void *)(&object_), name);
    }
    ~InitCallback() { (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(&object_))); }
    InitCallback(const InitCallback&) = delete;
    InitCallback& operator=(const InitCallback&) = delete;
    bool is_closure() const { return object_.value._type == OT_CLOSURE; }
    const KinokoSquirrelObject *borrow() const {
        return reinterpret_cast<const KinokoSquirrelObject *>(&object_);
    }
};

// 46EE20: callback, receiver, five integer arguments, then restore stack.
int32_t invoke_event(SQVM *vm, HSQOBJECT callback, HSQOBJECT environment,
    int32_t id, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    kinoko::script::StackTop stack(vm);
    sq_pushobject(vm, callback);
    sq_pushobject(vm, environment);
    sq_pushinteger(vm, id);
    sq_pushinteger(vm, left);
    sq_pushinteger(vm, top);
    sq_pushinteger(vm, right);
    sq_pushinteger(vm, bottom);
    return kinoko_sq_call(address(vm), 6, 1, 1);
}
}

// 463E60: retain the initial count, but re-read the placement buffer on each
// iteration; callbacks may replace its storage. No record is used after a call.
extern "C" int32_t kinoko_map_create_actors(KinokoActorManager *manager,
    KinokoActLayout *layout, const KinokoSquirrelObject *environment) {
    if (!layout || !LayoutView(layout).get(&LayoutRecord::owning_layer)) return 0;
    const int32_t count = placement_count(layout);
    int32_t created = 0;
    retdec_trace_i32("actor:map-records", count);
    for (int32_t index = 0; index < count; ++index) {
        const PlacementView record(placement_at(layout, index));
        const auto id = record.get(&Placement::chip_id);
        char name[256];
        sprintf_s(name, sizeof(name), "Init%04x", static_cast<unsigned int>(id));
        InitCallback callback(environment, name);
        if (!callback.is_closure()) continue;
        float x = static_cast<float>(record.get(&Placement::left));
        float y = static_cast<float>(record.get(&Placement::top));
        const unsigned char *initialization_data = nullptr;
        auto *chip = retdec_mcd_find_chip(kinoko_map_layer_chip_data(layout), id);
        if (chip) {
            const ChipView definition(chip->bytes);
            x = static_cast<float>(static_cast<double>(x) +
                definition.get(&ChipDefinition::width) * 0.5 + 1.0);
            y = static_cast<float>(static_cast<double>(y) +
                definition.get(&ChipDefinition::height) *
                ((definition.get(&ChipDefinition::flags) & 0x10000u) ? 0.5 : 1.0));
            initialization_data = chip->bytes;
        }
        if (kinoko_actor_create_map_instance(manager, callback.borrow(), x, y,
            static_cast<int32_t>(id), initialization_data)) ++created;
    }
    retdec_trace_i32("actor:map-created", created);
    return created;
}

// 46FD70: append even a missing/callback-free layer before deciding whether
// callbacks are needed. The caller owns callback/environment external refs.
extern "C" int32_t kinoko_map_create_events(KinokoMapManager *manager, SQVM *vm,
    const char *name, const KinokoSquirrelObject *callback,
    const KinokoSquirrelObject *environment) {
    auto *layout = kinoko_map_lookup_layout(manager, name);
    const auto function = load<ObjectStorage>(callback).value;
    const auto receiver = load<ObjectStorage>(environment).value;
    kinoko_map_append_event(address(manager), address(layout));
    if (!layout || function._type != OT_CLOSURE) return 0;
    const int32_t count = placement_count(layout);
    int32_t completed = 0;
    for (int32_t index = 0; index < count; ++index) {
        const PlacementView record(placement_at(layout, index));
        const auto id = record.get(&Placement::chip_id);
        const auto left = record.get(&Placement::left), top = record.get(&Placement::top);
        auto *chip = retdec_mcd_find_chip(kinoko_map_layer_chip_data(layout), id);
        // Retain R136's defined failure for malformed records; do not invent
        // bounds from the original decompiler's uninitialized temporaries.
        if (!chip) {
            retdec_trace_i32("map:event-missing-chip", static_cast<int32_t>(id));
            return -1;
        }
        const ChipView definition(chip->bytes);
        if (invoke_event(vm, function, receiver, static_cast<int32_t>(id), left, top,
            left + definition.get(&ChipDefinition::width),
            top + definition.get(&ChipDefinition::height)) < 0) return -1;
        ++completed;
    }
    retdec_trace_i32("map:event-created", completed);
    return completed;
}
