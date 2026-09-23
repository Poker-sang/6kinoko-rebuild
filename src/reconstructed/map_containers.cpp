#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <vector>
#include <stdexcept>
#include <iterator>

extern "C" unsigned char g37;
namespace kinoko::map {
using RenderLayer = kinoko::map::RenderLayerRecord;
static_assert(sizeof(RenderLayer) == 8);
struct Containers {
    std::list<RenderLayer> layers;
    std::vector<KinokoActLayout *> events;
};
ManagerView manager_view(KinokoMapManager* manager) { return ManagerView(manager); }
Containers& state(KinokoMapManager* manager) { return *manager_view(manager).get(&ManagerRecord::containers); }
}
using namespace kinoko::map;
extern "C" void kinoko_map_containers_construct(KinokoMapManager* manager) {
    manager_view(manager).set(&ManagerRecord::containers, new Containers);
}
extern "C" void kinoko_map_containers_clear(KinokoMapManager* manager) {
    auto& value = state(manager);
    value.events.clear(); // 46F620 retains vector capacity
    value.layers.clear(); // borrowed layout values are never destroyed here
}
extern "C" void kinoko_map_containers_destroy(KinokoMapManager* manager) {
    delete manager_view(manager).get(&ManagerRecord::containers);
    manager_view(manager).set(&ManagerRecord::containers, static_cast<Containers *>(nullptr));
}
extern "C" void kinoko_map_containers_assign(KinokoMapManager* destination, KinokoMapManager* source) {
    if (destination == source) return;
    auto& out = state(destination);
    const auto& in = state(source);
    // 4700B0 clears/reconstructs nodes; never reuse old node addresses or
    // copy a forged source vtable. 46F320 retains capacity on shorter copies.
    out.layers.clear();
    for (const auto& layer : in.layers) out.layers.push_back({&g37, layer.layout});
    out.events.assign(in.events.begin(), in.events.end());
}
extern "C" KinokoRenderLayer* kinoko_map_append_render(KinokoMapManager* manager, KinokoActLayout* layout) {
    auto& layers = state(manager).layers;
    if (layers.size() == 0x1ffffffeu) throw std::length_error("list<T> too long");
    layers.push_back({&g37, layout});
    return reinterpret_cast<KinokoRenderLayer*>(&layers.back());
}
extern "C" uint32_t kinoko_map_render_count(KinokoMapManager* manager) {
    return static_cast<uint32_t>(state(manager).layers.size());
}
extern "C" KinokoRenderLayer* kinoko_map_render_at(KinokoMapManager* manager, uint32_t index) {
    auto& layers = state(manager).layers;
    if (index >= layers.size()) return 0;
    return reinterpret_cast<KinokoRenderLayer*>(&*std::next(layers.begin(), index));
}
extern "C" void kinoko_map_append_event(KinokoMapManager* manager, KinokoActLayout* layout) {
    state(manager).events.push_back(layout);
}
extern "C" uint32_t kinoko_map_event_count(KinokoMapManager* manager) {
    return static_cast<uint32_t>(state(manager).events.size());
}
extern "C" KinokoActLayout* kinoko_map_event_at(KinokoMapManager* manager, uint32_t index) {
    const auto& events = state(manager).events;
    return index < events.size() ? events[index] : nullptr;
}
extern "C" uint32_t kinoko_map_event_capacity(KinokoMapManager* manager) {
    return static_cast<uint32_t>(state(manager).events.capacity());
}
