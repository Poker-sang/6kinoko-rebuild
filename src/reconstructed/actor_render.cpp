#include "kinoko/angle_math.h"
#include "kinoko/actor_render.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cmath>
namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;
using kinoko::legacy::pointer;
}

// 45F0F0 / 45F350, with camera projection from 466320. The frame is borrowed
// from its animation owner; its working quad is reset before every actor draw.
extern "C" int32_t kinoko_actor_render(KinokoActor *receiver,KinokoCamera *camera) {
    const auto trace=kinoko_actor_render_trace_begin(receiver,camera);
    if (!receiver) return 0;
    const ActorView actor(receiver);
    auto *frame_pointer=actor.get(&ActorRecord::current_frame);
    if (!actor.get(&ActorRecord::active) || !actor.get(&ActorRecord::visible) || !frame_pointer) return 0;
    const RecordView<FrameRecord> frame(frame_pointer);
    if (!frame.get(&FrameRecord::texture)) return 0; // existing missing-texture guard
    if (camera) {
        const auto view=RecordView<CameraBoundsRecord>(camera).load();
        const auto bounds=actor.get(&ActorRecord::world_bounds);
        const float extent=actor.get(&ActorRecord::scale)*256.0f;
        if (bounds.left>view.bounds.right+extent || view.bounds.left-extent>bounds.right ||
            view.bounds.bottom+extent<bounds.top || bounds.bottom<view.bounds.top-extent) return 0;
    }
    auto positions=frame.get(&FrameRecord::base_positions);
    const float facing=-actor.get(&ActorRecord::direction);
    const float pivot_x=facing*frame.get(&FrameRecord::pivot_x);
    const float pivot_y=static_cast<float>(frame.get(&FrameRecord::pivot_y));
    for (auto &p:positions) p.x*=facing;
    const float scale_x=actor.get(&ActorRecord::scale_x)*actor.get(&ActorRecord::scale);
    const float scale_y=actor.get(&ActorRecord::scale_y)*actor.get(&ActorRecord::scale);
    for (auto &p:positions) {
        p.x=(p.x-pivot_x)*scale_x+pivot_x;
        p.y=(p.y-pivot_y)*scale_y+pivot_y;
    }
    const float angle=actor.get(&ActorRecord::rotation)*facing;
    if (angle!=0.0f) {
        const float cosine=kinoko_cos_degrees(angle),sine=kinoko_sin_degrees(angle);
        for (auto &p:positions) {
            const float x=p.x-pivot_x,y=p.y-pivot_y;
            p.x=x*cosine+pivot_x-y*sine;
            p.y=x*sine+pivot_y+y*cosine;
        }
    }
    const uint32_t color=static_cast<uint32_t>(actor.get(&ActorRecord::blue)) |
        ((static_cast<uint32_t>(actor.get(&ActorRecord::green)) |
        ((static_cast<uint32_t>(actor.get(&ActorRecord::red)) |
        (static_cast<uint32_t>(actor.get(&ActorRecord::alpha))<<8))<<8))<<8);
    kinoko_actor_frame_color(frame_pointer,color);
    // Original 405080 stores the actor translation before the second camera
    // translation. Combining them changes rounding near a pixel boundary.
    const float translate_x=actor.get(&ActorRecord::offset_x)*facing+actor.get(&ActorRecord::x)-pivot_x;
    const float translate_y=actor.get(&ActorRecord::y)-pivot_y+actor.get(&ActorRecord::offset_y);
    for (auto &p:positions) {
        p.x+=translate_x;p.y+=translate_y;
    }
    frame.set(&FrameRecord::positions,positions);
    kinoko_camera_project(camera,reinterpret_cast<KinokoQuad *>(frame_pointer));
    kinoko_actor_render_trace_draw(receiver);
    const auto blend=actor.get(&ActorRecord::blend);
    kinoko_actor_render_set_blend(blend>=2 && blend<=4?blend:1);
    const auto result=kinoko_actor_render_submit(frame_pointer);
    kinoko_actor_render_set_blend(1);
    kinoko_actor_render_trace_end(trace,result);
    return 1;
}
