#include "kinoko/actor_collision.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/map_layout_records.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
static_assert(sizeof(KinokoCollisionRecord) == 12, "Original collision records require Win32 pointers");
using namespace kinoko::actor;
using Rect = Bounds;
struct Contact {
    float top, bottom, slope = 0;
    float right, left;
    float solid_top = 65535, solid_bottom = -65535;
    int support = std::numeric_limits<int>::max();
    bool ceiling_adjusted = false, floor_adjusted = false;
    bool left_soft = false, right_soft = false;
};
struct Chip {
    float left, top, right, bottom, width, height;
    uint32_t flags;
    int16_t shape;
    explicit Chip(const KinokoCollisionRecord &record) {
        const kinoko::map::PlacementView placement(const_cast<void *>(record.layout));
        const kinoko::map::ChipView chip(const_cast<unsigned char *>(record.chip));
        left = placement.get(&kinoko::map::Placement::fractional_left);
        top = placement.get(&kinoko::map::Placement::fractional_top);
        width = static_cast<float>(chip.get(&kinoko::map::ChipDefinition::width));
        height = static_cast<float>(chip.get(&kinoko::map::ChipDefinition::height));
        right = left + width;
        bottom = top + height;
        flags = chip.get(&kinoko::map::ChipDefinition::flags);
        shape = chip.get(&kinoko::map::ChipDefinition::shape);
    }
    bool overlap(const Rect &r, bool strict_x, bool strict_y) const {
        return (strict_x ? left < r.right && right > r.left : left <= r.right && right >= r.left) &&
               (strict_y ? top < r.bottom && bottom > r.top : top <= r.bottom && bottom >= r.top);
    }
    float slope() const { return height / width; }
};
bool ignores_platform(const ActorView &actor, const Chip &c) {
    return (c.flags & actor.get(&ActorRecord::flags) & 1) != 0;
}
bool ignores_slope(const ActorView &actor) {
    return ((static_cast<uint32_t>(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags)) | actor.get(&ActorRecord::flags)) & 0x800000) != 0;
}

// 466E20: constrain the vertical interval before resolving the horizontal move.
bool constrain_interval(Contact &s, const ActorView &actor, const Rect &r, const Chip &c) {
    if ((c.flags & 0xf0) == 0xf0 || !c.overlap(r, false, false))
        return false;
    if (c.shape == -1 && ignores_platform(actor, c))
        return false;
    if (c.shape == -1 || c.shape == 0) {
        if (r.bottom < c.top + 2 && s.bottom > c.top) {
            s.bottom = c.top;
            return true;
        }
        if (r.top > c.bottom - 2 && s.top < c.bottom) {
            s.top = c.bottom;
            return true;
        }
        return false;
    }
    float ratio = c.slope();
    if (c.shape == 1 || c.shape == 2) {
        if (r.bottom > c.bottom)
            return false;
        bool down_left = c.shape == 1;
        float center_distance = down_left ? c.right - actor.get(&ActorRecord::x)
                                         : actor.get(&ActorRecord::x) - c.left;
        if ((c.flags & 0x20) && s.bottom > std::max(0.0f, center_distance * ratio) + c.top + 4)
            return false;
        float old_center = actor.get(&ActorRecord::world_bounds).left + actor.get(&ActorRecord::world_bounds).right;
        float new_center = r.left + r.right;
        float edge = 2 * (down_left ? c.right : c.left);
        if (!ignores_slope(actor) && ratio < 2 &&
            (down_left ? old_center > edge && new_center <= edge
                       : old_center < edge && new_center >= edge))
            s.slope = std::max(s.slope, ratio);
        float distance = down_left ? c.right - r.right : r.left - c.left;
        float floor = std::max(0.0f, distance * ratio) + c.top;
        if (s.bottom <= floor)
            return false;
        if (!ignores_slope(actor) && ratio < 2)
            s.slope = std::max(s.slope, ratio);
        s.floor_adjusted = true;
        s.bottom = floor;
        return true;
    }
    if (c.shape == 3 || c.shape == 4) {
        if (r.top < c.top)
            return false;
        bool down_left = c.shape == 3;
        float center_distance = down_left ? actor.get(&ActorRecord::x) - c.left
                                         : c.right - actor.get(&ActorRecord::x);
        if ((c.flags & 0x10) && s.top < c.bottom - std::max(0.0f, center_distance * ratio) - 4)
            return false;
        float distance = down_left ? r.left - c.left : c.right - r.right;
        float ceiling = c.bottom - std::max(0.0f, distance * ratio);
        if (s.top >= ceiling)
            return false;
        s.ceiling_adjusted = true;
        s.top = ceiling;
        if (!s.floor_adjusted)
            s.bottom = ceiling + actor.get(&ActorRecord::world_bounds).bottom - actor.get(&ActorRecord::world_bounds).top;
        return true;
    }
    return false;
}

// 4674B0: side contacts; touching horizontal edges count, vertical edges do not.
int sides(Contact &s, const ActorView &actor, const Rect &r, Rect &limits, const Chip &c) {
    if (ignores_platform(actor, c) || (c.flags & 0xf0) == 0xf0 ||
        !c.overlap(r, false, true) || c.shape > 0 || (c.flags & 0x30))
        return 0;
    actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) | (c.flags));
    if ((r.left < c.left || r.right > c.right) &&
        (r.left > c.left || r.right < c.right)) {
        bool soft = (c.flags & 0x4000000) != 0;
        if (r.left < c.left) {
            if (limits.right > c.left) { limits.right = c.left; s.right_soft = soft; }
            else if (limits.right == c.left) s.right_soft = s.right_soft && soft;
            return 2;
        }
        if (r.right > c.right) {
            if (limits.left < c.right) { limits.left = c.right; s.left_soft = soft; }
            else if (limits.left == c.right) s.left_soft = s.left_soft && soft;
            return 1;
        }
        return 0;
    }
    if (r.top >= c.top)
        limits.top = std::max(limits.top, c.bottom);
    if (r.bottom <= c.bottom)
        limits.bottom = std::min(limits.bottom, c.top);
    return 4;
}

// 4677C0: ceiling contacts and the small-corner correction bounds.
int ceiling(Contact &s, const ActorView &actor, Rect &r, float &edge_top,
            const Chip &c) {
    if (!c.overlap(r, true, false))
        return 0;
    actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) | (((c.flags & 0x3000000) && s.solid_bottom >= c.bottom)
            ? c.flags & 0xfcffffff : c.flags));
    if ((c.flags & 0x20) || (c.flags & 0xf0) == 0xf0 || ignores_platform(actor, c))
        return 0;
    float center = (r.left + r.right) * 0.5f;
    if (c.shape == 1 || c.shape == 2)
        return 0;
    if (c.shape == 3 || c.shape == 4) {
        if (center < c.left || center > c.right)
            return 0;
        float ratio = c.slope();
        float surface = c.shape == 3 ? c.bottom - (center - c.left) * ratio
                                     : c.top + (center - c.left) * ratio;
        if (r.top > surface)
            return 4;
        if ((c.flags & 0x10) && surface - 4 >
            std::max(s.top, actor.get(&ActorRecord::world_bounds).top + actor.get(&ActorRecord::parent_velocity_y)))
            return 0;
        r.top = std::max(r.top, surface);
        if (std::fabs(actor.get(&ActorRecord::pitch_top)) < ratio)
            actor.set(&ActorRecord::pitch_top, c.shape == 3 ? -ratio : ratio);
        if (s.solid_bottom <= r.top && !(c.flags & 0x3000000)) {
            s.solid_bottom = r.top;
            actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) & (0xfcffffff));
        }
        return 8;
    }
    if (r.bottom <= c.bottom)
        return 0;
    s.right = std::min(s.right, c.left);
    s.left = std::max(s.left, c.right);
    actor.set(&ActorRecord::pitch, 0.0f);
    if ((c.flags & 0x10) && c.bottom - 4 >
        std::max(s.top, actor.get(&ActorRecord::world_bounds).top + actor.get(&ActorRecord::parent_velocity_y)))
        return 0;
    if (s.solid_bottom <= c.bottom && !(c.flags & 0x3000000)) {
        s.solid_bottom = c.bottom;
        actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) & (0xfcffffff));
    }
    if (center < c.left || center > c.right) {
        if (!(c.flags & 0x4000000))
            edge_top = std::max(edge_top, c.bottom);
        return 1;
    }
    if (!(c.flags & 0x4000000))
        r.top = std::max(r.top, c.bottom);
    return 2;
}

// 467CD0: floor contacts, one-way surfaces, slope and support-record selection.
int floor_contact(Contact &s, const ActorView &actor, Rect &r, float &edge_bottom,
                  const Chip &c, int index) {
    if (!c.overlap(r, true, false))
        return 0;
    actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) | (((c.flags & 0x3000000) && s.solid_top <= c.top)
            ? c.flags & 0xfcffffff : c.flags));
    if ((c.flags & 0x10) || (c.flags & 0xf0) == 0xf0)
        return 0;
    float center = (r.left + r.right) * 0.5f;
    if (c.shape == 3 || c.shape == 4)
        return 0;
    if (c.shape == 1 || c.shape == 2) {
        if (center < c.left || center > c.right)
            return 0;
        float ratio = c.slope();
        float surface = c.shape == 1 ? c.bottom - (center - c.left) * ratio
                                     : c.top + (center - c.left) * ratio;
        if (r.bottom < surface)
            return 16;
        if ((c.flags & 0x20) && surface + 4 <
            std::min(s.bottom, actor.get(&ActorRecord::world_bounds).bottom + actor.get(&ActorRecord::parent_velocity_y)))
            return 0;
        r.bottom = std::min(r.bottom, surface);
        if (std::fabs(actor.get(&ActorRecord::pitch)) < ratio)
            actor.set(&ActorRecord::pitch, c.shape == 1 ? -ratio : ratio);
        if (s.solid_top >= r.bottom && !(c.flags & 0x3000000)) {
            s.solid_top = r.bottom;
            actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) & (0xfcffffff));
        }
        return 8;
    }
    if (r.top >= c.top)
        return 0;
    if ((ignores_platform(actor, c) || (c.flags & 0x20)) && c.top + 4 <
        std::min(s.bottom, actor.get(&ActorRecord::world_bounds).bottom + actor.get(&ActorRecord::parent_velocity_y)))
        return 0;
    s.support = std::min(s.support, index);
    if (s.solid_top >= c.top && !(c.flags & 0x3000000)) {
        s.solid_top = c.top;
        actor.set(&ActorRecord::collision_flags, actor.get(&ActorRecord::collision_flags) & (0xfcffffff));
    }
    if (center < c.left) { edge_bottom = std::min(edge_bottom, c.top); return 2; }
    if (center > c.right) { edge_bottom = std::min(edge_bottom, c.top); return 4; }
    r.bottom = std::min(r.bottom, c.top);
    return 1;
}
}

extern "C" int32_t kinoko_actor_collision_move(KinokoActor *receiver,
    const KinokoCollisionRecord *records, int32_t count, float dx, float dy) {
    const ActorView actor(receiver);
    Rect old = actor.get(&ActorRecord::world_bounds);
    int32_t *hit = reinterpret_cast<int32_t *>(actor.bytes(&ActorRecord::hits));
    std::fill(hit, hit + 4, 0);
    actor.set(&ActorRecord::collision_flags, 0u);
    Contact s;
    s.top = std::max(old.top, old.top + dy);
    s.bottom = std::min(old.bottom, old.bottom + dy);
    Rect moved = {old.left + dx, s.top, old.right + dx, s.bottom};
    for (int i = 0; i < count; ++i) {
        moved.top = s.top;
        moved.bottom = s.bottom;
        if (constrain_interval(s, actor, moved, Chip(records[i])))
            i = -1;
    }
    dy += s.slope * std::fabs(dx);
    moved.top = s.top;
    moved.bottom = s.bottom;
    Rect limits = moved;
    int flags = 0;
    for (int i = 0; i < count; ++i)
        flags |= sides(s, actor, moved, limits, Chip(records[i]));
    auto &crushed = *actor.bytes(&ActorRecord::crushed);
    if ((flags & 4) || limits.left >= limits.right) {
        s.top = std::max(s.top, limits.top);
        hit[0] = hit[2] = 1;
        crushed = 1;
    } else if (moved.right > limits.right) {
        if (moved.left < limits.left) {
            hit[0] = hit[2] = 1;
            moved.left = limits.left;
            moved.right = limits.right;
        } else if (!crushed) {
            moved.left += limits.right - moved.right;
            moved.right = limits.right;
            if (!s.right_soft) hit[2] = 1;
        }
    } else if (moved.left < limits.left) {
        if (!crushed) {
            moved.right += limits.left - moved.left;
            moved.left = limits.left;
            if (!s.left_soft) hit[0] = 1;
        }
    } else {
        crushed = 0;
    }
    actor.set(&ActorRecord::free_width, moved.right - moved.left);
    s.right = moved.right;
    s.left = moved.left;
    actor.set(&ActorRecord::pitch_top, 0.0f);
    moved.top = crushed ? s.top : old.top + dy;
    moved.bottom = std::max(s.bottom, s.top);
    float edge_top = moved.top;
    if (moved.bottom >= moved.top)
        for (int i = 0; i < count; ++i)
            hit[1] |= ceiling(s, actor, moved, edge_top, Chip(records[i]));
    if (hit[1] & 4) hit[1] = 0;
    else if (hit[1] & 8) hit[1] = 2;
    else if ((hit[1] & 1) && edge_top >= moved.top) moved.top = edge_top;
    if (hit[1] == 1 && dy < 0 && !(actor.get(&ActorRecord::collision_flags) & 0x8000000)) {
        Rect trial = moved;
        bool try_right = moved.right > s.right && moved.right < s.right + 10;
        bool try_left = moved.left < s.left && moved.left > s.left - 10;
        if (try_right || try_left) {
            trial.left = try_right ? s.right - (old.right - old.left) : s.left;
            trial.right = trial.left + old.right - old.left;
            trial.top = old.top + dy;
            Rect trial_limits = trial;
            for (int i = 0; i < count; ++i)
                sides(s, actor, trial, trial_limits, Chip(records[i]));
            if (try_right ? trial_limits.left <= trial.left : trial_limits.right >= trial.right) {
                moved.left = trial.left;
                moved.right = trial.right;
                moved.top = trial.top;
                hit[1] = 0;
            }
        }
    }
    // 469151..469162 keeps both operations in x87 before storing a float.
    // Rounding the intermediate sum to float can put a descending rider just
    // above its support, losing the floor contact and then Actor::step.
    moved.bottom = static_cast<float>(static_cast<double>(old.bottom) + moved.top - old.top);
    actor.set(&ActorRecord::pitch, 0.0f);
    float edge_bottom = moved.bottom;
    if (moved.bottom >= moved.top)
        for (int i = 0; i < count; ++i)
            hit[3] |= floor_contact(s, actor, moved, edge_bottom, Chip(records[i]), i);
    if (hit[3] & 16) hit[3] = 0;
    else if (hit[3] & 9) hit[3] = 1;
    else if ((hit[3] & 6) && edge_bottom <= moved.bottom) moved.bottom = edge_bottom;
    actor.set(&ActorRecord::x, actor.get(&ActorRecord::x) + ((moved.right - old.right + moved.left - old.left) * 0.5f));
    if (crushed) actor.set(&ActorRecord::x, actor.get(&ActorRecord::x) - (1));
    actor.set(&ActorRecord::y, actor.get(&ActorRecord::y) + (moved.bottom - old.bottom));
    actor.set(&ActorRecord::free_height, moved.bottom - moved.top);
    return ignores_slope(actor) ? -1 : s.support;
}
