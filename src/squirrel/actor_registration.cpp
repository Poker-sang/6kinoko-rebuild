#include "kinoko/actor_records.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/actor_manager.h"


namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class Function> int32_t entry(Function function) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(function));
}
struct ActorMethod { const char* name; int32_t target; int32_t wrapper; };
enum class FieldKind { Integer, Float, Boolean };
struct ActorField { const char* name; int32_t offset; FieldKind kind; int32_t flags; };
// Original 460E00 registration order and storage offsets. In particular,
// isActive/isStatic share one byte; InterrputCollisionCallback is not a typo here.
const ActorMethod methods[] = {
    {"Release", entry(&kinoko_actor_release), entry(&kinoko_sqplus_void_method)},
    {"Reset", entry(&kinoko_actor_reset_method), entry(&kinoko_sqplus_void_method)},
    {"SetUpdateFunction", entry(&kinoko_actor_set_update_callback), entry(&kinoko_sqplus_object_method)},
    {"SetCollisionCallbackFunction", entry(&kinoko_actor_set_collision_callback), entry(&kinoko_sqplus_object_method)},
    {"InterrputCollisionCallback", entry(&kinoko_actor_interrupt_collision), entry(&kinoko_sqplus_void_method)},
    {"SetTake", entry(&kinoko_actor_set_take_method), entry(&kinoko_sqplus_integer_method)},
    {"SetStep", entry(&kinoko_actor_set_step_method), entry(&kinoko_sqplus_object_method)},
    {"SetChipFlag", entry(&kinoko_actor_set_chip_flags), entry(&kinoko_sqplus_integer_method)},
    {"SetChipBoundType", entry(&kinoko_actor_set_chip_bound_type), entry(&kinoko_sqplus_integer_method)},
    {"GetChipID", entry(&kinoko_actor_get_chip_id), entry(&kinoko_sqplus_integer_method)},
    {"GetChipFlag", entry(&kinoko_actor_get_chip_flags), entry(&kinoko_sqplus_integer_result_method)},
    {"IsExistChip", entry(&kinoko_actor_has_chip), entry(&kinoko_sqplus_rectangle_method)},
    {"ResetPriority", entry(&kinoko_actor_reset_priority_method), entry(&kinoko_sqplus_integer_method)},
    {"SyncAnimation", entry(&kinoko_actor_sync_animation), entry(&kinoko_sqplus_object_method)},
    {"Move", entry(&kinoko_method_actor_move), entry(&kinoko_sqplus_move_method)},
};
constexpr ActorField fields[] = {
    {"timeTotal", offsetof(kinoko::actor::ActorRecord, take_duration), FieldKind::Integer, 0},
    {"px", offsetof(kinoko::actor::ActorRecord, offset_x), FieldKind::Float, 0},
    {"py", offsetof(kinoko::actor::ActorRecord, offset_y), FieldKind::Float, 0},
    {"sx", offsetof(kinoko::actor::ActorRecord, scale_x), FieldKind::Float, 0},
    {"sy", offsetof(kinoko::actor::ActorRecord, scale_y), FieldKind::Float, 0},
    {"rotate", offsetof(kinoko::actor::ActorRecord, rotation), FieldKind::Float, 0},
    {"scale", offsetof(kinoko::actor::ActorRecord, scale), FieldKind::Float, 0},
    {"a", offsetof(kinoko::actor::ActorRecord, alpha), FieldKind::Integer, 0},
    {"r", offsetof(kinoko::actor::ActorRecord, red), FieldKind::Integer, 0},
    {"g", offsetof(kinoko::actor::ActorRecord, green), FieldKind::Integer, 0},
    {"b", offsetof(kinoko::actor::ActorRecord, blue), FieldKind::Integer, 0},
    {"blend", offsetof(kinoko::actor::ActorRecord, blend), FieldKind::Integer, 0},
    {"take", offsetof(kinoko::actor::ActorRecord, take), FieldKind::Integer, 1},
    {"id", offsetof(kinoko::actor::ActorRecord, id), FieldKind::Integer, 0},
    {"priority", offsetof(kinoko::actor::ActorRecord, priority), FieldKind::Integer, 0},
    {"updateGroup", offsetof(kinoko::actor::ActorRecord, update_group), FieldKind::Integer, 0},
    {"flag", offsetof(kinoko::actor::ActorRecord, flags), FieldKind::Integer, 0},
    {"isActive", offsetof(kinoko::actor::ActorRecord, active), FieldKind::Boolean, 0},
    {"isStatic", offsetof(kinoko::actor::ActorRecord, active), FieldKind::Boolean, 0},
    {"isVisible", offsetof(kinoko::actor::ActorRecord, visible), FieldKind::Boolean, 0},
    {"ox", offsetof(kinoko::actor::ActorRecord, spawn_x), FieldKind::Float, 0},
    {"oy", offsetof(kinoko::actor::ActorRecord, spawn_y), FieldKind::Float, 0},
    {"x", offsetof(kinoko::actor::ActorRecord, x), FieldKind::Float, 0},
    {"y", offsetof(kinoko::actor::ActorRecord, y), FieldKind::Float, 0},
    {"freeWidth", offsetof(kinoko::actor::ActorRecord, free_width), FieldKind::Float, 0},
    {"freeHeight", offsetof(kinoko::actor::ActorRecord, free_height), FieldKind::Float, 0},
    {"direction", offsetof(kinoko::actor::ActorRecord, direction), FieldKind::Float, 0},
    {"pitch", offsetof(kinoko::actor::ActorRecord, pitch), FieldKind::Float, 0},
    {"pitchTop", offsetof(kinoko::actor::ActorRecord, pitch_top), FieldKind::Float, 0},
    {"vx", offsetof(kinoko::actor::ActorRecord, velocity_x), FieldKind::Float, 0},
    {"vy", offsetof(kinoko::actor::ActorRecord, velocity_y), FieldKind::Float, 0},
    {"collisionFlag", offsetof(kinoko::actor::ActorRecord, collision_flags), FieldKind::Integer, 0},
    {"hitLeft", offsetof(kinoko::actor::ActorRecord, hits) + 0 * sizeof(int32_t), FieldKind::Integer, 0},
    {"hitRight", offsetof(kinoko::actor::ActorRecord, hits) + 2 * sizeof(int32_t), FieldKind::Integer, 0},
    {"hitTop", offsetof(kinoko::actor::ActorRecord, hits) + 1 * sizeof(int32_t), FieldKind::Integer, 0},
    {"hitBottom", offsetof(kinoko::actor::ActorRecord, hits) + 3 * sizeof(int32_t), FieldKind::Integer, 0},
    {"left", offsetof(kinoko::actor::ActorRecord, world_bounds) + offsetof(kinoko::actor::Bounds, left), FieldKind::Float, 0},
    {"top", offsetof(kinoko::actor::ActorRecord, world_bounds) + offsetof(kinoko::actor::Bounds, top), FieldKind::Float, 0},
    {"right", offsetof(kinoko::actor::ActorRecord, world_bounds) + offsetof(kinoko::actor::Bounds, right), FieldKind::Float, 0},
    {"bottom", offsetof(kinoko::actor::ActorRecord, world_bounds) + offsetof(kinoko::actor::Bounds, bottom), FieldKind::Float, 0},
    {"xPrev", offsetof(kinoko::actor::ActorRecord, previous_x), FieldKind::Float, 0},
    {"yPrev", offsetof(kinoko::actor::ActorRecord, previous_y), FieldKind::Float, 0},
    {"leftPrev", offsetof(kinoko::actor::ActorRecord, previous_bounds) + offsetof(kinoko::actor::Bounds, left), FieldKind::Float, 0},
    {"topPrev", offsetof(kinoko::actor::ActorRecord, previous_bounds) + offsetof(kinoko::actor::Bounds, top), FieldKind::Float, 0},
    {"rightPrev", offsetof(kinoko::actor::ActorRecord, previous_bounds) + offsetof(kinoko::actor::Bounds, right), FieldKind::Float, 0},
    {"bottomPrev", offsetof(kinoko::actor::ActorRecord, previous_bounds) + offsetof(kinoko::actor::Bounds, bottom), FieldKind::Float, 0},
    {"collisionGroup", offsetof(kinoko::actor::ActorRecord, collision_group), FieldKind::Integer, 0},
    {"collisionMask", offsetof(kinoko::actor::ActorRecord, collision_mask), FieldKind::Integer, 0},
    {"callbackGroup", offsetof(kinoko::actor::ActorRecord, callback_group), FieldKind::Integer, 0},
    {"callbackMask", offsetof(kinoko::actor::ActorRecord, callback_mask), FieldKind::Integer, 0},
};
} // namespace

extern "C" int32_t kinoko_actor_register_script_class(void) {
    int32_t actor[3]{};
    kinoko_sqplus_define_actor_class(actor, "Actor", 0);
    kinoko_sqplus_object_assign((void *)(const_cast<void *>(kinoko_actor_class_object())), (const void *)(actor));
    for (const auto& method : methods)
        kinoko_sqplus_register_actor_method(kinoko_actor_default_vm(), actor, method.name, (void *)(intptr_t)(method.target), (void *)(intptr_t)(method.wrapper), 0);
    auto* descriptor = kinoko_native_binding_type(-1);
    for (const auto& field : fields) {
        auto bind = field.kind == FieldKind::Integer ? kinoko_sqplus_bind_integer :
            field.kind == FieldKind::Float ? kinoko_sqplus_bind_float : kinoko_sqplus_bind_boolean;
        bind(actor, descriptor, field.offset, const_cast<char*>(field.name), field.flags);
    }
    // Construct OT_NULL explicitly; zero-filled object storage is not OT_NULL.
    int32_t null_object[3]{}, key[3]{};
    kinoko_sqplus_object_initialize((void *)(null_object));
    kinoko_sqplus_object_assign((void *)(const_cast<void *>(kinoko_actor_step_key())), kinoko_sqplus_new_string((void *)(key), (const char *)("step")));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(key)));
    kinoko_sqplus_object_assign((void *)(const_cast<void *>(kinoko_actor_user_key())), kinoko_sqplus_new_string((void *)(key), (const char *)("user")));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(key)));
    kinoko_sqplus_object_raw_set_object((void *)(const_cast<void *>(kinoko_actor_class_object())), (const void *)(g601), (const void *)(null_object));
    kinoko_sqplus_object_raw_set_object((void *)(const_cast<void *>(kinoko_actor_class_object())), (const void *)(g600), (const void *)(null_object));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(null_object)));
    return (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(actor)));
}
