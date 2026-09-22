// Diagnostic observations only; no gameplay decisions or VM writes.
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_manager.h"
#include "kinoko/actor_render.h"
#include "kinoko/map_collision.h"
#include "kinoko/game_host.h"
#include "kinoko/map_manager.h"
#include "kinoko/application.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cstdio>
#include <cstring>
extern "C" {
extern char *g644;
void retdec_trace(const char *);
void retdec_trace_i32(const char *,int32_t);
void retdec_trace_squirrel_name(const char *,int32_t);
}
using namespace kinoko::actor;
namespace {
template<class T> const unsigned char *actor_bytes(int32_t actor,T ActorRecord::*member,size_t inner=0) {
    return ActorView(kinoko::legacy::pointer<void>(actor)).bytes(member)+inner;
}
template<class T,class M> T actor_bits(int32_t actor,M ActorRecord::*member,size_t inner=0) {
    return kinoko::legacy::load<T>(actor_bytes(actor,member,inner));
}
template<class T,class M> T camera_bits(KinokoCamera *camera,M kinoko::camera::Record::*member,size_t inner=0) {
    return kinoko::legacy::load<T>(kinoko::camera::View(camera).bytes(member)+inner);
}
}
extern "C" void retdec_trace_star_state(const char *phase, int32_t actor) {
    static struct { uint32_t handle; int hits, samples; } observed[32];
    uint32_t handle;
    int32_t sprite, proto=0;
    int release, hits, index;
    char message[768];
    if (!actor || actor_bits<int32_t>(actor, &ActorRecord::take)!=1060) return;
    release=strcmp(phase,"release")==0;
    if (actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type))==0x08000100)
        proto=kinoko::script::address(kinoko::legacy::pointer<SQClosure>(actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value)))->_function);
    if (!release && (!proto || kinoko::legacy::pointer<SQFunctionProto>(proto)->_name._type!=0x08000010 ||
        strcmp(_stringval(kinoko::legacy::pointer<SQFunctionProto>(proto)->_name),"UpdateWalk")!=0)) return;
    handle=actor_bits<uint32_t>(actor, &ActorRecord::pool_handle);
    index=(int)(handle%32);
    hits=actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t));
    if(observed[index].handle!=handle) {
        observed[index].handle=handle;
        observed[index].samples=0;
        observed[index].hits=-1;
    }
    if(!release && ((observed[index].hits==hits && kinoko_application_frame_count()%10!=0) || observed[index].samples>=256)) return;
    observed[index].hits=hits;
    ++observed[index].samples;
    sprite=actor_bits<int32_t>(actor, &ActorRecord::current_frame);
    sprintf_s(message,sizeof(message),
        "actor:star frame=%d phase=%s handle=%08X xy=(%.6g,%.6g) v=(%.6g,%.6g) "
        "hits=(%d,%d,%d,%d) bounds=(%.6g,%.6g,%.6g,%.6g) "
        "active=%d visible=%d release=%d priority=%d alpha=%d texture=%d spriteY=(%.6g,%.6g) "
        "mapHeight=%d camera=(%.6g,%.6g,%.6g,%.6g)",
        kinoko_application_frame_count(),phase,handle,actor_bits<float>(actor, &ActorRecord::x),actor_bits<float>(actor, &ActorRecord::y),
        actor_bits<float>(actor, &ActorRecord::velocity_x),actor_bits<float>(actor, &ActorRecord::velocity_y),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 0 * sizeof(int32_t)),actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 2 * sizeof(int32_t)),hits,
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)),actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)),actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)),
        actor_bits<uint8_t>(actor, &ActorRecord::active),actor_bits<uint8_t>(actor, &ActorRecord::visible),
        actor_bits<uint8_t>(actor, &ActorRecord::release_pending),actor_bits<int32_t>(actor, &ActorRecord::priority),
        actor_bits<int32_t>(actor, &ActorRecord::alpha),sprite ? kinoko::native::RecordView<FrameRecord>(kinoko::legacy::pointer<void>(sprite)).get(&FrameRecord::texture) : 0,
        sprite ? kinoko::native::RecordView<FrameRecord>(kinoko::legacy::pointer<void>(sprite)).get(&FrameRecord::positions)[0].y : 0,sprite ? kinoko::native::RecordView<FrameRecord>(kinoko::legacy::pointer<void>(sprite)).get(&FrameRecord::positions)[3].y : 0,
        kinoko_map_manager_height(kinoko_game_objects()->map),
        camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)),camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)),
        camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)),camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)));
    retdec_trace(message);
    if(release && g644) {
        auto *vm=reinterpret_cast<SQVM *>(g644);
        const auto frames=vm->_callsstacksize;
        for (SQInteger i=frames-1;i>=0 && i>=frames-5;--i) {
            const auto& frame=vm->_callsstack[i];
            if (sq_type(frame._closure)==OT_CLOSURE) {
                auto *caller=_closure(frame._closure)->_function;
                retdec_trace_squirrel_name("actor:star-release-source",kinoko::script::address(
                    sq_type(caller->_sourcename)==OT_STRING?_stringval(caller->_sourcename):"<unknown>"));
                retdec_trace_squirrel_name("actor:star-release-function",kinoko::script::address(
                    sq_type(caller->_name)==OT_STRING?_stringval(caller->_name):"<anonymous>"));
                retdec_trace_i32("actor:star-release-instruction",static_cast<int32_t>(frame._ip-caller->_instructions)-1);
            }
        }
    }
}

static void retdec_trace_invalid_actor(const char *phase, int32_t actor) {
    static volatile LONG count;
    uint32_t bits[12];
    int invalid=0;
    if(!actor) return;
    const unsigned char *fields[]={actor_bytes(actor, &ActorRecord::x), actor_bytes(actor, &ActorRecord::y), actor_bytes(actor, &ActorRecord::velocity_x), actor_bytes(actor, &ActorRecord::velocity_y), actor_bytes(actor, &ActorRecord::parent_velocity_x), actor_bytes(actor, &ActorRecord::parent_velocity_y), actor_bytes(actor, &ActorRecord::free_width), actor_bytes(actor, &ActorRecord::free_height), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom))};

    for(int i=0;i<12;++i) {
        memcpy(bits+i,fields[i],4);
        invalid |= (bits[i]&0x7f800000u)==0x7f800000u;
    }
    if(invalid && InterlockedIncrement(&count)<=32) {
        char message[512];
        sprintf_s(message,sizeof(message),
            "actor:invalid-state frame=%d phase=%s actor=%08X take=%d "
            "xy=%08X,%08X v=%08X,%08X parentDelta=%08X,%08X free=%08X,%08X "
            "bounds=%08X,%08X,%08X,%08X step=%08X,%08X",
            kinoko_application_frame_count(),phase,(uint32_t)actor,actor_bits<int32_t>(actor, &ActorRecord::take),
            bits[0],bits[1],bits[2],bits[3],bits[4],bits[5],bits[6],bits[7],
            bits[8],bits[9],bits[10],bits[11],
            actor_bits<uint32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type)),actor_bits<uint32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value)));
        retdec_trace(message);
    }
}

extern "C" int32_t kinoko_actor_trace_step_begin(KinokoActor *receiver, int32_t callback_type) {
    const int32_t actor = (int32_t)(intptr_t)receiver;
    static volatile LONG step_trace_count;
    LONG step_trace_index;
    step_trace_index = InterlockedIncrement(&step_trace_count);
    if (step_trace_index <= 64 &&
        actor_bits<int32_t>(actor, &ActorRecord::id) >= 0x200 &&
        actor_bits<int32_t>(actor, &ActorRecord::id) <= 0x207) {
        int32_t y_bits;

        memcpy(&y_bits, actor_bytes(actor, &ActorRecord::y),
               sizeof(y_bits));
        retdec_trace_i32("actor:step-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        retdec_trace_i32("actor:step-type", callback_type);
        retdec_trace_i32("actor:step-data",
                         actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value)));
        retdec_trace_i32("actor:step-state-vm",
                         actor_bits<int32_t>(actor, &ActorRecord::update_vm));
        retdec_trace_i32("actor:step-state-type",
                         actor_bits<int32_t>(actor, &ActorRecord::update_environment, offsetof(KinokoOwnedObjectWords, type)));
        retdec_trace_i32("actor:step-state-data",
                         actor_bits<int32_t>(actor, &ActorRecord::update_environment, offsetof(KinokoOwnedObjectWords, value)));
        retdec_trace_i32("actor:step-callback-data",
                         actor_bits<int32_t>(actor, &ActorRecord::step));
        retdec_trace_i32("actor:step-callback-delegate",
                         actor_bits<int32_t>(actor, &ActorRecord::step_control));
        retdec_trace_i32("actor:step-y-before", y_bits);
    }
    if (callback_type == 0x08000100) retdec_trace_invalid_actor("before-script",actor);
    return step_trace_index;
}

extern "C" void kinoko_actor_trace_step_end(KinokoActor *receiver, int32_t step_result, int32_t step_trace_index) {
    const int32_t actor = (int32_t)(intptr_t)receiver;
        retdec_trace_invalid_actor("after-script",actor);
        /* Original 45E180 failure retirement is handled by the C++ adapter. */
        if (step_result < 0) {
            static volatile LONG failure_count;
            if (InterlockedIncrement(&failure_count) <= 64) {
                char message[384];
                sprintf_s(message, sizeof(message),
                    "actor:update-failed frame=%d actor=%08X id=%X take=%d "
                    "xy=(%.3f,%.3f) v=(%.3f,%.3f) camera=(%.3f,%.3f,%.3f,%.3f)",
                    kinoko_application_frame_count(), (uint32_t)actor, actor_bits<uint32_t>(actor, &ActorRecord::id),
                    actor_bits<int32_t>(actor, &ActorRecord::take),
                    actor_bits<float>(actor, &ActorRecord::x), actor_bits<float>(actor, &ActorRecord::y),
                    actor_bits<float>(actor, &ActorRecord::velocity_x), actor_bits<float>(actor, &ActorRecord::velocity_y),
                    camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)), camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)),
                    camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)), camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)));
                retdec_trace(message);
            }
        }
        if (step_trace_index <= 64)
            retdec_trace_i32("actor:step-result", step_result);
        if (step_trace_index <= 64 &&
            actor_bits<int32_t>(actor, &ActorRecord::id) >= 0x200 &&
            actor_bits<int32_t>(actor, &ActorRecord::id) <= 0x207) {
            int32_t y_bits;

            memcpy(&y_bits, actor_bytes(actor, &ActorRecord::y),
                   sizeof(y_bits));
            retdec_trace_i32("actor:step-y-after", y_bits);
        }

}

extern "C" void kinoko_actor_trace_motion(KinokoActor *receiver, int32_t phase) {
    int32_t actor = (int32_t)(intptr_t)receiver;
    static volatile LONG trace_count;
    LONG trace_index;
    if (phase == 0) {
        retdec_trace_invalid_actor("before-motion", actor);
        retdec_trace_star_state("before-motion", actor);
    } else if (phase == 1) {
    trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 16) {
        int32_t before_x;
        int32_t after_x;
        memcpy(&before_x, actor_bytes(actor, &ActorRecord::previous_x),
               sizeof(before_x));
        memcpy(&after_x, actor_bytes(actor, &ActorRecord::x),
               sizeof(after_x));
        retdec_trace_i32("actor:motion-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        retdec_trace_i32("actor:motion-before-x", before_x);
        retdec_trace_i32("actor:motion-after-x", after_x);
    }
    } else {
        retdec_trace_invalid_actor("after-motion", actor);
        retdec_trace_star_state("after-motion", actor);
    }
}

static void retdec_trace_actor_window_state(int32_t phase, int32_t actor,
                                             int32_t update_mask)
{
    int32_t id;
    int32_t frame;
    int32_t node;
    int32_t value;

    if (actor == 0 || kinoko_application_frame_count() < 540 || kinoko_application_frame_count() > 820 || (kinoko_application_frame_count() % 10) != 0)
        return;
    id = actor_bits<int32_t>(actor, &ActorRecord::id);
    if (id < 0x200 || id > 0x207)
        return;

    frame = actor_bits<int32_t>(actor, &ActorRecord::current_frame);
    node = actor_bits<int32_t>(actor, &ActorRecord::animation);
    retdec_trace_i32("actor:diag-frame-counter", kinoko_application_frame_count());
    retdec_trace_i32("actor:diag-phase", phase);
    retdec_trace_i32("actor:diag-id", id);
    retdec_trace_i32("actor:diag-address", actor);
    retdec_trace_i32("actor:diag-animation-key",
                     actor_bits<int32_t>(actor, &ActorRecord::take));
    retdec_trace_i32("actor:diag-frame-pointer", frame);
    retdec_trace_i32("actor:diag-node-pointer", node);
    retdec_trace_i32("actor:diag-frame-index",
                     actor_bits<int32_t>(actor, &ActorRecord::frame_index));
    retdec_trace_i32("actor:diag-timer",
                     actor_bits<int32_t>(actor, &ActorRecord::frame_time));
    value = frame != 0
        ? (int32_t)kinoko::native::RecordView<FrameRecord>(kinoko::legacy::pointer<void>(frame)).get(&FrameRecord::duration) : 0;
    retdec_trace_i32("actor:diag-duration", value);
    retdec_trace_i32("actor:diag-active",
                     actor_bits<unsigned char>(actor, &ActorRecord::active));
    retdec_trace_i32("actor:diag-visible",
                     actor_bits<unsigned char>(actor, &ActorRecord::registration_flag20));
    retdec_trace_i32("actor:diag-removable",
                     actor_bits<unsigned char>(actor, &ActorRecord::release_pending));
    retdec_trace_i32("actor:diag-update-flags",
                     actor_bits<int32_t>(actor, &ActorRecord::update_group));
    retdec_trace_i32("actor:diag-update-mask", update_mask);
    retdec_trace_i32("actor:diag-mask-hit",
                     (actor_bits<int32_t>(actor, &ActorRecord::update_group) & update_mask) != 0);
    memcpy(&value, actor_bytes(actor, &ActorRecord::x), sizeof(value));
    retdec_trace_i32("actor:diag-x", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::y), sizeof(value));
    retdec_trace_i32("actor:diag-y", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), sizeof(value));
    retdec_trace_i32("actor:diag-left", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)), sizeof(value));
    retdec_trace_i32("actor:diag-top", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), sizeof(value));
    retdec_trace_i32("actor:diag-right", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)), sizeof(value));
    retdec_trace_i32("actor:diag-bottom", value);
}

static void retdec_trace_player_state(const char *phase, int32_t actor,
                                       int32_t camera) {
    static volatile LONG count;
    static int32_t last_actor, last_take;
    int32_t closure, proto, take, transition;
    uint32_t xy_bits[2];
    const char *source, *name;
    char message[896];
    if (actor == 0 || actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type)) != 0x08000100)
        return;
    closure = actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value));
    proto = kinoko::script::address(kinoko::legacy::pointer<SQClosure>(closure)->_function);
    if (proto == 0 || kinoko::legacy::pointer<SQFunctionProto>(proto)->_sourcename._type != 0x08000010 ||
        kinoko::legacy::pointer<SQFunctionProto>(proto)->_name._type != 0x08000010)
        return;
    source = _stringval(kinoko::legacy::pointer<SQFunctionProto>(proto)->_sourcename);
    name = _stringval(kinoko::legacy::pointer<SQFunctionProto>(proto)->_name);
    if (_stricmp(source, "data/script/player.nut") != 0 || strcmp(name, "Update") != 0)
        return;
    take = actor_bits<int32_t>(actor, &ActorRecord::take);
    transition = last_actor != actor || last_take != take;
    last_actor = actor;
    last_take = take;
    if (!transition && actor_bits<float>(actor, &ActorRecord::velocity_x) == 0 && actor_bits<float>(actor, &ActorRecord::velocity_y) == 0 &&
        actor_bits<float>(actor, &ActorRecord::free_width) >= 12 &&
        !(actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)) && actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t))) &&
        kinoko_application_frame_count() % 60 != 0)
        return;
    if (InterlockedIncrement(&count) > 6000 && !transition)
        return;
    memcpy(xy_bits, actor_bytes(actor, &ActorRecord::x), sizeof(xy_bits));
    /* Observe the inputs to the unchanged script death checks without touching the VM stack. */
    sprintf_s(message, sizeof(message),
        "actor:player-state frame=%d phase=%s actor=%08X take=%d xy=(%.3f,%.3f) "
        "v=(%.3f,%.3f) free=(%.3f,%.3f) hits=(%d,%d,%d,%d) flags=%08X "
        "bounds=(%.3f,%.3f,%.3f,%.3f) camera=(%.3f,%.3f,%.3f,%.3f) xyBits=%08X,%08X",
        kinoko_application_frame_count(), phase, (uint32_t)actor, actor_bits<int32_t>(actor, &ActorRecord::take),
        actor_bits<float>(actor, &ActorRecord::x), actor_bits<float>(actor, &ActorRecord::y),
        actor_bits<float>(actor, &ActorRecord::velocity_x), actor_bits<float>(actor, &ActorRecord::velocity_y),
        actor_bits<float>(actor, &ActorRecord::free_width), actor_bits<float>(actor, &ActorRecord::free_height),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 0 * sizeof(int32_t)), actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 2 * sizeof(int32_t)), actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t)),
        actor_bits<uint32_t>(actor, &ActorRecord::collision_flags),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)),
        camera ? camera_bits<float>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, left)) : 0,
        camera ? camera_bits<float>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, top)) : 0,
        camera ? camera_bits<float>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, right)) : 0,
        camera ? camera_bits<float>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)) : 0, xy_bits[0], xy_bits[1]);
    retdec_trace(message);
}

extern "C" int32_t kinoko_actor_render_trace_begin(KinokoActor *receiver, KinokoCamera *camera_pointer) {
    int32_t actor = (int32_t)(intptr_t)receiver;
    int32_t camera = (int32_t)(intptr_t)camera_pointer;
    int32_t frame;
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);
    if (!actor) return trace_index;
    frame = actor_bits<int32_t>(actor, &ActorRecord::current_frame);
    if (trace_index <= 16) {
        retdec_trace_i32("actor:render-actor", actor);
        retdec_trace_i32("actor:render-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        retdec_trace_i32("actor:render-frame", frame);
        retdec_trace_i32("actor:render-node",
                         actor_bits<int32_t>(actor, &ActorRecord::animation));
        retdec_trace_i32("actor:render-handle",
                         frame != 0 ? kinoko::native::RecordView<FrameRecord>(kinoko::legacy::pointer<void>(frame)).get(&FrameRecord::texture) : 0);
        retdec_trace_i32("actor:render-active",
                         actor_bits<int32_t>(actor, &ActorRecord::active));
        retdec_trace_i32("actor:render-visible",
                         actor_bits<int32_t>(actor, &ActorRecord::visible));
        retdec_trace_i32("actor:render-left",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)));
        retdec_trace_i32("actor:render-top",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)));
        retdec_trace_i32("actor:render-right",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)));
        retdec_trace_i32("actor:render-bottom",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)));
        retdec_trace_i32("actor:render-camera-left",
                         camera != 0 ? camera_bits<int32_t>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, left)) : 0);
        retdec_trace_i32("actor:render-camera-top",
                         camera != 0 ? camera_bits<int32_t>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, top)) : 0);
        retdec_trace_i32("actor:render-camera-right",
                         camera != 0 ? camera_bits<int32_t>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, right)) : 0);
        retdec_trace_i32("actor:render-camera-bottom",
                         camera != 0 ? camera_bits<int32_t>(kinoko::legacy::pointer<KinokoCamera>(camera), &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)) : 0);
    }
    retdec_trace_star_state("render-entry",actor);
    return trace_index;
}

extern "C" void kinoko_actor_render_trace_draw(KinokoActor *actor) { retdec_trace_star_state("draw",(int32_t)(intptr_t)actor); }

extern "C" void kinoko_actor_render_trace_end(int32_t index, int32_t result) {
    if (index <= 16) retdec_trace_i32("actor:render-submit",result);
}

extern "C" void kinoko_actor_trace_collision(KinokoActor *actor, int32_t after) {
    retdec_trace_invalid_actor(after ? "after-collision" : "before-collision", (int32_t)(intptr_t)actor);
}

extern "C" void kinoko_actor_manager_trace_actor(int32_t phase, KinokoActor *actor, KinokoCamera *camera, int32_t mask) {
    retdec_trace_actor_window_state(phase, (int32_t)(intptr_t)actor, mask);
    retdec_trace_player_state(phase == 1 ? "before-script" : phase == 2 ? "after-script" : "after-motion",
        (int32_t)(intptr_t)actor, (int32_t)(intptr_t)camera);
}

extern "C" int32_t kinoko_actor_render_layer_update(void *storage,KinokoCamera *camera) {
    static volatile LONG trace_count;
    const auto trace=InterlockedIncrement(&trace_count);
    if (!storage) return 0;
    const kinoko::native::RecordView<RenderLayerRecord> layer(storage);
    if (trace<=16) {
        using kinoko::legacy::address;
        using Camera=kinoko::camera::Record;
        auto *global_camera=kinoko_game_objects()->camera;
        retdec_trace_i32("actor:layer-render-this",address(storage));
        retdec_trace_i32("actor:layer-render-arg",address(camera));
        retdec_trace_i32("actor:layer-render-camera",address(global_camera));
        retdec_trace_i32("actor:camera-x",camera_bits<int32_t>(global_camera,&Camera::x));
        retdec_trace_i32("actor:camera-y",camera_bits<int32_t>(global_camera,&Camera::y));
        retdec_trace_i32("actor:camera-cx",camera_bits<int32_t>(global_camera,&Camera::center_x));
        retdec_trace_i32("actor:camera-cy",camera_bits<int32_t>(global_camera,&Camera::center_y));
        retdec_trace_i32("actor:camera-width",camera_bits<int32_t>(global_camera,&Camera::width));
        retdec_trace_i32("actor:camera-height",camera_bits<int32_t>(global_camera,&Camera::height));
    }
    return kinoko_actor_manager_render_layer(layer.get(&RenderLayerRecord::manager),camera,layer.get(&RenderLayerRecord::index));
}
