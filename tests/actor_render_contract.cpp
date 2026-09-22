#include "kinoko/actor_render.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace kinoko::actor;
using kinoko::legacy::address;
namespace { std::vector<int> blends; int submissions=0; }
extern "C" {
float function_404130(long double degrees) { return static_cast<float>(std::cos(degrees*3.141592653589793/180.0)); }
float function_4040d0(long double degrees) { return static_cast<float>(std::sin(degrees*3.141592653589793/180.0)); }
int32_t kinoko_actor_render_trace_begin(KinokoActor *,KinokoCamera *) { return 1; }
void kinoko_actor_render_trace_draw(KinokoActor *) {}
void kinoko_actor_render_trace_end(int32_t,int32_t) {}
void kinoko_actor_render_set_blend(int32_t mode) { blends.push_back(mode); }
int32_t kinoko_actor_render_submit(KinokoAnimationFrame *) { ++submissions;return -1; }
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"render line %d: %s\n",__LINE__,#x);return 1; } } while(0)
int main() {
    FrameRecord frame{};frame.texture=1;
    frame.base_positions={Position3{0,0,3},Position3{8,0,3},Position3{0,4,3},Position3{8,4,3}};
    ActorRecord actor{};
    actor.current_frame=reinterpret_cast<KinokoAnimationFrame *>(&frame);actor.sprite_frame=actor.current_frame;
    actor.active=actor.visible=1;actor.direction=-1;actor.scale=actor.scale_x=actor.scale_y=1;
    actor.x=-0.25f;actor.y=0.25f;actor.alpha=actor.red=actor.green=actor.blue=255;actor.blend=4;
    auto *receiver=reinterpret_cast<KinokoActor *>(&actor);
    CHECK(kinoko_actor_render(receiver,nullptr)==1);
    CHECK(frame.positions[0].x==-1 && frame.positions[0].y==1 && frame.positions[0].z==3);
    CHECK(frame.positions[3].x==7 && frame.positions[3].y==5);
    CHECK((blends==std::vector<int>{4,1})); // reset even when submission fails
    CHECK(frame.base_positions[0].x==0 && frame.base_positions[3].x==8);
    actor.direction=1;actor.x=actor.y=0;actor.scale_x=2;frame.pivot_x=2;
    CHECK(kinoko_actor_render(receiver,nullptr)==1);
    CHECK(frame.positions[0].x==4 && frame.positions[1].x==-12);
    FrameAppearance appearance{};appearance.color=0x80402010u;frame.owned_payload=&appearance;
    actor.alpha=actor.red=actor.green=actor.blue=128;
    CHECK(kinoko_actor_render(receiver,nullptr)==1);
    CHECK(frame.vertices[0].color==0x40201008u);
    CHECK(kinoko_actor_render(receiver,nullptr)==1 && frame.vertices[0].color==0x40201008u);
    actor.rotation=90;actor.direction=-1;actor.scale_x=1;frame.pivot_x=0;
    CHECK(kinoko_actor_render(receiver,nullptr)==1);
    CHECK(std::fabs(frame.positions[1].y-8)<1.1f);
    CameraBoundsRecord camera{};camera.bounds={0,0,10,10};camera.center_x=100;camera.center_y=100;
    actor.world_bounds={267,0,268,1};
    const auto before=submissions;
    CHECK(kinoko_actor_render(receiver,reinterpret_cast<KinokoCamera *>(&camera))==0 && submissions==before);
    actor.visible=0;CHECK(!kinoko_actor_render(receiver,nullptr));
    actor.visible=1;frame.texture=0;CHECK(!kinoko_actor_render(receiver,nullptr));
    std::puts("PASS: Actor quad transform, signed pixel rounding, appearance reset and blend lifetime");
}
