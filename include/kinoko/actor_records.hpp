#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace kinoko::actor {
using Address = std::uint32_t; // serialized native Win32 address, never an owner
using ScriptStorage = std::array<unsigned char, 12>; // external refs: ObjectView
struct Bounds { float left, top, right, bottom; };
struct InitialData {
    std::array<unsigned char, 12> unknown;
    std::int16_t width, height;
    std::int64_t chip_flags;
    std::array<unsigned char, 10> unknown24;
    std::uint16_t chip_bound_type;
    std::array<unsigned char, 12> unknown36;
};
// One schema shared by constructor, callbacks, animation and actor methods.
// Opaque bytes are deliberately not assigned speculative semantics.
struct ActorRecord {
    Address vtable;
    std::array<unsigned char, 4> unknown4;
    std::int32_t type;
    std::array<unsigned char, 8> unknown12;
    std::uint8_t registration_flag20, visible;
    std::uint8_t release_pending, unknown23;
    Address owner, owner_control; // strong native control; not a VM reference
    Address step, step_control;   // weak native control
    std::uint8_t active;
    std::array<unsigned char, 3> unknown41;
    ScriptStorage script_object, update_callback, collision_callback;
    std::array<unsigned char, 16> unknown80;
    ScriptStorage object96, object108;
    std::array<unsigned char, 4> unknown120;
    ScriptStorage object124, object136;
    Address manager;              // borrowed; manager owns the live Actor set
    Address sprite_frame;         // borrowed from manager-owned animation frames
    std::array<unsigned char, 12> unknown156;
    float scale, scale_x, scale_y;
    std::array<unsigned char, 20> unknown180;
    Address animation, current_frame; // borrowed, never freed by Actor
    std::int32_t take, frame_index, frame_time, animation_flags;
    std::array<unsigned char, 4> unknown224;
    std::int32_t priority;
    std::uint32_t update_group, flags;
    float x, y;
    std::array<unsigned char, 24> unknown248;
    float direction;
    std::array<unsigned char, 36> unknown276;
    std::uint32_t collision_group, collision_mask, callback_group, callback_mask;
    Address collision_records, collision_slots;
    std::int32_t collision_index;
    std::array<unsigned char, 12> inline_slots;
    float bounds_anchor_x, bounds_anchor_y;
    std::array<unsigned char, 16> unknown360;
    InitialData initial;
    Bounds local_bounds, world_bounds;
    Bounds previous_bounds;
    std::uint32_t collision_flags, unknown476;
    std::array<int32_t, 8> collision_scan_cache;
    // Remaining storage begins with the layer cache. No new bound check or
    // guessed supported layer count is imposed on the original GetChipID ABI.
    std::array<unsigned char, 32> chip_cache_storage;
};
using native::ControlRecord;
using native::ControlTable;
struct AnimationRecord {
    std::array<unsigned char, 8> unknown0;
    Address frames_begin, frames_end;
    std::array<unsigned char, 8> unknown16;
    std::uint8_t loops, has_bounds;
    std::array<unsigned char, 2> unknown26;
    std::int32_t left, top, right, bottom, flags;
};
struct FrameRecord {
    Address vtable;
    std::int32_t texture; // borrowed handle; manager releases texture owners
    std::array<unsigned char, 232> drawing_data;
    std::int16_t duration;
    std::uint16_t unknown242;
    Address owned_payload; // malloc-owned, released before frame storage
};
struct TreeIndex {
    Address policy, head;
    std::int32_t count;
};
struct ListIndex { Address head; std::uint32_t count; };
struct VectorIndex { Address begin, end, capacity; };
// Only the verified prefix of the manager is described, not a new allocation
// size. Index nodes and animation lists have distinct ownership semantics.
struct ManagerPrefix {
    std::array<unsigned char, 36> unknown0;
    TreeIndex animation_lookup; // owns nodes; values borrow animation list items
    std::array<unsigned char, 4> unknown48;
    ListIndex animations;       // owns animation items, frames and payloads
    std::array<unsigned char, 8> unknown60;
    VectorIndex textures;      // owns texture handles, retains vector capacity
    std::array<unsigned char, 4> unknown80;
    TreeIndex actors;          // live actor/priority tree
    std::array<unsigned char, 4> unknown96;
    VectorIndex iteration;    // transient borrowed actor addresses
    std::array<unsigned char, 4> unknown112;
    std::int32_t iteration_state;
    std::uint8_t cleanup_pending;
    std::array<unsigned char, 3> unknown121;
};
using ActorView = native::RecordView<ActorRecord>;
using ManagerView = native::RecordView<ManagerPrefix>;
inline constexpr std::array<ScriptStorage ActorRecord::*, 7> script_members{
    &ActorRecord::script_object, &ActorRecord::update_callback,
    &ActorRecord::collision_callback, &ActorRecord::object96,
    &ActorRecord::object108, &ActorRecord::object124, &ActorRecord::object136};

static_assert(sizeof(InitialData) == 48 && sizeof(ActorRecord) == 0x220);
static_assert(sizeof(ControlRecord) == 16 && sizeof(ControlTable) == 12);
static_assert(sizeof(AnimationRecord) == 48 && sizeof(FrameRecord) == 248);
static_assert(sizeof(ManagerPrefix) == 124);
#define KINOKO_ACTOR_FIELD(T, M, O) static_assert(offsetof(T, M) == O)
KINOKO_ACTOR_FIELD(ActorRecord, type, 8);
KINOKO_ACTOR_FIELD(ActorRecord, registration_flag20, 20);
KINOKO_ACTOR_FIELD(ActorRecord, active, 40);
KINOKO_ACTOR_FIELD(ActorRecord, update_group, 232);
KINOKO_ACTOR_FIELD(ActorRecord, collision_group, 312);
KINOKO_ACTOR_FIELD(ActorRecord, collision_mask, 316);
KINOKO_ACTOR_FIELD(ActorRecord, collision_index, 336);
KINOKO_ACTOR_FIELD(ActorRecord, collision_scan_cache, 480);
KINOKO_ACTOR_FIELD(ActorRecord, release_pending, 22);
KINOKO_ACTOR_FIELD(ActorRecord, owner, 24);
KINOKO_ACTOR_FIELD(ActorRecord, owner_control, 28);
KINOKO_ACTOR_FIELD(ActorRecord, step, 32);
KINOKO_ACTOR_FIELD(ActorRecord, step_control, 36);
KINOKO_ACTOR_FIELD(ActorRecord, script_object, 44);
KINOKO_ACTOR_FIELD(ActorRecord, update_callback, 56);
KINOKO_ACTOR_FIELD(ActorRecord, collision_callback, 68);
KINOKO_ACTOR_FIELD(ActorRecord, object96, 96);
KINOKO_ACTOR_FIELD(ActorRecord, object108, 108);
KINOKO_ACTOR_FIELD(ActorRecord, object124, 124);
KINOKO_ACTOR_FIELD(ActorRecord, object136, 136);
KINOKO_ACTOR_FIELD(ActorRecord, manager, 148);
KINOKO_ACTOR_FIELD(ActorRecord, sprite_frame, 152);
KINOKO_ACTOR_FIELD(ActorRecord, scale, 168);
KINOKO_ACTOR_FIELD(ActorRecord, animation, 200);
KINOKO_ACTOR_FIELD(ActorRecord, frame_time, 216);
KINOKO_ACTOR_FIELD(ActorRecord, priority, 228);
KINOKO_ACTOR_FIELD(ActorRecord, x, 240);
KINOKO_ACTOR_FIELD(ActorRecord, direction, 272);
KINOKO_ACTOR_FIELD(ActorRecord, collision_records, 328);
KINOKO_ACTOR_FIELD(ActorRecord, collision_slots, 332);
KINOKO_ACTOR_FIELD(ActorRecord, inline_slots, 340);
KINOKO_ACTOR_FIELD(ActorRecord, bounds_anchor_x, 352);
KINOKO_ACTOR_FIELD(ActorRecord, initial, 376);
KINOKO_ACTOR_FIELD(InitialData, width, 12);
KINOKO_ACTOR_FIELD(InitialData, chip_flags, 16);
KINOKO_ACTOR_FIELD(InitialData, chip_bound_type, 34);
KINOKO_ACTOR_FIELD(ActorRecord, local_bounds, 424);
KINOKO_ACTOR_FIELD(ActorRecord, world_bounds, 440);
KINOKO_ACTOR_FIELD(ActorRecord, chip_cache_storage, 512);
KINOKO_ACTOR_FIELD(AnimationRecord, loops, 24);
KINOKO_ACTOR_FIELD(AnimationRecord, left, 28);
KINOKO_ACTOR_FIELD(AnimationRecord, flags, 44);
KINOKO_ACTOR_FIELD(FrameRecord, duration, 240);
KINOKO_ACTOR_FIELD(FrameRecord, owned_payload, 244);
KINOKO_ACTOR_FIELD(ManagerPrefix, animation_lookup, 36);
KINOKO_ACTOR_FIELD(ManagerPrefix, animations, 52);
KINOKO_ACTOR_FIELD(ManagerPrefix, textures, 68);
KINOKO_ACTOR_FIELD(ManagerPrefix, actors, 84);
KINOKO_ACTOR_FIELD(ManagerPrefix, iteration, 100);
KINOKO_ACTOR_FIELD(ManagerPrefix, iteration_state, 116);
KINOKO_ACTOR_FIELD(ManagerPrefix, cleanup_pending, 120);
#undef KINOKO_ACTOR_FIELD
}
