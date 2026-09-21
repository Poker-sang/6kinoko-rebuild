#include "kinoko/act_array.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/upstream_bindings.hpp"
#include <memory>
#include <string>
#include <unordered_set>

extern "C" int32_t g664;

namespace {
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::legacy::field;
namespace sqrat = kinoko::script::upstream;

struct LayerDelete {
    void operator()(unsigned char* layer) const {
        retdec_destroy_cact_layer(address(layer));
        std::free(layer);
    }
};
struct KeyDelete {
    void operator()(unsigned char* key) const { retdec_destroy_cact_key(address(key)); }
};

void assign_object(int32_t destination, int32_t source) {
    const auto old_vm=pointer<SQVM>(field<int32_t>(destination+4));
    const auto old_value=kinoko::legacy::load<HSQOBJECT>(pointer<void>(destination+8));
    if (old_vm) sqrat::sqrat_destroy_object(old_vm,old_value,field<uint8_t>(destination+16)!=0);
    // Original 41ECA0 copies VM, pair and ownership flag, retains the pair,
    // and leaves the destination Table/Instance vtable intact.
    std::memcpy(pointer<void>(destination+4),pointer<void>(source+4),13);
    const auto vm=pointer<SQVM>(field<int32_t>(destination+4));
    if (vm) sqrat::sqrat_retain(vm,kinoko::legacy::load<HSQOBJECT>(pointer<void>(destination+8)));
}

void clone_script_text(int32_t destination, int32_t source) {
    // 41EA50 calls GetText (415EA0), then SetText (415F60) after assignment.
    // GetText's result is ignored: compiled input becomes this exact marker;
    // SetText clears the filename and leaves the copied compiled flag intact.
    std::string text;
    const auto compiled=field<uint8_t>(source+101);
    if (compiled) text="/* This script is compiled. Can't read this. Don't edit this.*/";
    else {
        const auto size=field<uint32_t>(source+96);
        const auto bytes=pointer<const char>(field<int32_t>(source+92));
        if (size>0x1000000 || (size && !bytes)) throw std::bad_alloc();
        if (bytes && size) {
            const auto end=static_cast<const char*>(std::memchr(bytes,0,size));
            if (!end) throw std::bad_alloc();
            text.assign(bytes,end);
        }
    }
    kinoko::legacy::Allocation<char> buffer(static_cast<char*>(std::malloc(text.size()+1)));
    if (!buffer) throw std::bad_alloc();
    std::memcpy(buffer.get(),text.c_str(),text.size()+1);
    kinoko::legacy::StringView(pointer<void>(destination+64)).assign("",0);
    std::free(pointer<void>(field<int32_t>(destination+92)));
    field<int32_t>(destination+92)=address(buffer.release());
    field<uint32_t>(destination+96)=static_cast<uint32_t>(text.size()+1);
    field<uint8_t>(destination+100)=1;
    field<uint8_t>(destination+101)=compiled;
}

void clone_list(int32_t destination, int32_t source, int32_t offset, bool bind) {
    const auto head=field<int32_t>(source+offset);
    if (!head) return;
    std::unordered_set<int32_t> visited;
    for (auto node=field<int32_t>(head);node!=head;node=field<int32_t>(node)) {
        if (!node || visited.size()>=0x10000 || !visited.insert(node).second) throw std::bad_alloc();
        const auto original=field<int32_t>(node+8);
        if (!original) throw std::bad_alloc();
        const auto copy=retdec_call_thiscall0_result(pointer<void>(original),
            field<void*>(field<int32_t>(original)+20));
        std::unique_ptr<unsigned char,KeyDelete> owner(pointer<unsigned char>(copy));
        if (!copy || !retdec_act_append_list(destination+offset,copy)) throw std::bad_alloc();
        owner.release();
        ++field<int32_t>(destination+offset+4);
        const auto layout=field<int32_t>(copy+4);
        if (bind && layout) retdec_call_thiscall1_result(pointer<void>(layout),
            field<void*>(field<int32_t>(layout)+24),destination);
    }
}
}

// Original 41EA50 and its 41ECA0 assignment: constructor-owned containers,
// shallow hierarchy links, deep keys, and source-backed Sqrat reference copies.
extern "C" int32_t __fastcall kinoko_method_clone_act_layer(int32_t source, void*) {
    if (!source) return 0;
    try {
        kinoko::legacy::Allocation<unsigned char> storage(static_cast<unsigned char*>(std::calloc(1,348)));
        if (!storage || !retdec_construct_cact_layer(address(storage.get()),g664)) return 0;
        std::unique_ptr<unsigned char,LayerDelete> owned(storage.release());
        const auto result=address(owned.get());
        std::memcpy(pointer<void>(result+4),pointer<void>(source+4),68);
        kinoko_act_array_clone(result+72,source+72);
        field<int32_t>(result+88)=field<int32_t>(source+88);
        field<uint8_t>(result+92)=field<uint8_t>(source+92);
        std::memcpy(pointer<void>(result+96),pointer<void>(source+96),16);
        const kinoko::legacy::StringView input(pointer<void>(source+112)),output(pointer<void>(result+112));
        output.assign(input.data(),input.length());
        if (output.length()!=input.length()) return 0;
        field<uint16_t>(result+140)=field<uint16_t>(source+140);
        std::memcpy(pointer<void>(result+144),pointer<void>(source+144),36);
        assign_object(result+308,source+308);
        assign_object(result+328,source+328);
        clone_script_text(result+204,source+204);
        clone_list(result,source,180,true);
        clone_list(result,source,192,false);
        return address(owned.release());
    } catch (...) { return 0; }
}
