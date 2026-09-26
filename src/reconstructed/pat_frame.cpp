#include "kinoko/angle_math.h"
#include "kinoko/pat_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/integer_vector.h"
#include "kinoko/texture_store.h"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;
using kinoko::legacy::address;
using kinoko::legacy::pointer;

// 405320: Z, then Y, then X, with single-precision stores between axes.
// x87 4054D4..4054FF and 405620..40564B settle the Y/X signs obscured by
// reused decompiler temporaries: +z*sin on x/y, -x/y*sin on z.
void rotate(std::array<Position3,4> &positions,const FrameAppearance &a,float pivot_x,float pivot_y) {
    if (a.roll_z!=0.0f) {
        const float c=kinoko_cos_degrees(a.roll_z),s=kinoko_sin_degrees(a.roll_z);
        for (auto &p:positions) {
            const float x=p.x-pivot_x,y=p.y-pivot_y;
            p.x=x*c+pivot_x-y*s;p.y=x*s+pivot_y+y*c;
        }
    }
    if (a.roll_y!=0.0f) {
        const float c=kinoko_cos_degrees(a.roll_y),s=kinoko_sin_degrees(a.roll_y);
        for (auto &p:positions) {
            const float x=p.x-pivot_x,z=p.z;
            p.x=z*s+pivot_x+x*c;p.z=z*c-x*s;
        }
    }
    if (a.roll_x!=0.0f) {
        const float c=kinoko_cos_degrees(a.roll_x),s=kinoko_sin_degrees(a.roll_x);
        for (auto &p:positions) {
            const float y=p.y-pivot_y,z=p.z;
            p.y=z*s+pivot_y+y*c;p.z=z*c-y*s;
        }
    }
}
}

extern "C" int32_t kinoko_pat_build_frame(KinokoActorManager *manager,KinokoAnimationFrame *receiver,
    const KinokoPatFrameFields *fields,uint32_t resource_base) {
    if (!manager || !receiver || !fields) return 0;
    const ManagerView owner(manager);
    auto* resources=reinterpret_cast<KinokoIntegerVector*>(owner.bytes(&ManagerPrefix::textures));
    const auto count=kinoko_integer_vector_size(resources);
    const auto *handles=kinoko_integer_vector_data(resources);
    int32_t handle=0;
    // Check the combined index, not each operand independently (one-past-end
    // used to pass when base+index equalled count).
    if (resource_base<count && fields->resource_index<count-resource_base)
        handle=handles[resource_base+fields->resource_index];
    const RecordView<FrameRecord> frame(receiver);
    frame.clear();
    frame.set(&FrameRecord::vtable,static_cast<Address>(address(kinoko_pat_frame_methods())));
    frame.set(&FrameRecord::sprite_x,fields->sprite_x);frame.set(&FrameRecord::sprite_y,fields->sprite_y);
    frame.set(&FrameRecord::pivot_x,fields->offset_x);frame.set(&FrameRecord::pivot_y,fields->offset_y);
    frame.set(&FrameRecord::duration,fields->duration);
    auto vertices=frame.get(&FrameRecord::vertices);
    for (auto &vertex:vertices) vertex.color=UINT32_MAX;
    frame.set(&FrameRecord::vertices,vertices);
    static std::atomic<int32_t> traces{};
    if (++traces<=48) {
        kinoko_trace_i32("actor:pat-frame-resource",fields->resource_index);
        kinoko_trace_i32("actor:pat-frame-sprite-x",fields->sprite_x);
        kinoko_trace_i32("actor:pat-frame-sprite-y",fields->sprite_y);
        kinoko_trace_i32("actor:pat-frame-width",fields->source_width);
        kinoko_trace_i32("actor:pat-frame-height",fields->source_height);
        kinoko_trace_i32("actor:pat-frame-offset-x",fields->offset_x);
        kinoko_trace_i32("actor:pat-frame-offset-y",fields->offset_y);
        kinoko_trace_i32("actor:pat-frame-handle",handle);
    }
    uint32_t width=0,height=0;
    if (handle>0 && handle<KINOKO_TEXTURE_CAPACITY) {
        width=kinoko_texture_slots[handle].width;height=kinoko_texture_slots[handle].height;
    }
    if (fields->type==2) {
        auto *appearance=static_cast<FrameAppearance *>(std::calloc(1,sizeof(FrameAppearance)));
        if (!appearance) return 0;
        const RecordView<FrameAppearance> view(appearance);
        const int32_t mode=fields->auxiliary_mode;
        view.set(&FrameAppearance::blend,mode>=0 && mode<=2?mode+1:mode);
        uint32_t color;std::memcpy(&color,fields->auxiliary_bytes,sizeof(color));
        view.set(&FrameAppearance::color,color);
        view.set(&FrameAppearance::scale_x,fields->auxiliary_values[0]/100.0f);
        view.set(&FrameAppearance::scale_y,fields->auxiliary_values[1]/100.0f);
        view.set(&FrameAppearance::roll_x,static_cast<float>(fields->auxiliary_values[2]));
        view.set(&FrameAppearance::roll_y,static_cast<float>(fields->auxiliary_values[3]));
        view.set(&FrameAppearance::roll_z,static_cast<float>(fields->auxiliary_values[4]));
        frame.set(&FrameRecord::owned_payload,appearance);
    }
    if (handle && width && height) {
        const float w=static_cast<float>(fields->source_width),h=static_cast<float>(fields->source_height);
        frame.set(&FrameRecord::texture,handle);
        frame.set(&FrameRecord::texture_width,static_cast<float>(width));
        frame.set(&FrameRecord::texture_height,static_cast<float>(height));
        const float u_extent=w/width,v_extent=h/height;
        frame.set(&FrameRecord::source_u_extent,u_extent);frame.set(&FrameRecord::source_v_extent,v_extent);
        vertices[0].u=static_cast<float>(fields->sprite_x)/width;
        vertices[0].v=static_cast<float>(fields->sprite_y)/height;
        vertices[1].u=vertices[0].u+u_extent;vertices[1].v=vertices[0].v;
        vertices[2].u=vertices[0].u;vertices[2].v=vertices[0].v+v_extent;
        vertices[3].u=vertices[1].u;vertices[3].v=vertices[2].v;
        std::array<Position3,4> positions{Position3{0,0,0},Position3{w,0,0},Position3{0,h,0},Position3{w,h,0}};
        if (fields->type==0) {
            for (auto &p:positions) { p.x*=2.0f;p.y*=2.0f; }
        } else if (auto *appearance=frame.get(&FrameRecord::owned_payload)) {
            const auto a=RecordView<FrameAppearance>(appearance).load();
            const float pivot_x=static_cast<float>(fields->offset_x),pivot_y=static_cast<float>(fields->offset_y);
            for (auto &p:positions) p.x=(p.x-pivot_x)*a.scale_x+pivot_x;
            for (auto &p:positions) p.y=(p.y-pivot_y)*a.scale_y+pivot_y;
            rotate(positions,a,pivot_x,pivot_y);
            for (auto &vertex:vertices) vertex.color=a.color;
        }
        frame.set(&FrameRecord::vertices,vertices);
        frame.set(&FrameRecord::positions,positions);
        frame.set(&FrameRecord::base_positions,positions);
    }
    return 1;
}
