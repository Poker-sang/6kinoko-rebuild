#include "kinoko/act_clone.h"
#include "kinoko/texture_store.h"
#include "kinoko/act_runtime.h"
#include "kinoko/boost_control.hpp"
#include "kinoko/act_host.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/legacy_method_entries.h"
#include <memory>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <unordered_map>
#include <vector>

extern "C" {
extern unsigned char g327, g313;
extern int32_t g483, g484;
int32_t retdec_act_bind_layouts(int32_t act);
}

namespace {
namespace up = kinoko::native::upstream;
using Address = uint32_t;
template<class T> T &field(Address address, size_t offset=0) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(address)+offset);
}
void *pointer(Address address) { return reinterpret_cast<void *>(static_cast<uintptr_t>(address)); }
Address address(const void *p) { return static_cast<Address>(reinterpret_cast<uintptr_t>(p)); }
void dispose_chip_data(void* data) { retdec_mcd_free(static_cast<retdec_mcd_data*>(data)); }

struct String { char storage[16]; uint32_t size, capacity; };
struct Vector { Address begin, end, capacity; };
struct Node { Address next, previous, value; };
static_assert(sizeof(String)==24 && sizeof(Vector)==12 && sizeof(Node)==12);

// These are the original object fields; the C++ containers below own only
// temporary clone bookkeeping, never memory interpreted by the old ABI.
enum : size_t {
    act_script=100, act_layers=208, act_resources=224,
    layer_children=72, layer_parent=88, layer_resource=100,
    layer_id=104, layer_name=112, layer_keys=180, layer_events=192,
    layer_script=204, map_records=264
};

class Clone {
public:
    ~Clone() {
        if (committed_) return;
        for(auto texture: textures_) kinoko_texture_release(texture);
        for(auto resource: chip_owners_) kinoko_act_release_chip_data(static_cast<int32_t>(resource));
        for(auto allocation: allocations_) std::free(allocation);
    }

    Address act(Address source) {
        const Address result=copy(source,240);
        string(result,source,16);
        string(result,source,44);
        script(result+act_script,source+act_script);
        field<uint8_t>(result,204)=0;
        const auto resources=clone_vector(result,source,act_resources);
        const auto layers=clone_vector(result,source,act_layers);
        std::unordered_map<Address,Address> layer_map;
        for(Address slot=resources.begin;slot!=resources.end;slot+=4) {
            const auto original=field<Address>(slot);
            if(original) field<Address>(slot)=resource(original);
        }
        for(Address slot=layers.begin;slot!=layers.end;slot+=4) {
            const auto original=field<Address>(slot);
            if(original) field<Address>(slot)=layer_map[original]=layer(original);
        }
        for(const auto &pair:layer_map) {
            auto &parent=field<Address>(pair.second,layer_parent);
            if(parent) parent=layer_map.at(parent);
            const auto &children=field<Vector>(pair.second,layer_children);
            for(Address slot=children.begin;slot!=children.end;slot+=4)
                field<Address>(slot)=layer_map.at(field<Address>(slot));
        }
        // Reassociate resources by ID and rebind each copied layout's layer,
        // including its alpha/blend pointer properties (original 427D49..E89).
        if (!retdec_act_bind_layouts(static_cast<int32_t>(result))) throw std::bad_alloc();
        committed_=true;
        return result;
    }

private:
    Address allocate(size_t bytes) {
        void *p=std::calloc(1,bytes ? bytes : 1);
        if(!p) throw std::bad_alloc();
        try { allocations_.push_back(p); } catch(...) { std::free(p); throw; }
        return address(p);
    }
    Address copy(Address source,size_t bytes) {
        Address result=allocate(bytes);
        if(bytes) std::memcpy(pointer(result),pointer(source),bytes);
        return result;
    }
    void string(Address dest,Address source,size_t offset) {
        const auto &input=field<String>(source,offset);
        auto &output=field<String>(dest,offset);
        output={};
        output.size=input.size;
        output.capacity=input.size<16 ? 15 : input.size;
        const char *text=input.capacity<16 ? input.storage :
            static_cast<const char *>(pointer(field<Address>(source,offset)));
        char *target=output.storage;
        if(input.size>=16) {
            const auto buffer=allocate(static_cast<size_t>(input.size)+1);
            field<Address>(dest,offset)=buffer;
            target=static_cast<char *>(pointer(buffer));
        }
        if(input.size) std::memcpy(target,text,input.size);
        target[input.size]=0;
    }
    Vector clone_vector(Address dest,Address source,size_t offset) {
        const auto input=field<Vector>(source,offset);
        if(input.end<input.begin) throw std::bad_alloc();
        const auto bytes=input.end-input.begin;
        auto &out=field<Vector>(dest,offset);
        if(!input.begin && !bytes) { out={}; return out; }
        out.begin=copy(input.begin,bytes);
        out.end=out.capacity=out.begin+bytes;
        return out;
    }
    void script(Address dest,Address source) {
        // 416700 copies script text/bytecode, not live callback environments.
        std::memcpy(pointer(dest),pointer(source),104);
        for(size_t offset: {4u,24u,44u}) {
            field<Address>(dest,offset)=0;
            field<int32_t>(dest,offset+4)=g483;
            field<int32_t>(dest,offset+8)=g484;
            field<int32_t>(dest,offset+12)=g483;
            field<int32_t>(dest,offset+16)=g484;
        }
        string(dest,source,64);
        const auto size=field<uint32_t>(source,96);
        field<Address>(dest,92)=copy(field<Address>(source,92),size);
    }
    Address layout(Address source) {
        const bool map=field<Address>(source)==address(&g327);
        const auto result=copy(source,map ? 464 : 316);
        if(map) {
            clone_vector(result,source,map_records);
            clone_vector(result,source,280);
            clone_vector(result,source,296);
            // Source ACTs have no render caches. Runtime caches are rebuilt by
            // SetLayer and Update, never shared with another activation.
            std::memset(pointer(result+332),0,120);
            field<int32_t>(result,452)=-1;
            field<uint8_t>(result,460)=1;
        }
        return result;
    }
    Address key(Address source) {
        const auto result=copy(source,36);
        string(result,source,8);
        if(field<Address>(source,4)) field<Address>(result,4)=layout(field<Address>(source,4));
        return result;
    }
    void list(Address dest,Address source,size_t offset) {
        const auto head=allocate(sizeof(Node));
        field<Node>(head)={head,head,0};
        field<Address>(dest,offset)=head;
        field<uint32_t>(dest,offset+4)=0;
        const auto source_head=field<Address>(source,offset);
        if(!source_head) return;
        for(auto node=field<Node>(source_head).next;node!=source_head;node=field<Node>(node).next) {
            const auto value=field<Node>(node).value;
            const auto clone_node=allocate(sizeof(Node));
            auto &sentinel=field<Node>(head);
            field<Node>(clone_node)={head,sentinel.previous,value ? key(value) : 0};
            field<Node>(sentinel.previous).next=clone_node;
            sentinel.previous=clone_node;
            ++field<uint32_t>(dest,offset+4);
        }
    }
    Address layer(Address source) {
        const auto result=copy(source,348);
        clone_vector(result,source,layer_children);
        string(result,source,layer_name);
        list(result,source,layer_keys);
        list(result,source,layer_events);
        script(result+layer_script,source+layer_script);
        for(size_t offset: {308u,328u}) {
            field<Address>(result,offset+4)=0;
            field<int32_t>(result,offset+8)=g483;
            field<int32_t>(result,offset+12)=g484;
            field<uint8_t>(result,offset+16)=0;
        }
        return result;
    }
    Address resource(Address source) {
        const bool chip=field<Address>(source)==address(&g313);
        const auto result=copy(source,100);
        string(result,source,8);
        if(chip) {
            string(result,source,36);
            string(result,source,72);
            auto &control=field<Address>(source,68);
            if(!control) {
                // Original 42FA50 shares chip data. Use the actual Boost 1.44
                // counter and an MCD destructor, never the Actor slot deleter.
                auto* counted=up::create_callback_control(pointer(field<Address>(source,64)), dispose_chip_data);
                if(!counted) throw std::bad_alloc();
                control=address(counted);
            }
            chip_owners_.push_back(result);
            field<Address>(result,68)=control;
            up::add_strong(static_cast<up::CountedControl*>(pointer(control)));
        } else {
            string(result,source,40);
            const auto handle=field<int32_t>(source,68);
            if(handle) {
                textures_.push_back(handle);
                if(!kinoko_texture_retain(handle)) { textures_.pop_back(); throw std::bad_alloc(); }
            }
        }
        return result;
    }
    std::vector<void *> allocations_;
    std::vector<int32_t> textures_;
    std::vector<Address> chip_owners_;
    bool committed_=false;
};
}

// Original 42B580/42B5C0/42B3F0. Keep both constructor vtables; the sprite
// assignment copies offsets 8..235, and layout assignment copies 236..312.
// Padding 313..315 is not part of the source assignment.
extern "C" int32_t __fastcall kinoko_method_clone_c2d_layout(int32_t source, void*) {
    if (!source) return 0;
    auto* result=static_cast<unsigned char*>(std::calloc(1,316));
    if (!result) return 0;
    retdec_construct_c2dlayout(static_cast<int32_t>(address(result)));
    std::memcpy(result+8,static_cast<unsigned char*>(pointer(source))+8,305);
    return static_cast<int32_t>(address(result));
}

// Original 4265E0 copies the string and dispatches the layout's clone virtual.
// It does not copy trailing key padding or treat every layout as C2DLayout.
extern "C" int32_t __fastcall kinoko_method_clone_act_key(int32_t source, void*) {
    if (!source) return 0;
    auto release=[](unsigned char* key) {
        if (!key) return;
        const kinoko::legacy::StringView name(key+8);
        if (name.is_heap()) std::free(name.data());
        std::free(key);
    };
    std::unique_ptr<unsigned char,decltype(release)> result(
        static_cast<unsigned char*>(std::calloc(1,36)),release);
    if (!result) return 0;
    const auto key=address(result.get());
    field<Address>(key)=address(kinoko_act_host_symbols()->key_vtable);
    field<uint32_t>(key,28)=15;
    const kinoko::legacy::StringView input(static_cast<unsigned char*>(pointer(source))+8);
    const kinoko::legacy::StringView output(result.get()+8);
    output.assign(input.data(),input.length());
    if (output.length()!=input.length()) return 0;
    const auto layout=field<Address>(source,4);
    if (layout) field<int32_t>(key,4)=retdec_call_thiscall0_result(
        pointer(layout),field<void*>(field<Address>(layout),20));
    return static_cast<int32_t>(address(result.release()));
}

extern "C" int32_t __fastcall kinoko_act_clone(int32_t source,void *) {
    if(!source) return 0;
    try { return static_cast<int32_t>(Clone().act(static_cast<Address>(source))); }
    catch(...) { return 0; }
}

extern "C" int32_t kinoko_act_release_chip_data(int32_t resource) {
    auto &control=field<Address>(static_cast<Address>(resource),68);
    if(!control) return 1;
    const auto saved=control;
    control=0;
    up::release_strong(static_cast<up::CountedControl*>(pointer(saved)));
    // The upstream control invokes the MCD destructor on final release.
    return 0;
}
