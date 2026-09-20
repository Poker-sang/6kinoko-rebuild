#include "kinoko/legacy_method_entries.h"
#include <cstring>

static_assert(sizeof(void*) == sizeof(int32_t), "Original Win32 object addresses");

// Only the ABI is adapted here. Recovered game/loading bodies remain the
// single source of truth; the adapters do not add ordering or fallback logic.
extern "C" {
int32_t retdec_destroy_cact_with_flags(int32_t receiver, unsigned char flags);
int32_t retdec_c2dlayout_set_layer_impl(int32_t receiver, int32_t layer);
int32_t retdec_c2dlayout_update_faithful_impl(int32_t receiver);
int32_t retdec_c2dlayout_draw_impl(int32_t receiver, float x, float y);
int32_t function_43c860_this(int32_t receiver, int32_t source, int32_t mode);
int32_t retdec_begin_stage_this(int32_t receiver, int32_t stage);
int32_t retdec_root_table_construct_this(int32_t receiver, int32_t vm, int32_t output);
int32_t retdec_act_bitblt_this(int32_t receiver, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t resource, int32_t source_x, int32_t source_y, int32_t blend, float alpha);
int32_t function_457a10_impl(int32_t receiver, int32_t argument);
int32_t function_45d970_this(int32_t receiver, char flags);
int32_t function_45dbd0_this(int32_t receiver, float dx, float dy);
int32_t function_45eb00_this(int32_t receiver);
int32_t function_469620_this(int32_t receiver, int32_t argument);
int32_t function_46a6f0_this(int32_t receiver, uint32_t handle);
int32_t function_46aa60_this(int32_t receiver);
int32_t function_46ab10_this(int32_t receiver, int32_t output);
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
    return function_43c860_this(receiver, source, mode);
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

// function_457a10
extern "C" int32_t __fastcall kinoko_method_update_children(int32_t receiver, void* /* unused_edx */,
    int32_t argument) {
    return function_457a10_impl(receiver, argument);
}

// function_45d970_bridge
extern "C" int32_t __fastcall kinoko_method_destroy_actor(int32_t receiver, void* /* unused_edx */,
    char flags) {
    return function_45d970_this(receiver, flags);
}

// function_45dbd0
extern "C" int32_t __fastcall kinoko_method_actor_move(int32_t receiver, void* /* unused_edx */,
    float dx, float dy) {
    return function_45dbd0_this(receiver, dx, dy);
}

// function_45eb00
extern "C" int32_t __fastcall kinoko_method_actor_destroy_state(int32_t receiver, void* /* unused_edx */) {
    return function_45eb00_this(receiver);
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
    return function_469620_this(receiver, argument);
}

// function_46a6f0
extern "C" int32_t __fastcall kinoko_method_actor_manager_remove(int32_t receiver,
    void* /* unused_edx */, uint32_t handle) {
    return function_46a6f0_this(receiver, handle);
}

// retdec_actor_manager_vtable_push
extern "C" int32_t __fastcall kinoko_method_actor_manager_push(int32_t receiver, void* /* unused_edx */) {
    return function_46aa60_this(receiver);
}

// function_46ab10
extern "C" int32_t __fastcall kinoko_method_actor_manager_top(int32_t receiver, void* /* unused_edx */,
    int32_t output) {
    return function_46ab10_this(receiver, output);
}
