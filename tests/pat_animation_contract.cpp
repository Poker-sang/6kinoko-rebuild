#include "kinoko/file_io.h"
#include "kinoko/pat_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/animation_storage.h"
#include "kinoko/integer_vector.h"
#include "kinoko/integer_map.h"
#include "kinoko/texture_store.h"
#include "kinoko/legacy_memory.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
struct KinokoArchiveReader { std::vector<unsigned char> bytes; size_t cursor=0; };
namespace {
KinokoArchiveReader stream;
int closes=0,loads=0;
std::string loaded_path;
template<class T> void put(T value) {
    const auto *p=reinterpret_cast<const unsigned char *>(&value);
    stream.bytes.insert(stream.bytes.end(),p,p+sizeof(value));
}
void zero(size_t count) { stream.bytes.insert(stream.bytes.end(),count,0); }
void frame(int16_t duration,bool first) {
    put(uint32_t{0});put(int16_t{8});put(int16_t{16});put(int16_t{8});put(int16_t{4});
    put(int16_t{0});put(int16_t{0});put(duration);put(uint8_t{2});put(int16_t{0});
    for (uint8_t c:{128,64,32,16}) put(c);
    for (uint16_t v:{100,100,90,90,0}) put(v);
    zero(40);put(uint8_t{0});zero(8);put(uint8_t{1});
    for (int32_t v:(first?std::array<int32_t,4>{-2,-3,6,8}:std::array<int32_t,4>{0,0,99,99})) put(v);
    put(uint8_t{1});zero(16);put(uint8_t{1});zero(16);put(uint8_t{1});zero(16);
    zero(24);zero(6);
}
}
extern "C" {
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
int32_t kinoko_reader_open(KinokoArchiveReader **output,const char *) { *output=&stream;stream.cursor=0;return 1; }
void kinoko_reader_close(KinokoArchiveReader *) { ++closes; }
int32_t kinoko_reader_read_exact(KinokoArchiveReader *source,void *output,uint32_t size) {
    auto &reader=*source;
    if (size>reader.bytes.size()-reader.cursor) return 0;
    std::memcpy(output,reader.bytes.data()+reader.cursor,size);reader.cursor+=size;return 1;
}
int32_t kinoko_texture_acquire(const char *name) { ++loads;loaded_path=name;return 2; }
const void *kinoko_pat_frame_methods(void) { return nullptr; }
void kinoko_trace_i32(const char *,int32_t) {}
void kinoko_trace_squirrel_name(const char *,int32_t) {}
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"PAT line %d: %s\n",__LINE__,#x);return 1; } } while(0)
int main() {
    ManagerPrefix manager{};auto *receiver=reinterpret_cast<KinokoActorManager *>(&manager);
    kinoko_animation_list_construct((void*)(uintptr_t)(address(&manager.animations)));
    kinoko_integer_vector_construct((KinokoIntegerVector*)(&manager.textures));
    kinoko_integer_vector_append((KinokoIntegerVector*)(&manager.textures), 1);
    kinoko_texture_slots[1].width=64;kinoko_texture_slots[1].height=64;
    kinoko_texture_slots[2].width=256;kinoko_texture_slots[2].height=128;
    put(uint8_t{5});put(uint16_t{1});
    const char name[]="sprite.png";for (char c:std::string(name)) put(c);zero(128-std::strlen(name));
    put(uint32_t{3}); // forward alias, head, continuation
    put(int32_t{-1});put(int32_t{10});put(int32_t{20});
    put(int32_t{20});put(uint16_t{0});put(uint16_t{0});put(uint8_t{1});put(uint32_t{2});
    frame(2,true);frame(-1,false);
    put(int32_t{-2});put(uint16_t{0});put(uint16_t{0});put(uint8_t{0});put(uint32_t{0});
    CHECK(kinoko_pat_load(receiver,"animation.pat","data/player"));
    CHECK(stream.cursor==stream.bytes.size() && closes==1 && loads==1);
    CHECK(loaded_path=="data/player\\sprite.png");
    CHECK(manager.animations.count==2 && manager.animation_lookup.count==2);
    const auto lookup=manager.animation_lookup.owner;
    const auto head_pointer=kinoko_animation_find(receiver,20);
    const auto head_address=address(head_pointer);
    CHECK(kinoko_animation_find(receiver,10)==head_pointer);
    const auto head=kinoko::native::RecordView<AnimationRecord>(pointer<void>(head_address)).load();
    CHECK(head.duration_total==1 && head.left==-2 && head.bottom==8 && head.has_bounds);
    CHECK(head.next && head.previous==nullptr);
    const auto tail=kinoko::native::RecordView<AnimationRecord>(head.next).load();
    CHECK(tail.next==pointer<KinokoAnimation>(head_address) && tail.previous==tail.next);
    const auto &first=*reinterpret_cast<FrameRecord *>(head.frames_begin);
    CHECK(first.texture==2 && first.duration==2 && first.owned_payload);
    CHECK(first.owned_payload->blend==1 && first.owned_payload->color==0x80402010u);
    CHECK(first.vertices[0].u==8.0f/256 && first.vertices[0].v==16.0f/128);
    CHECK(std::fabs(first.base_positions[3].y+8)<0.001f && std::fabs(first.base_positions[3].z+4)<0.001f);
    FrameRecord invalid{};KinokoPatFrameFields fields{};fields.resource_index=1;fields.type=1;
    CHECK(kinoko_pat_build_frame(receiver,reinterpret_cast<KinokoAnimationFrame *>(&invalid),&fields,1));
    CHECK(!invalid.texture); // base+index == resource count is not a valid handle
    // One complete node, then a truncated node: completed ownership is kept;
    // pending allocation and its first frame appearance are reclaimed.
    stream.bytes.clear();stream.cursor=0;put(uint32_t{2});
    for (int32_t take:{30,40}) {
        put(take);put(uint16_t{0});put(uint16_t{0});put(uint8_t{0});put(uint32_t{2});
        frame(1,true);if(take==30)frame(1,false);
    }
    CHECK(!kinoko_pat_read_animations(&stream,receiver,1));
    CHECK(manager.animations.count==3);
    CHECK(kinoko_animation_find(receiver,40)==nullptr);
    kinoko_animation_lookup_clear(receiver);
    kinoko_clear_animation_list((void*)(uintptr_t)(address(&manager.animations)));
    CHECK(manager.animations.count==0);
    kinoko_clear_animation_list((void*)(uintptr_t)(address(&manager.animations)));
    kinoko_animation_list_destroy((void*)(uintptr_t)(address(&manager.animations)));
    kinoko_integer_vector_destroy((KinokoIntegerVector*)(&manager.textures));kinoko_animation_lookup_destroy(receiver);
    std::puts("PASS: PAT byte alignment, resource base, aliases, linked takes, 3-axis signs and partial ownership");
}
