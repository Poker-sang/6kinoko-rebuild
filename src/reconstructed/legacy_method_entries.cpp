#include "kinoko/actor_pool.h"
#include "kinoko/actor_owner_list.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/game_script_host.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/act_layout3d_io.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_abi.h"
#include <cstddef>
#include <cstring>

static_assert(sizeof(void*) == sizeof(int32_t), "Original Win32 object addresses");

// Only the ABI is adapted here. Recovered game/loading bodies remain the
// single source of truth; the adapters do not add ordering or fallback logic.
extern "C" {
int32_t retdec_destroy_cact_with_flags(int32_t receiver, unsigned char flags);
int32_t retdec_c2dlayout_set_layer_impl(int32_t receiver, int32_t layer);
int32_t retdec_c2dlayout_update_faithful_impl(int32_t receiver);
int32_t retdec_c2dlayout_draw_impl(int32_t receiver, float x, float y);
int32_t retdec_begin_stage_this(int32_t receiver, int32_t stage);
int32_t retdec_root_table_construct_this(int32_t receiver, int32_t vm, int32_t output);
int32_t retdec_act_bitblt_this(int32_t receiver, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t resource, int32_t source_x, int32_t source_y, int32_t blend, float alpha);
extern int32_t g953;






}

// retdec_cact_destructor_bridge
extern "C" int32_t __fastcall kinoko_method_destroy_act(int32_t receiver, void* /* unused_edx */,
    unsigned char flags) {
    return retdec_destroy_cact_with_flags(receiver, flags);
}

// function_42bcc0
extern "C" int32_t __fastcall kinoko_method_layout_set_layer(int32_t receiver, void* /* unused_edx */,
    int32_t layer) {
    return retdec_c2dlayout_set_layer_impl(receiver, layer);
}

// function_42c100
extern "C" int32_t __fastcall kinoko_method_layout_update(int32_t receiver, void* /* unused_edx */) {
    return retdec_c2dlayout_update_faithful_impl(receiver);
}

// function_42c300
extern "C" int32_t __fastcall kinoko_method_layout_draw(int32_t receiver, void* /* unused_edx */,
    float x, float y) {
    return retdec_c2dlayout_draw_impl(receiver, x, y);
}

// function_43c860
extern "C" int32_t __fastcall kinoko_method_layout3d_assign(int32_t receiver, void* /* unused_edx */,
    int32_t source, int32_t mode) {
    return kinoko_act_read_layout3d_properties(
        kinoko::legacy::pointer<KinokoActLayout>(receiver),
        kinoko::legacy::pointer<int32_t>(source),mode);
}

// function_450950
extern "C" int32_t __fastcall kinoko_method_begin_stage(int32_t receiver, void* /* unused_edx */,
    int32_t stage) {
    return retdec_begin_stage_this(receiver, stage);
}

// function_450e30
extern "C" int32_t __fastcall kinoko_method_root_table_construct(int32_t receiver,
    void* /* unused_edx */, int32_t vm, int32_t output) {
    return retdec_root_table_construct_this(receiver, vm, output);
}

// function_4514a0
extern "C" int32_t __fastcall kinoko_method_act_bitblt(int32_t receiver, void* /* unused_edx */,
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t resource, int32_t source_x,
    int32_t source_y, int32_t blend, float alpha) {
    return retdec_act_bitblt_this(receiver, x, y, width, height, resource, source_x, source_y, blend, alpha);
}

// IDA 457A10: the child pointer range is stored at +156/+160. The manager
// at 51BAA0 resolves each child; the resolved object's first virtual method
// receives the incoming argument. A null lookup can occur in the rebuilt
// runtime, so retain the established guard at that boundary.
namespace {
struct MeshChildRange {
    std::byte preceding[156];
    int32_t* begin;
    int32_t* end;
};
static_assert(offsetof(MeshChildRange, begin) == 156);
static_assert(offsetof(MeshChildRange, end) == 160);
int32_t update_mesh_children(MeshChildRange* node, int32_t argument) {
    if (!node || !node->begin || !node->end || node->end - node->begin < 1)
        return 0;
    int32_t result = 0;
    for (auto* item = node->begin; item != node->end; ++item) {
        if (!*item) continue;
        auto* manager = &g953;
        auto* methods = *reinterpret_cast<int32_t**>(manager);
        if (!methods || !methods[3]) continue;
        const auto handle = retdec_call_thiscall2_result(manager,
            reinterpret_cast<void*>(methods[3]), *item, argument);
        if (!handle) continue;
        auto* object = *kinoko::legacy::pointer<int32_t*>(handle);
        if (!object) continue;
        auto* object_methods = *reinterpret_cast<int32_t**>(object);
        if (object_methods && object_methods[0]) {
            result = retdec_call_thiscall1_result(object,
                reinterpret_cast<void*>(object_methods[0]), argument);
        }
    }
    return result;
}
} // namespace

// function_457a10
extern "C" int32_t __fastcall kinoko_method_update_children(int32_t receiver, void* /* unused_edx */,
    int32_t argument) {
    return update_mesh_children(reinterpret_cast<MeshChildRange*>(receiver), argument);
}




// function_45dbd0
extern "C" int32_t __fastcall kinoko_method_actor_move(int32_t receiver, void* /* unused_edx */,
    float dx, float dy) {
    return kinoko_actor_move(kinoko_game_collision_state(), reinterpret_cast<KinokoActor *>(receiver), dx, dy);
}

// function_45eb00
extern "C" int32_t __fastcall kinoko_actor_reset_method(int32_t receiver, void* /* unused_edx */) {
    return kinoko_actor_reset(reinterpret_cast<KinokoActor *>(receiver));
}

// function_466490
extern "C" int32_t __fastcall kinoko_method_class_type(int32_t receiver, void* /* unused_edx */) {
    // SqPlus ClassType::GetType returns the word at original offset +8.
    // memcpy preserves the x86 load for unaligned recovered object views.
    int32_t result;
    const auto* object = reinterpret_cast<const unsigned char*>(
        static_cast<uintptr_t>(static_cast<uint32_t>(receiver)));
    std::memcpy(&result, object + 8, sizeof(result));
    return result;
}

// function_469620
extern "C" int32_t __fastcall kinoko_method_render_layer_update(int32_t receiver, void* /* unused_edx */,
    int32_t argument) {
    return kinoko_actor_render_layer_update(reinterpret_cast<void *>(receiver), reinterpret_cast<KinokoCamera *>(argument));
}

// function_46a6f0
extern "C" int32_t __fastcall kinoko_method_actor_manager_remove(int32_t receiver,
    void* /* unused_edx */, uint32_t handle) {
    return kinoko_actor_pool_retire(reinterpret_cast<KinokoActorPool *>(receiver), handle);
}

// retdec_actor_manager_vtable_push
extern "C" int32_t __fastcall kinoko_method_actor_manager_push(int32_t receiver, void* /* unused_edx */) {
    return (int32_t)(intptr_t)kinoko_actor_owner_list_acquire(reinterpret_cast<KinokoActorManager *>(receiver));
}

// function_46ab10
extern "C" int32_t __fastcall kinoko_method_actor_manager_top(int32_t receiver, void* /* unused_edx */,
    int32_t output) {
    return (int32_t)(intptr_t)kinoko_actor_pool_request(reinterpret_cast<KinokoActorPool *>(receiver), reinterpret_cast<uint32_t *>(output));
}
