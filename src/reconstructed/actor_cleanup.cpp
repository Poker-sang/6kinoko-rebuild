#include "kinoko/integer_vector.h"
#include "kinoko/animation_storage.h"
#include "kinoko/integer_map.h"
#include "kinoko/actor_priority.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/actor_records.hpp"

#include <cstddef>
#include <cstdlib>

extern "C" {
int32_t function_405d60(int32_t texture_handle);
int32_t function_463730(int32_t actor_tree);
extern int32_t g23;
}

namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;

template <typename T>
T *pointer(int32_t address) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
int32_t address(const void *value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}

}

// 464E20: texture handles, live actors, nonowning lookup, owning animations,
// priority tree, then transient iteration state. Keep capacities and sentinels.
extern "C" int32_t kinoko_clear_actor_manager(int32_t manager) {
    const ManagerView state(pointer<void>(manager));
    const auto textures = state.view(&ManagerPrefix::textures);
    const int32_t texture_slot=address(textures.data());
    const auto* handles=pointer<int32_t>(kinoko_integer_vector_data(texture_slot));
    for(uint32_t i=0;i<kinoko_integer_vector_size(texture_slot);++i) function_405d60(handles[i]);
    kinoko_integer_vector_clear(texture_slot);
    // Actor destruction precedes releasing animations that actors only borrow.
    const auto actors = state.view(&ManagerPrefix::actors);
    function_463730(address(actors.data()));
    const auto animations = state.view(&ManagerPrefix::animation_lookup);
    kinoko_integer_map_clear(animations.get(&TreeIndex::head));
    animations.set(&TreeIndex::count,int32_t{0});
    kinoko_clear_animation_list(address(state.bytes(&ManagerPrefix::animations)));
    kinoko_priority_clear(address(actors.data()));
    const auto iteration = state.view(&ManagerPrefix::iteration);
    const auto iteration_begin = iteration.get(&VectorIndex::begin);
    iteration.set(&VectorIndex::end, iteration_begin);
    state.set(&ManagerPrefix::iteration_state, int32_t{0});
    state.set(&ManagerPrefix::cleanup_pending, uint8_t{0});
    return static_cast<int32_t>(iteration_begin);
}
