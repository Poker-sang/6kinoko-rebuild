#include "kinoko/act_layout_render.hpp"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/game_script_host.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/act_layout3d_io.h"
#include "kinoko/legacy_abi.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace {
constexpr int32_t return_value = 0x13579bdf;
std::array<uint32_t, 12> observed{};
std::size_t observed_count = 0;
int observed_entry = 0;
template<class T> uint32_t bits(T* value) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)); }
uint32_t bits(float value) { uint32_t result; std::memcpy(&result, &value, 4); return result; }
uint32_t bits(int32_t value) { return static_cast<uint32_t>(value); }
uint32_t bits(uint32_t value) { return value; }
uint32_t bits(char value) { return static_cast<uint32_t>(static_cast<int32_t>(value)); }
uint32_t bits(unsigned char value) { return value; }
int32_t record(int entry, std::initializer_list<uint32_t> arguments) {
    observed_entry = entry;
    observed_count = arguments.size();
    std::size_t index = 0;
    for (auto value : arguments) observed[index++] = value;
    return return_value;
}
bool check(int entry, int32_t result, std::initializer_list<uint32_t> expected) {
    if (observed_entry != entry || result != return_value || observed_count != expected.size()) return false;
    std::size_t index = 0;
    for (auto value : expected) if (observed[index++] != value) return false;
    return true;
}
}

// Stub only the original C bodies. Calls below execute the real C++ entry
// adapters via legacy_abi.cpp's __thiscall invocations (no inline assembly).

extern "C" void* kinoko_destroy_cact_with_flags(KinokoActDocument* receiver, unsigned char flags) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(record(1, {bits(receiver), bits(flags)})));
}

int32_t kinoko::act::bind_layout_2d(KinokoActLayout* receiver, KinokoActLayer* layer) {
    return record(2, {bits(receiver), bits(layer)});
}

int32_t kinoko::act::update_layout_2d(KinokoActLayout* receiver) {
    return record(3, {bits(receiver)});
}

int32_t kinoko::act::draw_layout_2d(KinokoActLayout* receiver, float x, float y) {
    return record(4, {bits(receiver), bits(x), bits(y)});
}

extern "C" int32_t kinoko_act_read_layout3d_properties(KinokoActLayout *receiver,
                                                         KinokoArchiveReader** source, int32_t mode) {
    return record(5, {static_cast<uint32_t>(reinterpret_cast<uintptr_t>(receiver)),
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source)), bits(mode)});
}

extern "C" int32_t kinoko_begin_stage_this(KinokoActRuntime* receiver, int32_t stage) {
    return record(6, {bits(receiver), bits(stage)});
}

extern "C" int32_t kinoko_root_table_construct_this(KinokoActRuntime* receiver, struct SQVM* vm, void* output) {
    return record(7, {bits(receiver), bits(vm), bits(output)});
}

extern "C" int32_t kinoko_act_append_blit(KinokoActRuntime* receiver, int32_t x, int32_t y, int32_t width,
    int32_t height, KinokoActResource* resource, int32_t source_x, int32_t source_y, int32_t blend, float alpha) {
    return record(8, {bits(receiver), bits(x), bits(y), bits(width), bits(height), bits(resource),
            bits(source_x), bits(source_y), bits(blend), bits(alpha)});
}

extern "C" int32_t kinoko_update_mesh_children(void* node, int32_t argument) {
    return record(9, {bits((int32_t)(intptr_t)node), bits(argument)});
}



extern "C" KinokoCollisionState *kinoko_game_collision_state(void) { return nullptr; }
extern "C" int32_t kinoko_actor_move(KinokoCollisionState *, KinokoActor *actor, float dx, float dy) {
    const auto receiver=(int32_t)(intptr_t)actor;
    return record(11, {bits(receiver), bits(dx), bits(dy)});
}

extern "C" int32_t kinoko_actor_reset(KinokoActor *actor) {
    const auto receiver=(int32_t)(intptr_t)actor;
    return record(12, {bits(receiver)});
}

extern "C" int32_t kinoko_actor_render_layer_update(void *layer, KinokoCamera *camera) {
    auto receiver=(int32_t)(intptr_t)layer; auto argument=(int32_t)(intptr_t)camera;
    return record(14, {bits(receiver), bits(argument)});
}

extern "C" int32_t kinoko_actor_pool_retire(KinokoActorPool *pool, uint32_t handle) {
    auto receiver=(int32_t)(intptr_t)pool;
    return record(15, {bits(receiver), bits(handle)});
}

extern "C" KinokoActor *kinoko_actor_owner_list_acquire(KinokoActorManager *manager) {
    const auto receiver=(int32_t)(intptr_t)manager;
    return reinterpret_cast<KinokoActor *>(record(16, {bits(receiver)}));
}

extern "C" KinokoActor *kinoko_actor_pool_request(KinokoActorPool *pool, uint32_t *handle) {
    auto receiver=(int32_t)(intptr_t)pool; auto output=(int32_t)(intptr_t)handle;
    return reinterpret_cast<KinokoActor *>(record(17, {bits(receiver), bits(output)}));
}

int main() {
    alignas(4) std::array<unsigned char, 16> object{};
    const int32_t type = 0x2468ace;
    // Exercise the source getter on an unaligned recovered view as well.
    std::memcpy(object.data() + 9, &type, 4);
    void* receiver = object.data() + 1;
    const uint32_t receiver_bits = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(receiver));
    for (int iteration = 0; iteration < 10000; ++iteration) {
        if (!check(1, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_destroy_act),
            static_cast<int32_t>(bits(static_cast<unsigned char>(0xa5)))), {receiver_bits,
            bits(static_cast<unsigned char>(0xa5))})) {
            std::fprintf(stderr, "Entry contract failed: retdec_cact_destructor_bridge\n"); return 1;
        }
        if (!check(2, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_layout_set_layer), static_cast<int32_t>(bits(-31))),
            {receiver_bits, bits(-31)})) {
            std::fprintf(stderr, "Entry contract failed: function_42bcc0\n"); return 1;
        }
        if (!check(3, kinoko_call_thiscall0_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_layout_update)), {receiver_bits})) {
            std::fprintf(stderr, "Entry contract failed: function_42c100\n"); return 1;
        }
        if (!check(4, kinoko_call_thiscall2_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_layout_draw), static_cast<int32_t>(bits(-3.25f)),
            static_cast<int32_t>(bits(7.75f))), {receiver_bits, bits(-3.25f), bits(7.75f)})) {
            std::fprintf(stderr, "Entry contract failed: function_42c300\n"); return 1;
        }
        if (!check(5, kinoko_call_thiscall2_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_layout3d_assign), static_cast<int32_t>(bits(-31)),
            static_cast<int32_t>(bits(-14))), {receiver_bits, bits(-31), bits(-14)})) {
            std::fprintf(stderr, "Entry contract failed: function_43c860\n"); return 1;
        }
        if (!check(6, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_begin_stage), static_cast<int32_t>(bits(-31))),
            {receiver_bits, bits(-31)})) {
            std::fprintf(stderr, "Entry contract failed: function_450950\n"); return 1;
        }
        if (!check(7, kinoko_call_thiscall2_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_root_table_construct),
            static_cast<int32_t>(bits(-31)), static_cast<int32_t>(bits(-14))), {receiver_bits, bits(-31),
            bits(-14)})) {
            std::fprintf(stderr, "Entry contract failed: function_450e30\n"); return 1;
        }
        if (!check(8, kinoko_call_draw_method(receiver,
            reinterpret_cast<void*>(&kinoko_method_act_bitblt), (-31), (-14), (3), (20), (37), (54),
            (71), (88), (7.75f)), {receiver_bits, bits(-31), bits(-14), bits(3), bits(20), bits(37),
            bits(54), bits(71), bits(88), bits(7.75f)})) {
            std::fprintf(stderr, "Entry contract failed: function_4514a0\n"); return 1;
        }
        if (!check(9, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_update_children), static_cast<int32_t>(bits(-31))),
            {receiver_bits, bits(-31)})) {
            std::fprintf(stderr, "Entry contract failed: function_457a10\n"); return 1;
        }
        if (!check(11, kinoko_call_thiscall2_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_actor_move), static_cast<int32_t>(bits(-3.25f)),
            static_cast<int32_t>(bits(7.75f))), {receiver_bits, bits(-3.25f), bits(7.75f)})) {
            std::fprintf(stderr, "Entry contract failed: function_45dbd0\n"); return 1;
        }
        if (!check(12, kinoko_call_thiscall0_result(receiver,
            reinterpret_cast<void*>(&kinoko_actor_reset_method)), {receiver_bits})) {
            std::fprintf(stderr, "Entry contract failed: function_45eb00\n"); return 1;
        }
        if (!check(14, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_render_layer_update),
            static_cast<int32_t>(bits(-31))), {receiver_bits, bits(-31)})) {
            std::fprintf(stderr, "Entry contract failed: function_469620\n"); return 1;
        }
        if (!check(15, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_actor_manager_remove),
            static_cast<int32_t>(bits(0x89abcdefu))), {receiver_bits, bits(0x89abcdefu)})) {
            std::fprintf(stderr, "Entry contract failed: function_46a6f0\n"); return 1;
        }
        if (!check(16, kinoko_call_thiscall0_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_actor_manager_push)), {receiver_bits})) {
            std::fprintf(stderr, "Entry contract failed: retdec_actor_manager_vtable_push\n"); return 1;
        }
        if (!check(17, kinoko_call_thiscall1_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_actor_manager_top), static_cast<int32_t>(bits(-31))),
            {receiver_bits, bits(-31)})) {
            std::fprintf(stderr, "Entry contract failed: function_46ab10\n"); return 1;
        }
        if (kinoko_call_thiscall0_result(receiver,
            reinterpret_cast<void*>(&kinoko_method_class_type)) != type) return 1;
    }
    std::puts("PASS: all migrated x86 entries, receiver, float bits, argument order, result and stack cleanup");
}
