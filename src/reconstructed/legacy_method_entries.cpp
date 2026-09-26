#include "kinoko/act_frame.h"
#include "kinoko/act_layout_render.hpp"
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
void* kinoko_destroy_cact_with_flags(KinokoActDocument* receiver, unsigned char flags);
int32_t kinoko_begin_stage_this(KinokoActRuntime* receiver, int32_t stage);
int32_t kinoko_root_table_construct_this(KinokoActRuntime* receiver, struct SQVM* vm, void* output);
int32_t kinoko_update_mesh_children(void* receiver, int32_t argument);






}

// retdec_cact_destructor_bridge
extern "C" void* __fastcall kinoko_method_destroy_act(KinokoActDocument* receiver, void* /* unused_edx */,
    unsigned char flags) {
    return kinoko_destroy_cact_with_flags(receiver, flags);
}

// function_42bcc0
extern "C" int32_t __fastcall kinoko_method_layout_set_layer(KinokoActLayout* receiver, void* /* unused_edx */,
    KinokoActLayer* layer) {
    return kinoko::act::bind_layout_2d(receiver, layer);
}

// function_42c100
extern "C" int32_t __fastcall kinoko_method_layout_update(KinokoActLayout* receiver, void* /* unused_edx */) {
    return kinoko::act::update_layout_2d(receiver);
}

// function_42c300
extern "C" int32_t __fastcall kinoko_method_layout_draw(KinokoActLayout* receiver, void* /* unused_edx */,
    float x, float y) {
    return kinoko::act::draw_layout_2d(receiver, x, y);
}

// function_43c860
extern "C" int32_t __fastcall kinoko_method_layout3d_assign(KinokoActLayout* receiver, void* /* unused_edx */,
    KinokoArchiveReader** source, int32_t mode) {
    return kinoko_act_read_layout3d_properties(receiver, source, mode);
}

// function_450950
extern "C" int32_t __fastcall kinoko_method_begin_stage(KinokoActRuntime* receiver, void* /* unused_edx */,
    int32_t stage) {
    return kinoko_begin_stage_this((KinokoActRuntime*)(uintptr_t)(receiver), stage);
}

// function_450e30
extern "C" int32_t __fastcall kinoko_method_root_table_construct(KinokoActRuntime* receiver,
    void* /* unused_edx */, struct SQVM* vm, void* output) {
    return kinoko_root_table_construct_this((KinokoActRuntime*)(uintptr_t)(receiver), vm, (void*)(uintptr_t)(output));
}

// function_4514a0
extern "C" int32_t __fastcall kinoko_method_act_bitblt(KinokoActRuntime* receiver, void* /* unused_edx */,
    int32_t x, int32_t y, int32_t width, int32_t height, KinokoActResource* resource, int32_t source_x,
    int32_t source_y, int32_t blend, float alpha) {
    return kinoko_act_append_blit(receiver, x, y, width, height, resource, source_x, source_y, blend, alpha);
}

// function_457a10
extern "C" int32_t __fastcall kinoko_method_update_children(void* receiver, void* /* unused_edx */,
    int32_t argument) {
    return kinoko_update_mesh_children(reinterpret_cast<void*>(receiver), argument);
}




// function_45dbd0
extern "C" int32_t __fastcall kinoko_method_actor_move(struct KinokoActor* receiver, void* /* unused_edx */,
    float dx, float dy) {
    return kinoko_actor_move(kinoko_game_collision_state(), reinterpret_cast<KinokoActor *>(receiver), dx, dy);
}

// function_45eb00
extern "C" int32_t __fastcall kinoko_actor_reset_method(struct KinokoActor* receiver, void* /* unused_edx */) {
    return kinoko_actor_reset(reinterpret_cast<KinokoActor *>(receiver));
}

// function_466490
extern "C" int32_t __fastcall kinoko_method_class_type(const void* receiver, void* /* unused_edx */) {
    // SqPlus ClassType::GetType returns the word at original offset +8.
    // memcpy preserves the x86 load for unaligned recovered object views.
    int32_t result;
    const auto* object = reinterpret_cast<const unsigned char*>(
        receiver);
    std::memcpy(&result, object + 8, sizeof(result));
    return result;
}

// function_469620
extern "C" int32_t __fastcall kinoko_method_render_layer_update(void* receiver, void* /* unused_edx */,
    struct KinokoCamera* argument) {
    return kinoko_actor_render_layer_update(reinterpret_cast<void *>(receiver), reinterpret_cast<KinokoCamera *>(argument));
}

// function_46a6f0
extern "C" int32_t __fastcall kinoko_method_actor_manager_remove(struct KinokoActorPool* receiver,
    void* /* unused_edx */, uint32_t handle) {
    return kinoko_actor_pool_retire(reinterpret_cast<KinokoActorPool *>(receiver), handle);
}

// retdec_actor_manager_vtable_push
extern "C" struct KinokoActor* __fastcall kinoko_method_actor_manager_push(struct KinokoActorManager* receiver, void* /* unused_edx */) {
    return kinoko_actor_owner_list_acquire(reinterpret_cast<KinokoActorManager *>(receiver));
}

// function_46ab10
extern "C" struct KinokoActor* __fastcall kinoko_method_actor_manager_top(struct KinokoActorPool* receiver, void* /* unused_edx */,
    uint32_t* output) {
    return kinoko_actor_pool_request(reinterpret_cast<KinokoActorPool *>(receiver), reinterpret_cast<uint32_t *>(output));
}
