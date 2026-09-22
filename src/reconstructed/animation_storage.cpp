#include "kinoko/animation_storage.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <vector>
#include <memory>
namespace {
using namespace kinoko::legacy;
using namespace kinoko::actor;
struct Animation {
    AnimationRecord record{};
    std::vector<FrameRecord> frames;
    explicit Animation(uint32_t count):frames(count) {
        record.frames_begin=address(frames.data());
        record.frames_end=record.frames_begin+count*sizeof(FrameRecord);
        field<uint32_t>(address(&record)+16)=record.frames_end;
    }
    ~Animation() { for(auto& frame:frames) std::free(frame.owned_payload); }
};
static_assert(offsetof(Animation,record)==0);
using Animations=std::list<std::unique_ptr<Animation>>;
Animations*& storage(int32_t list) { return field<Animations*>(list); }
}
extern "C" void kinoko_animation_list_construct(int32_t list) { storage(list)=new Animations;field<int32_t>(list+4)=0; }
extern "C" void kinoko_animation_list_destroy(int32_t list) { delete storage(list);storage(list)=nullptr;field<int32_t>(list+4)=0; }
extern "C" int32_t kinoko_animation_create(uint32_t frames) { return address(new Animation(frames)); }
extern "C" void kinoko_animation_discard(int32_t animation) { delete pointer<Animation>(animation); }
extern "C" void kinoko_animation_adopt(int32_t list,int32_t animation) {
    if(!storage(list)) kinoko_animation_list_construct(list);
    storage(list)->push_back(std::unique_ptr<Animation>(pointer<Animation>(animation)));
    field<int32_t>(list+4)=static_cast<int32_t>(storage(list)->size());
}
extern "C" int32_t kinoko_clear_animation_list(int32_t list) {
    if(list && storage(list)) storage(list)->clear();
    if(list) field<int32_t>(list+4)=0;return 0;
}
