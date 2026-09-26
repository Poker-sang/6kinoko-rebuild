#include "kinoko/animation_storage.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/integer_map.h"
#include "kinoko/integer_vector.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <vector>
#include <memory>
namespace {
using namespace kinoko::legacy;
using namespace kinoko::actor;
using kinoko::native::RecordView;
struct Animation {
    AnimationRecord record{};
    std::vector<FrameRecord> frames;
    explicit Animation(uint32_t count):frames(count) {
        record.frames_begin=reinterpret_cast<KinokoAnimationFrame *>(frames.data());
        record.frames_end=count?reinterpret_cast<KinokoAnimationFrame *>(frames.data()+count):record.frames_begin;
        record.frames_capacity=record.frames_end;
    }
    ~Animation() {
        // Each appearance belongs to one frame. Aliases and linked takes
        // never recursively destroy one another.
        for(auto &frame:frames) std::free(frame.owned_payload);
    }
};
static_assert(offsetof(Animation,record)==0);
using Animations=std::list<std::unique_ptr<Animation>>;
struct ListStorage { Animations *owner; uint32_t count; };
static_assert(sizeof(ListStorage)==sizeof(ListIndex));
using ListView=RecordView<ListStorage>;
ListView view(void* list) { return ListView(list); }
void adopt(const ListView list,Animation *animation) {
    auto *owner=list.get(&ListStorage::owner);
    if (!owner) {
        owner=new Animations;
        list.set(&ListStorage::owner,owner);
    }
    // Allocate the list node before taking ownership: the pending owner
    // remains responsible if emplace throws.
    owner->emplace_back();
    owner->back().reset(animation);
    list.set(&ListStorage::count,static_cast<uint32_t>(owner->size()));
}
}
extern "C" KinokoAnimation *kinoko_animation_allocate(uint32_t count) {
    return reinterpret_cast<KinokoAnimation *>(new Animation(count));
}
extern "C" void kinoko_animation_release(KinokoAnimation *animation) {
    delete reinterpret_cast<Animation *>(animation);
}
extern "C" void kinoko_animation_manager_adopt(KinokoActorManager *manager,KinokoAnimation *animation) {
    adopt(ListView(ManagerView(manager).bytes(&ManagerPrefix::animations)),reinterpret_cast<Animation *>(animation));
}
extern "C" int32_t kinoko_animation_bind(KinokoActorManager *manager,int32_t take,KinokoAnimation *animation) {
    const auto index=ManagerView(manager).view(&ManagerPrefix::animation_lookup);
    auto head=index.get(&KinokoIntegerMapIndex::owner);
    if (!head) {
        head=kinoko_integer_map_create();index.set(&KinokoIntegerMapIndex::owner,head);
    }
    // The legacy map owns integer slots, not the pointed-to animations.
    const auto result=kinoko_integer_map_put(head,take,address(animation));
    index.set(&KinokoIntegerMapIndex::count,static_cast<int32_t>(kinoko_integer_map_size(head)));
    return result!=0;
}
extern "C" KinokoAnimation *kinoko_animation_find(KinokoActorManager *manager,int32_t take) {
    const auto head=ManagerView(manager).get(&ManagerPrefix::animation_lookup).owner;
    if (!head) return nullptr;
    const auto slot=kinoko_integer_map_find(head,take);
    return slot?pointer<KinokoAnimation>(*slot):nullptr;
}
extern "C" void kinoko_animation_add_texture(KinokoActorManager *manager,int32_t handle) {
    kinoko_integer_vector_append((KinokoIntegerVector*)(ManagerView(manager).bytes(&ManagerPrefix::textures)), handle);
}
// Container entry points receive the owner slot directly.
extern "C" void kinoko_animation_list_construct(void* list) {
    view(list).set(&ListStorage::owner,new Animations);view(list).set(&ListStorage::count,uint32_t{0});
}
extern "C" void kinoko_animation_list_destroy(void* list) {
    delete view(list).get(&ListStorage::owner);
    view(list).set(&ListStorage::owner,static_cast<Animations *>(nullptr));view(list).set(&ListStorage::count,uint32_t{0});
}
extern "C" int32_t kinoko_animation_create(uint32_t frames) { return address(kinoko_animation_allocate(frames)); }
extern "C" void kinoko_animation_discard(int32_t animation) { kinoko_animation_release(pointer<KinokoAnimation>(animation)); }
extern "C" void kinoko_animation_adopt(int32_t list,int32_t animation) { adopt(view(pointer<void>(list)),pointer<Animation>(animation)); }
extern "C" int32_t kinoko_clear_animation_list(void* list) {
    if (list) {
        if (auto *owner=view(list).get(&ListStorage::owner)) owner->clear();
        view(list).set(&ListStorage::count,uint32_t{0});
    }
    return 0;
}
