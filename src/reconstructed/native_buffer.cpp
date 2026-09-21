#include "kinoko/native_buffer.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
namespace {
using kinoko::legacy::field;
using kinoko::legacy::address;
using Buffer=std::vector<uint32_t>;
void check(uint32_t bytes) {
    if(bytes%4 || bytes>0x7ffffffc) throw std::length_error("native record buffer");
}
void publish(int32_t slot,Buffer* owner) {
    const auto begin=owner && !owner->empty()?address(owner->data()):0;
    field<uint32_t>(slot)=begin;
    field<uint32_t>(slot+4)=begin+(owner?static_cast<uint32_t>(owner->size()*4):0);
    field<Buffer*>(slot+8)=owner;
}
}
extern "C" int32_t kinoko_native_buffer_resize(int32_t slot,uint32_t bytes) {
    try {
        check(bytes);
        auto* owner=field<Buffer*>(slot+8);
        if(owner) {owner->resize(bytes/4);publish(slot,owner);}
        else {
            auto value=std::make_unique<Buffer>(bytes/4);
            publish(slot,value.release());
        }
        return 1;
    } catch(...) {return 0;}
}
extern "C" void kinoko_native_buffer_replace(int32_t slot,const void* data,uint32_t bytes) {
    check(bytes);
    auto value=std::make_unique<Buffer>(bytes/4);
    if(bytes) std::memcpy(value->data(),data,bytes);
    delete field<Buffer*>(slot+8);
    publish(slot,value.release());
}
extern "C" void kinoko_native_buffer_destroy(int32_t slot) {
    delete field<Buffer*>(slot+8);publish(slot,nullptr);
}

extern "C" int32_t kinoko_native_buffer_ensure(int32_t slot,uint32_t bytes) {
    try {
        check(bytes);
        auto* owner=field<Buffer*>(slot+8);
        const auto used=field<uint32_t>(slot+4)-field<uint32_t>(slot);
        if(used%4 || used>bytes && (!owner || used>owner->size()*4)) return 0;
        if(owner && owner->size()*4>=bytes) return 1;
        if(!kinoko_native_buffer_resize(slot,bytes)) return 0;
        field<uint32_t>(slot+4)=field<uint32_t>(slot)+used;
        return 1;
    } catch(...) {return 0;}
}
