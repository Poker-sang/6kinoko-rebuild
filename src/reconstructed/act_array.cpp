#include "kinoko/act_array.hpp"
#include "kinoko/legacy_memory.hpp"
#include <stdexcept>
namespace {
using kinoko::legacy::field;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::ActArray;
void publish(int32_t slot, ActArray* values) noexcept {
    const auto begin=values && !values->empty()?address(values->data()):0;
    field<int32_t>(slot)=begin;
    field<int32_t>(slot+4)=begin+(values?static_cast<int32_t>(values->size()*4):0);
    field<ActArray*>(slot+8)=values;
}
}
void kinoko::replace_act_array(int32_t slot, std::unique_ptr<ActArray> values) noexcept {
    delete field<ActArray*>(slot+8);
    publish(slot,values.release());
}
extern "C" int32_t kinoko_act_array_prepare(int32_t slot,uint32_t count) {
    if(count>0x10000) return 0;
    try {
        auto values=std::make_unique<ActArray>(count);
        kinoko::replace_act_array(slot,std::move(values));
        // The parser publishes each owned payload only after successful load.
        field<int32_t>(slot+4)=field<int32_t>(slot);
        return 1;
    } catch(...) { return 0; }
}
extern "C" void kinoko_act_array_clone(int32_t destination,int32_t source) {
    const auto begin=field<uint32_t>(source),end=field<uint32_t>(source+4);
    if(end<begin || (end-begin)%4 || (!begin && end) || (end-begin)/4>0x10000)
        throw std::length_error("ACT array");
    auto values=std::make_unique<ActArray>();
    if(end!=begin) values->assign(pointer<int32_t>(begin),pointer<int32_t>(end));
    kinoko::replace_act_array(destination,std::move(values));
}
extern "C" void kinoko_act_array_append(int32_t slot,int32_t value) {
    auto* values=field<ActArray*>(slot+8);
    if(!values) {
        auto owner=std::make_unique<ActArray>();owner->push_back(value);
        kinoko::replace_act_array(slot,std::move(owner));return;
    }
    if(values->size()>=0x10000) throw std::length_error("ACT array");
    values->push_back(value);publish(slot,values);
}
extern "C" void kinoko_act_array_destroy(int32_t slot) {
    delete field<ActArray*>(slot+8);publish(slot,nullptr);
}
