#include "kinoko/native_buffer.h"
#include "kinoko/native_record_view.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
namespace {
using Buffer=std::vector<uint32_t>;
struct BufferRecord { uint32_t *begin, *end; Buffer* owner; };
using BufferView=kinoko::native::RecordView<BufferRecord>;
static_assert(sizeof(BufferRecord)==12);
void check(uint32_t bytes) {
    if(bytes%4 || bytes>0x7ffffffc) throw std::length_error("native record buffer");
}
void publish(void* slot,Buffer* owner) {
    auto* begin=owner && !owner->empty()?owner->data():nullptr;
    const BufferRecord value{begin,begin?begin+owner->size():nullptr,owner};
    std::memcpy(slot,&value,sizeof(value));
}
}
extern "C" int32_t kinoko_native_buffer_resize(void* slot,uint32_t bytes) {
    try {
        check(bytes);
        auto* owner=BufferView(slot).get(&BufferRecord::owner);
        if(owner) {owner->resize(bytes/4);publish(slot,owner);}
        else {
            auto value=std::make_unique<Buffer>(bytes/4);
            publish(slot,value.release());
        }
        return 1;
    } catch(...) {return 0;}
}
extern "C" void kinoko_native_buffer_replace(void* slot,const void* data,uint32_t bytes) {
    check(bytes);
    auto value=std::make_unique<Buffer>(bytes/4);
    if(bytes) std::memcpy(value->data(),data,bytes);
    delete BufferView(slot).get(&BufferRecord::owner);
    publish(slot,value.release());
}
extern "C" void kinoko_native_buffer_destroy(void* slot) {
    delete BufferView(slot).get(&BufferRecord::owner);publish(slot,nullptr);
}
extern "C" int32_t kinoko_native_buffer_ensure(void* slot,uint32_t bytes) {
    try {
        check(bytes);
        const BufferView view(slot);
        const auto previous=view.load();
        auto* owner=previous.owner;
        // Preserve the original unsigned range validation even for malformed
        // fixture ranges, without subtracting pointers to unrelated objects.
        const auto used=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(previous.end)-
            reinterpret_cast<uintptr_t>(previous.begin));
        if(used%4 || used>bytes && (!owner || used>owner->size()*4)) return 0;
        if(owner && owner->size()*4>=bytes) return 1;
        if(!kinoko_native_buffer_resize(slot,bytes)) return 0;
        auto* begin=view.get(&BufferRecord::begin);
        view.set(&BufferRecord::end,begin?begin+used/sizeof(uint32_t):nullptr);
        return 1;
    } catch(...) {return 0;}
}
