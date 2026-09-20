#include "kinoko/map_containers.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <vector>
#include <stdexcept>
#include <iterator>

extern "C" unsigned char g37;
namespace {
using kinoko::legacy::field;
using kinoko::legacy::address;
struct RenderLayer { void* vtable; int32_t layout; };
static_assert(sizeof(RenderLayer) == 8);
struct Containers {
    std::list<RenderLayer> layers;
    std::vector<int32_t> events;
};
Containers& state(int32_t manager) { return *field<Containers*>(manager + 24); }
}
extern "C" void kinoko_map_containers_construct(int32_t manager) {
    field<Containers*>(manager + 24) = new Containers;
}
extern "C" void kinoko_map_containers_clear(int32_t manager) {
    auto& value = state(manager);
    value.events.clear(); // 46F620 retains vector capacity
    value.layers.clear(); // borrowed layout values are never destroyed here
}
extern "C" void kinoko_map_containers_destroy(int32_t manager) {
    delete field<Containers*>(manager + 24);
    field<Containers*>(manager + 24) = nullptr;
}
extern "C" void kinoko_map_containers_assign(int32_t destination, int32_t source) {
    if (destination == source) return;
    auto& out = state(destination);
    const auto& in = state(source);
    // 4700B0 clears/reconstructs nodes; never reuse old node addresses or
    // copy a forged source vtable. 46F320 retains capacity on shorter copies.
    out.layers.clear();
    for (const auto& layer : in.layers) out.layers.push_back({&g37, layer.layout});
    out.events.assign(in.events.begin(), in.events.end());
}
extern "C" int32_t kinoko_map_append_render(int32_t manager, int32_t layout) {
    auto& layers = state(manager).layers;
    if (layers.size() == 0x1ffffffeu) throw std::length_error("list<T> too long");
    layers.push_back({&g37, layout});
    return address(&layers.back());
}
extern "C" uint32_t kinoko_map_render_count(int32_t manager) {
    return static_cast<uint32_t>(state(manager).layers.size());
}
extern "C" int32_t kinoko_map_render_at(int32_t manager, uint32_t index) {
    auto& layers = state(manager).layers;
    if (index >= layers.size()) return 0;
    return address(&*std::next(layers.begin(), index));
}
extern "C" void kinoko_map_append_event(int32_t manager, int32_t layout) {
    state(manager).events.push_back(layout);
}
extern "C" uint32_t kinoko_map_event_count(int32_t manager) {
    return static_cast<uint32_t>(state(manager).events.size());
}
extern "C" int32_t kinoko_map_event_at(int32_t manager, uint32_t index) {
    const auto& events = state(manager).events;
    return index < events.size() ? events[index] : 0;
}
extern "C" uint32_t kinoko_map_event_capacity(int32_t manager) {
    return static_cast<uint32_t>(state(manager).events.capacity());
}
