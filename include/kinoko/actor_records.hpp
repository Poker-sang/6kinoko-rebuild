#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.hpp"
#include "kinoko/sprite.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/camera_records.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct KinokoActor;
struct KinokoActorPool;
struct KinokoActorManager;
struct KinokoAnimation;
struct KinokoAnimationFrame;
struct SQVM;

namespace kinoko::actor {
using Address = std::uint32_t; // serialized native Win32 address, never an owner
using ScriptStorage = std::array<unsigned char, 12>; // external refs: ObjectView
using Bounds = kinoko::camera::Bounds;
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
    std::int32_t owner_references;
    uint32_t pool_handle;
    void *priority_entry; // borrowed token; priority index owns it
    std::uint8_t registration_flag20, visible;
    std::uint8_t release_pending, unknown23;
    Address owner, owner_control; // strong native control; not a VM reference
    Address step, step_control;   // weak native control
    std::uint8_t active;
    std::array<unsigned char, 3> unknown41;
    ScriptStorage script_object, update_callback, collision_callback;
    float spawn_x, spawn_y, spawn_z;
    SQVM *update_vm;
    ScriptStorage object96, object108;
    SQVM *collision_vm; // callback state prefix, borrowed
    ScriptStorage collision_environment, collision_function;
    KinokoActorManager *manager;              // borrowed; manager owns the live Actor set
    KinokoAnimationFrame *sprite_frame; // borrowed from manager-owned animation frames
    float offset_x, offset_y, rotation;
    float scale, scale_x, scale_y;
    int32_t alpha, red, green, blue, blend;
    KinokoAnimation *animation; // borrowed, never freed by Actor
    KinokoAnimationFrame *current_frame;
    std::int32_t take, frame_index, frame_time, take_duration;
    int32_t id;
    std::int32_t priority;
    std::uint32_t update_group, flags;
    float x, y;
    float previous_x, previous_y;
    float velocity_x, velocity_y, parent_velocity_x, parent_velocity_y;
    float direction;
    float pitch, pitch_top;
    std::array<int32_t, 4> hits; // left, top, right, bottom
    uint8_t crushed;
    std::array<uint8_t, 3> padding301;
    float free_width, free_height;
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
    KinokoAnimation *next, *previous; // borrowed links; owning animation list is separate
    KinokoAnimationFrame *frames_begin, *frames_end, *frames_capacity;
    uint32_t unknown20;
    std::uint8_t loops, has_bounds;
    std::array<unsigned char, 2> unknown26;
    std::int32_t left, top, right, bottom, duration_total;
};
struct FrameAppearance {
    int32_t blend;
    uint32_t color;
    float scale_x, scale_y, roll_x, roll_y, roll_z;
};
using Position3 = kinoko::render::Position3;
struct FrameRecord {
    Address vtable;
    std::int32_t texture; // borrowed handle; manager releases texture owners
    std::array<KinokoSpriteVertex, 4> vertices;
    float texture_width, texture_height;
    std::array<Position3, 4> base_positions, positions;
    float source_u_extent, source_v_extent;
    int16_t sprite_x, sprite_y, pivot_x, pivot_y;
    std::int16_t duration;
    std::uint16_t unknown242;
    FrameAppearance *owned_payload; // malloc-owned, released before frame storage
};
struct TreeIndex {
    Address policy, head;
    std::int32_t count;
};
struct ListIndex { Address head; std::uint32_t count; };
struct VectorIndex { Address begin, end, capacity; };
// Manager iteration storage is native_buffer-owned; entries borrow live Actors.
struct ActorIterationBuffer { KinokoActor **begin, **end; void *storage_owner; };
struct RenderLayerRecord {
    const void *methods;
    KinokoActorManager *manager;
    int32_t index, begin, end;
};
using CameraBoundsRecord = kinoko::camera::Record;
// Only the verified prefix of the manager is described, not a new allocation
// size. Index nodes and animation lists have distinct ownership semantics.
struct ManagerPrefix {
    const void *methods;
    KinokoActorPool *pool;
    void *owner_list;
    std::array<unsigned char, 8> unknown12;
    std::array<RenderLayerRecord *, 4> render_layers;
    TreeIndex animation_lookup; // owns nodes; values borrow animation list items
    std::array<unsigned char, 4> unknown48;
    ListIndex animations;       // owns animation items, frames and payloads
    uint32_t unknown60;
    int32_t update_mask;
    VectorIndex textures;      // owns texture handles, retains vector capacity
    std::array<unsigned char, 4> unknown80;
    TreeIndex actors;          // live actor/priority tree
    std::array<unsigned char, 4> unknown96;
    ActorIterationBuffer iteration;
    std::array<unsigned char, 4> unknown112;
    std::int32_t iteration_count;
    std::uint8_t cleanup_pending;
    std::array<unsigned char, 3> unknown121;
    ActorIterationBuffer callback_candidates;
};
using ActorView = native::RecordView<ActorRecord>;
using ManagerView = native::RecordView<ManagerPrefix>;
inline constexpr std::array<ScriptStorage ActorRecord::*, 7> script_members{
    &ActorRecord::script_object, &ActorRecord::update_callback,
    &ActorRecord::collision_callback, &ActorRecord::object96,
    &ActorRecord::object108, &ActorRecord::collision_environment, &ActorRecord::collision_function};

static_assert(sizeof(InitialData) == 48 && sizeof(ActorRecord) == 0x220);
static_assert(sizeof(ControlRecord) == 16 && sizeof(ControlTable) == 12);
static_assert(sizeof(AnimationRecord) == 48 && sizeof(FrameRecord) == 248);
static_assert(sizeof(FrameAppearance) == 28 && offsetof(FrameAppearance, color) == 4);
static_assert(offsetof(FrameRecord, vertices) == 8 && offsetof(FrameRecord, base_positions) == 128);
static_assert(offsetof(FrameRecord, positions) == 176 && offsetof(FrameRecord, pivot_x) == 236);
static_assert(offsetof(CameraBoundsRecord, x) == 40 && offsetof(CameraBoundsRecord, bounds) == 72);
static_assert(sizeof(ManagerPrefix) == 136);
static_assert(sizeof(RenderLayerRecord) == 20 && offsetof(RenderLayerRecord, begin) == 12);
static_assert(offsetof(ManagerPrefix, pool) == 4 && offsetof(ManagerPrefix, owner_list) == 8);
static_assert(offsetof(ManagerPrefix, render_layers) == 20 && offsetof(ManagerPrefix, update_mask) == 64);
static_assert(offsetof(ActorRecord, pool_handle) == 12 && offsetof(ActorRecord, priority_entry) == 16);
static_assert(offsetof(ActorRecord, spawn_x) == 80 && offsetof(ActorRecord, update_vm) == 92);
static_assert(offsetof(ActorRecord, rotation) == 164 && offsetof(ActorRecord, blend) == 196);
#define KINOKO_ACTOR_FIELD(T, M, O) static_assert(offsetof(T, M) == O)
KINOKO_ACTOR_FIELD(ActorRecord, owner_references, 8);
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
KINOKO_ACTOR_FIELD(ActorRecord, collision_environment, 124);
KINOKO_ACTOR_FIELD(ActorRecord, collision_function, 136);
KINOKO_ACTOR_FIELD(ActorRecord, manager, 148);
KINOKO_ACTOR_FIELD(ActorRecord, sprite_frame, 152);
KINOKO_ACTOR_FIELD(ActorRecord, scale, 168);
KINOKO_ACTOR_FIELD(ActorRecord, animation, 200);
KINOKO_ACTOR_FIELD(ActorRecord, frame_time, 216);
KINOKO_ACTOR_FIELD(ActorRecord, priority, 228);
KINOKO_ACTOR_FIELD(ActorRecord, x, 240);
KINOKO_ACTOR_FIELD(ActorRecord, previous_x, 248);
KINOKO_ACTOR_FIELD(ActorRecord, previous_y, 252);
KINOKO_ACTOR_FIELD(ActorRecord, velocity_x, 256);
KINOKO_ACTOR_FIELD(ActorRecord, parent_velocity_y, 268);
KINOKO_ACTOR_FIELD(ActorRecord, pitch, 276);
KINOKO_ACTOR_FIELD(ActorRecord, hits, 284);
KINOKO_ACTOR_FIELD(ActorRecord, crushed, 300);
KINOKO_ACTOR_FIELD(ActorRecord, free_width, 304);
KINOKO_ACTOR_FIELD(ActorRecord, free_height, 308);
KINOKO_ACTOR_FIELD(ActorRecord, collision_vm, 120);
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
KINOKO_ACTOR_FIELD(AnimationRecord, frames_begin, 8);
KINOKO_ACTOR_FIELD(AnimationRecord, frames_capacity, 16);
KINOKO_ACTOR_FIELD(AnimationRecord, duration_total, 44);
KINOKO_ACTOR_FIELD(AnimationRecord, left, 28);
KINOKO_ACTOR_FIELD(FrameRecord, duration, 240);
KINOKO_ACTOR_FIELD(FrameRecord, owned_payload, 244);
KINOKO_ACTOR_FIELD(ManagerPrefix, animation_lookup, 36);
KINOKO_ACTOR_FIELD(ManagerPrefix, animations, 52);
KINOKO_ACTOR_FIELD(ManagerPrefix, textures, 68);
KINOKO_ACTOR_FIELD(ManagerPrefix, actors, 84);
KINOKO_ACTOR_FIELD(ManagerPrefix, iteration, 100);
KINOKO_ACTOR_FIELD(ManagerPrefix, iteration_count, 116);
KINOKO_ACTOR_FIELD(ManagerPrefix, cleanup_pending, 120);
KINOKO_ACTOR_FIELD(ManagerPrefix, callback_candidates, 124);
#undef KINOKO_ACTOR_FIELD
}
