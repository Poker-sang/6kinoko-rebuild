#include "kinoko/pat_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/animation_storage.h"
#include "kinoko/integer_map.h"
#include "kinoko/integer_vector.h"
#include "kinoko/texture_store.h"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
struct CloseReader {
    void operator()(KinokoArchiveReader *reader) const { retdec_destroy_reader(reinterpret_cast<int32_t *>(reader)); }
};
struct DiscardAnimation {
    void operator()(KinokoAnimation *animation) const { kinoko_animation_discard(address(animation)); }
};
using PendingAnimation=std::unique_ptr<KinokoAnimation,DiscardAnimation>;
class Reader {
    KinokoArchiveReader *reader_; // borrowed; caller owns file lifetime
public:
    explicit Reader(KinokoArchiveReader *reader):reader_(reader) {}
    bool bytes(void *value,uint32_t size) const { return retdec_reader_read_exact(address(reader_),value,size)!=0; }
    template<class T> bool read(T &value) const { return bytes(&value,sizeof(value)); }
    bool skip(uint32_t size) const {
        std::array<unsigned char,256> scratch;
        while (size) {
            const uint32_t count=size<scratch.size()?size:static_cast<uint32_t>(scratch.size());
            if (!bytes(scratch.data(),count)) return false;
            size-=count;
        }
        return true;
    }
};
bool publish(const ManagerView manager,int32_t take,KinokoAnimation *animation) {
    const auto lookup=manager.view(&ManagerPrefix::animation_lookup);
    auto head=lookup.get(&TreeIndex::head);
    if (!head) {
        head=static_cast<Address>(kinoko_integer_map_create());
        lookup.set(&TreeIndex::head,head);
    }
    const auto result=kinoko_integer_map_put(head,take,address(animation));
    lookup.set(&TreeIndex::count,static_cast<int32_t>(kinoko_integer_map_size(head)));
    return result!=0;
}
bool read_frame(const Reader &reader,KinokoActorManager *manager,KinokoAnimation *animation,
    KinokoAnimationFrame *frame,int32_t &duration,uint32_t resource_base) {
    KinokoPatFrameFields fields{};
    if (!reader.read(fields.resource_index) || !reader.read(fields.sprite_x) || !reader.read(fields.sprite_y) ||
        !reader.read(fields.source_width) || !reader.read(fields.source_height) ||
        !reader.read(fields.offset_x) || !reader.read(fields.offset_y) ||
        !reader.read(fields.duration) || !reader.read(fields.type)) return false;
    if (fields.type==2) {
        if (!reader.read(fields.auxiliary_mode)) return false;
        for (size_t i=0;i<4;++i) if (!reader.read(fields.auxiliary_bytes[3-i])) return false;
        for (auto &value:fields.auxiliary_values) if (!reader.read(value)) return false;
    }
    // The runtime only consumes the first bounds record. Retain every on-disk
    // read (including discarded editor fields), so the following frame starts
    // at exactly the original byte offset.
    uint16_t word;uint32_t dword;uint8_t byte,has_bounds;
    for (int i=0;i<20;++i) if (!reader.read(word)) return false;
    if (!reader.read(byte) || !reader.read(dword) || !reader.read(dword) || !reader.read(has_bounds)) return false;
    if (has_bounds) {
        std::array<int32_t,4> bounds;
        if (!reader.read(bounds)) return false;
        const RecordView<AnimationRecord> node(animation);
        if (!node.get(&AnimationRecord::has_bounds)) {
            node.set(&AnimationRecord::has_bounds,uint8_t{1});
            node.set(&AnimationRecord::left,bounds[0]);node.set(&AnimationRecord::top,bounds[1]);
            node.set(&AnimationRecord::right,bounds[2]);node.set(&AnimationRecord::bottom,bounds[3]);
        }
    }
    uint8_t plain_count,extra_count,has_extra;
    if (!reader.read(plain_count) || !reader.skip(plain_count*16u) || !reader.read(extra_count)) return false;
    for (uint32_t i=0;i<extra_count;++i) {
        if (!reader.skip(16) || !reader.read(has_extra) || (has_extra && !reader.skip(16))) return false;
    }
    for (int i=0;i<3;++i) if (!reader.read(dword) || !reader.read(dword)) return false;
    for (int i=0;i<3;++i) if (!reader.read(word)) return false;
    if (!kinoko_pat_build_frame(manager,frame,&fields,resource_base)) return false;
    duration=static_cast<int32_t>(static_cast<uint32_t>(duration)+static_cast<uint32_t>(fields.duration));
    return true;
}
int32_t load_texture(const char *directory,const char *name) {
    char path[260]{};
    retdec_trace_squirrel_name("actor:pat-texture",address(name));
    if (strcpy_s(path,sizeof(path),directory)) return 0;
    auto length=std::strlen(path);
    if (length && path[length-1]!='/' && path[length-1]!='\\') {
        if (length+1>=sizeof(path)) return 0;
        path[length++]='\\';path[length]=0;
    }
    if (strcat_s(path,sizeof(path),name)) return 0;
    return kinoko_texture_acquire(path);
}
}

// 464F80: register each complete node as it is read; pending nodes have scoped
// ownership. Published nodes/textures remain with the manager on later failure.
extern "C" int32_t kinoko_pat_read_animations(KinokoArchiveReader *stream,KinokoActorManager *receiver,uint32_t resource_base) {
    const Reader reader(stream);
    const ManagerView manager(receiver);
    uint32_t item_count;
    if (!reader.read(item_count) || item_count>4096u) return 0; // inherited guard
    std::vector<std::array<int32_t,2>> aliases;
    aliases.reserve(item_count);
    KinokoAnimation *head=nullptr,*tail=nullptr;
    for (uint32_t i=0;i<item_count;++i) {
        int32_t take;
        if (!reader.read(take)) return 0;
        if (take==-1) {
            std::array<int32_t,2> alias;
            if (!reader.read(alias)) return 0;
            aliases.push_back(alias);
            continue;
        }
        uint16_t header0,header1;uint8_t loops;uint32_t count;
        if (!reader.read(header0) || !reader.read(header1) || !reader.read(loops) ||
            !reader.read(count) || count>4096u) return 0;
        PendingAnimation allocation(pointer<KinokoAnimation>(kinoko_animation_create(count)));
        if (!allocation) return 0;
        const RecordView<AnimationRecord> node(allocation.get());
        node.set(&AnimationRecord::loops,loops);
        auto *frames=pointer<FrameRecord>(node.get(&AnimationRecord::frames_begin));
        int32_t duration=0;
        for (uint32_t frame=0;frame<count;++frame)
            if (!read_frame(reader,receiver,allocation.get(),reinterpret_cast<KinokoAnimationFrame *>(frames+frame),duration,resource_base)) return 0;
        node.set(&AnimationRecord::flags,duration);
        if (take==-2) {
            if (!tail) return 0;
            RecordView<AnimationRecord>(tail).set(&AnimationRecord::next,allocation.get());
            node.set(&AnimationRecord::next,head);
            node.set(&AnimationRecord::previous,tail);
        } else {
            if (!publish(manager,take,allocation.get())) return 0;
            head=allocation.get();
            node.set(&AnimationRecord::next,head);
        }
        auto *adopted=allocation.get();
        kinoko_animation_adopt(address(manager.bytes(&ManagerPrefix::animations)),address(adopted));
        allocation.release();tail=adopted;
    }
    // 465DA9..465E7B: reverse order is observable with chained/duplicate aliases.
    for (auto alias=aliases.rbegin();alias!=aliases.rend();++alias) {
        const auto lookup=manager.get(&ManagerPrefix::animation_lookup).head;
        if (!lookup) continue;
        const auto entry=kinoko_integer_map_find(lookup,(*alias)[1]);
        if (static_cast<Address>(entry)!=lookup &&
            !publish(manager,(*alias)[0],pointer<KinokoAnimation>(*pointer<int32_t>(entry)))) return 0;
    }
    retdec_trace_i32("animation:items",item_count);
    return 1;
}

extern "C" int32_t kinoko_pat_load(KinokoActorManager *receiver,const char *file_name,const char *directory) {
    static std::atomic<int32_t> traces{};
    const auto trace=++traces;
    if (!receiver || !file_name || !directory) return 0;
    if (trace<=32) {
        retdec_trace_squirrel_name("actor:pat-path",address(file_name));
        retdec_trace_squirrel_name("actor:pat-directory",address(directory));
    }
    int32_t reader_slot=0;
    if (!function_407370(address(&reader_slot),file_name)) return 0;
    const std::unique_ptr<KinokoArchiveReader,CloseReader> owner(pointer<KinokoArchiveReader>(reader_slot));
    const Reader reader(owner.get());
    const ManagerView manager(receiver);
    const auto resources=address(manager.bytes(&ManagerPrefix::textures));
    // The lambda gives one exit for the final trace, then the reader closes.
    const auto result=[&]() -> int32_t {
        uint8_t version;uint16_t count;
        if (!reader.read(version) || !reader.read(count)) return 0;
        const auto base=kinoko_integer_vector_size(resources);
        if (count>4096u) return 0;
        for (uint32_t i=0;i<count;++i) {
            char name[129]{};
            if (!reader.bytes(name,128)) return 0;
            const auto handle=load_texture(directory,name);
            kinoko_integer_vector_append(resources,handle); // preserve missing handle slot
        }
        if (!kinoko_pat_read_animations(owner.get(),receiver,base)) return 0;
        retdec_trace_i32("animation:header",version);
        retdec_trace_i32("animation:resources",count);
        retdec_trace_i32("animation:resource-base",base);
        return 1;
    }();
    if (trace<=32) retdec_trace_i32("actor:pat-result",result);
    return result;
}
