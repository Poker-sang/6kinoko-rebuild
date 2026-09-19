#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/script_callbacks.h"

extern "C" { extern int32_t g600[3], g601[3], g602[3]; }

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
    {"Release", entry(&kinoko_actor_release), entry(&function_460b00)},
    {"Reset", entry(&kinoko_method_actor_destroy_state), entry(&function_460b00)},
    {"SetUpdateFunction", entry(&kinoko_actor_set_update_callback), entry(&function_460b50)},
    {"SetCollisionCallbackFunction", entry(&kinoko_actor_set_collision_callback), entry(&function_460b50)},
    {"InterrputCollisionCallback", entry(&kinoko_actor_interrupt_collision), entry(&function_460b00)},
    {"SetTake", entry(&kinoko_actor_set_take_method), entry(&function_460bc0)},
    {"SetStep", entry(&function_4606d0), entry(&function_460b50)},
    {"SetChipFlag", entry(&kinoko_actor_set_chip_flags), entry(&function_460bc0)},
    {"SetChipBoundType", entry(&kinoko_actor_set_chip_bound_type), entry(&function_460bc0)},
    {"GetChipID", entry(&kinoko_actor_get_chip_id), entry(&function_460bc0)},
    {"GetChipFlag", entry(&kinoko_actor_get_chip_flags), entry(&function_460c10)},
    {"IsExistChip", entry(&kinoko_actor_has_chip), entry(&function_460c70)},
    {"ResetPriority", entry(&kinoko_actor_reset_priority_method), entry(&function_460bc0)},
    {"SyncAnimation", entry(&kinoko_actor_sync_animation), entry(&function_460b50)},
    {"Move", entry(&kinoko_method_actor_move), entry(&function_460cc0)},
};
constexpr ActorField fields[] = {
    {"timeTotal", 220, FieldKind::Integer, 0},
    {"px", 156, FieldKind::Float, 0},
    {"py", 160, FieldKind::Float, 0},
    {"sx", 172, FieldKind::Float, 0},
    {"sy", 176, FieldKind::Float, 0},
    {"rotate", 164, FieldKind::Float, 0},
    {"scale", 168, FieldKind::Float, 0},
    {"a", 180, FieldKind::Integer, 0},
    {"r", 184, FieldKind::Integer, 0},
    {"g", 188, FieldKind::Integer, 0},
    {"b", 192, FieldKind::Integer, 0},
    {"blend", 196, FieldKind::Integer, 0},
    {"take", 208, FieldKind::Integer, 1},
    {"id", 224, FieldKind::Integer, 0},
    {"priority", 228, FieldKind::Integer, 0},
    {"updateGroup", 232, FieldKind::Integer, 0},
    {"flag", 236, FieldKind::Integer, 0},
    {"isActive", 40, FieldKind::Boolean, 0},
    {"isStatic", 40, FieldKind::Boolean, 0},
    {"isVisible", 21, FieldKind::Boolean, 0},
    {"ox", 80, FieldKind::Float, 0},
    {"oy", 84, FieldKind::Float, 0},
    {"x", 240, FieldKind::Float, 0},
    {"y", 244, FieldKind::Float, 0},
    {"freeWidth", 304, FieldKind::Float, 0},
    {"freeHeight", 308, FieldKind::Float, 0},
    {"direction", 272, FieldKind::Float, 0},
    {"pitch", 276, FieldKind::Float, 0},
    {"pitchTop", 280, FieldKind::Float, 0},
    {"vx", 256, FieldKind::Float, 0},
    {"vy", 260, FieldKind::Float, 0},
    {"collisionFlag", 472, FieldKind::Integer, 0},
    {"hitLeft", 284, FieldKind::Integer, 0},
    {"hitRight", 292, FieldKind::Integer, 0},
    {"hitTop", 288, FieldKind::Integer, 0},
    {"hitBottom", 296, FieldKind::Integer, 0},
    {"left", 440, FieldKind::Float, 0},
    {"top", 444, FieldKind::Float, 0},
    {"right", 448, FieldKind::Float, 0},
    {"bottom", 452, FieldKind::Float, 0},
    {"xPrev", 248, FieldKind::Float, 0},
    {"yPrev", 252, FieldKind::Float, 0},
    {"leftPrev", 456, FieldKind::Float, 0},
    {"topPrev", 460, FieldKind::Float, 0},
    {"rightPrev", 464, FieldKind::Float, 0},
    {"bottomPrev", 468, FieldKind::Float, 0},
    {"collisionGroup", 312, FieldKind::Integer, 0},
    {"collisionMask", 316, FieldKind::Integer, 0},
    {"callbackGroup", 320, FieldKind::Integer, 0},
    {"callbackMask", 324, FieldKind::Integer, 0},
};
} // namespace

extern "C" int32_t function_460e00(void) {
    int32_t actor[3]{};
    function_460d10_actor(actor, "Actor", 0);
    function_4a95c0_this(address(g602), address(actor));
    for (const auto& method : methods)
        function_460e00_register_actor_method(address(g644), actor,
            method.name, method.target, method.wrapper, 0);
    auto* descriptor = kinoko_native_binding_type(-1);
    for (const auto& field : fields) {
        auto bind = field.kind == FieldKind::Integer ? function_460920 :
            field.kind == FieldKind::Float ? function_4609c0 : function_460a60;
        bind(actor, descriptor, field.offset, const_cast<char*>(field.name), field.flags);
    }
    // Construct OT_NULL explicitly; zero-filled object storage is not OT_NULL.
    int32_t null_object[3]{}, key[3]{};
    function_4a94e0_this(address(null_object));
    function_4a95c0_this(address(g601), function_4a9250(address(key), address("step")));
    function_4a9d70_this(address(key));
    function_4a95c0_this(address(g600), function_4a9250(address(key), address("user")));
    function_4a9d70_this(address(key));
    function_4a97b0_this(address(g602), address(g601), address(null_object));
    function_4a97b0_this(address(g602), address(g600), address(null_object));
    function_4a9d70_this(address(null_object));
    return function_4a9d70_this(address(actor));
}
