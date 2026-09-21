#include "kinoko/integer_map.h"
#include "kinoko/render_queue.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_resource_records.hpp"
#include <cstddef>
#include <cstdlib>
#include <new>
#include <list>
#include "kinoko/legacy_memory.hpp"

extern "C" {
int32_t function_450020(int32_t resource);
extern int32_t g603, g604;
extern int32_t g638, g639;
int32_t function_40b3a0(void);
}

namespace {
struct StageOwner {
    void *source; // owned ACT source; released through its original destructor
    void *data;   // owned auxiliary allocation
    void *runtime; // owned runtime allocation; may borrow source during setup
};
struct StageEntry;
using StageList=std::list<StageEntry>;
struct StageEntry { StageOwner* owner; StageList::iterator position; };
StageList* stages() { return kinoko::legacy::pointer<StageList>(g603); }
void release_stage_list() {
    delete stages();g603=g604=0;
}
void release_render_queue() { kinoko_clear_render_queue(); }
void release_sound_tree() {
    kinoko_integer_map_destroy(g638);g638=g639=0;
}

void destroy_owner(StageOwner *owner) {
    if (!owner) return;
    std::free(owner->data);
    owner->data = nullptr;
    // Detach legacy borrowed fixtures before destroying their source;
    // BeginStage clones remain independently owned by the runtime.
    if (owner->runtime) {
        const kinoko::native::RecordView<kinoko::act::RuntimeRecord> runtime(owner->runtime);
        if (runtime.get(&kinoko::act::RuntimeRecord::act) ==
                reinterpret_cast<uintptr_t>(owner->source))
            runtime.set(&kinoko::act::RuntimeRecord::act, uint32_t{0});
    }
    if (owner->source) {
        auto **vtable = *static_cast<void ***>(owner->source);
        retdec_call_thiscall1(owner->source, vtable[4], 1);
        owner->source = nullptr;
    }
    if (owner->runtime) {
        function_450020(static_cast<int32_t>(reinterpret_cast<uintptr_t>(owner->runtime)));
        std::free(owner->runtime);
        owner->runtime = nullptr;
    }
    std::free(owner);
}
}

// Use real CRT registration and callable source addresses; original absolute
// executable addresses cannot be registered in the reconstructed process.
extern "C" int32_t function_4d3ce0() {
    kinoko_stage_list_construct();
    return std::atexit(release_stage_list);
}

extern "C" int32_t function_4d3e50() {
    kinoko_initialize_render_queue();
    return std::atexit(release_render_queue);
}

extern "C" int32_t function_4d3f50() {
    g638=kinoko_integer_map_create();g639=0;
    return std::atexit(release_sound_tree);
}

// 465F70: destroy payloads first, reset the list, then release its nodes.
extern "C" int32_t kinoko_clear_global_stages() {
    if(!stages()) return 0;
    for(auto& entry:*stages()) { destroy_owner(entry.owner);entry.owner=nullptr; }
    stages()->clear();g604=0;return g603;
}

// 470890: the rebuilt sound manager owns buffers in its SE pool and BGM
// track records. Join its workers before releasing that storage; then clear
// the non-owning original ID lookup tree, preserving its sentinel.
extern "C" int32_t kinoko_clear_global_sound() {
    function_40b3a0();
    kinoko_integer_map_clear(g638);
    g639 = 0;
    return 1;
}

extern "C" void kinoko_stage_list_construct() { g603=kinoko::legacy::address(new StageList);g604=0; }
extern "C" void kinoko_stage_list_destroy() { release_stage_list(); }
extern "C" int32_t kinoko_stage_list_first() {
    return stages() && !stages()->empty()?kinoko::legacy::address(&stages()->front()):g603;
}
extern "C" int32_t kinoko_stage_list_next(int32_t token) {
    auto position=kinoko::legacy::pointer<StageEntry>(token)->position;
    ++position;return position==stages()->end()?g603:kinoko::legacy::address(&*position);
}
extern "C" int32_t kinoko_stage_list_value(int32_t token) {
    return kinoko::legacy::address(kinoko::legacy::pointer<StageEntry>(token)->owner);
}
extern "C" int32_t kinoko_stage_list_append(int32_t owner) {
    stages()->emplace_back();auto position=std::prev(stages()->end());
    position->owner=kinoko::legacy::pointer<StageOwner>(owner);position->position=position;
    g604=static_cast<int32_t>(stages()->size());return kinoko::legacy::address(&*position);
}
extern "C" int32_t function_4d47f0() {
    if(stages()) stages()->clear();g604=0;return g603;
}
