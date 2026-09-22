#include "kinoko/render_queue.h"
#include "kinoko/camera.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <stdexcept>

namespace {
// 46A210 appends every borrowed layer; duplicates and insertion order matter.
struct RenderLayer;
using DrawLayer = int32_t (__thiscall *)(RenderLayer *,KinokoCamera *);
struct LayerMethods { DrawLayer draw; };
struct RenderLayer { const LayerMethods *methods; };
std::list<RenderLayer *> layers; // borrowed; queue never destroys a layer
using kinoko::legacy::address;
using kinoko::legacy::field;
using kinoko::legacy::pointer;
}
extern "C" void kinoko_initialize_render_queue(void) { layers.clear(); }
extern "C" int32_t kinoko_render_queue_identity(void) { return address(&layers); }
extern "C" int32_t kinoko_render_queue_first(void) {
    return layers.empty() ? address(&layers) : address(&layers.front());
}
extern "C" int32_t kinoko_render_queue_size(void) { return static_cast<int32_t>(layers.size()); }
extern "C" int32_t kinoko_clear_render_queue(void) {
    layers.clear();
    return address(&layers);
}
extern "C" int32_t kinoko_append_render_queue(int32_t object) {
    if(layers.size()==0x3ffffffeu) throw std::length_error("list<T> too long");
    layers.push_back(pointer<RenderLayer>(object));
    return address(&layers.back());
}
extern "C" void kinoko_draw_render_queue(int32_t camera) {
    for(auto *object:layers) {
        if(!object) continue;
        const auto *methods=kinoko::legacy::load<const LayerMethods *>(object);
        if(methods && methods->draw) methods->draw(object,pointer<KinokoCamera>(camera));
    }
}
