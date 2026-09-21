#include "kinoko/native_buffer.h"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/string_layout.h"
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
#include <unordered_set>
#include <mutex>
#include <vector>

extern "C" {
extern unsigned char g327, g313;
extern int32_t g483, g484;
int32_t retdec_act_bind_cloned_layouts(int32_t act);
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
        for(auto object:string_layouts_) kinoko_method_delete_string_layout(static_cast<int32_t>(object),nullptr,1);
        for(auto slot: strings_) kinoko::legacy::StringView(pointer(slot)).destroy();
        for(auto slot: buffers_) kinoko_native_buffer_destroy(slot);
        for(auto slot: arrays_) kinoko_act_array_destroy(slot);
        for(auto head: lists_) kinoko_act_list_drop_storage(static_cast<int32_t>(head));
        for(auto allocation: allocations_) std::free(allocation);
    }

    Address act(Address source) {
        const Address result=copy(source,240);
        string(result,source,16);
        string(result,source,44);
        script(result+act_script,source+act_script);
        field<uint8_t>(result,204)=0;
        const auto resources=clone_array(result,source,act_resources);
        const auto layers=clone_array(result,source,act_layers);
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
        if (!retdec_act_bind_cloned_layouts(static_cast<int32_t>(result))) throw std::bad_alloc();
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
        auto &output=field<String>(dest,offset);output={};output.capacity=15;
        const kinoko::legacy::StringView input(static_cast<unsigned char*>(pointer(source))+offset);
        const kinoko::legacy::StringView target(static_cast<unsigned char*>(pointer(dest))+offset);
        target.assign(input.data(),input.length());
        if(target.length()!=input.length()) {target.destroy();throw std::bad_alloc();}
        try {strings_.push_back(dest+offset);}
        catch(...) {target.destroy();throw;}
    }

    Vector clone_array(Address dest,Address source,size_t offset) {
        field<Vector>(dest,offset)={};
        kinoko_act_array_clone(dest+offset,source+offset);
        try { arrays_.push_back(dest+offset); }
        catch(...) { kinoko_act_array_destroy(dest+offset);throw; }
        return field<Vector>(dest,offset);
    }
    Vector clone_vector(Address dest,Address source,size_t offset) {
        const auto input=field<Vector>(source,offset);
        if(input.end<input.begin) throw std::bad_alloc();
        auto &out=field<Vector>(dest,offset);out={};
        kinoko_native_buffer_replace(dest+offset,pointer(input.begin),input.end-input.begin);
        try { buffers_.push_back(dest+offset); }
        catch(...) {kinoko_native_buffer_destroy(dest+offset);throw;}
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
        if(field<Address>(source)==address(g350)) {
            const auto result=static_cast<Address>(kinoko_method_clone_string_layout(static_cast<int32_t>(source),nullptr));
            if(!result) throw std::bad_alloc();
            try {string_layouts_.push_back(result);}
            catch(...) {kinoko_method_delete_string_layout(static_cast<int32_t>(result),nullptr,1);throw;}
            return result;
        }
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
        if (field<Address>(source)==address(kinoko_act_timeline_vtable())) {
            const auto result=copy(source,28);
            clone_vector(result,source,12);
            return result;
        }
        const auto result=copy(source,36);
        string(result,source,8);
        if(field<Address>(source,4)) field<Address>(result,4)=layout(field<Address>(source,4));
        return result;
    }
    void list(Address dest,Address source,size_t offset) {
        int32_t head=0;
        if(!retdec_act_make_list(&head)) throw std::bad_alloc();
        try { lists_.push_back(head); } catch(...) { kinoko_act_list_drop_storage(head);throw; }
        field<Address>(dest,offset)=head;field<uint32_t>(dest,offset+4)=0;
        const auto source_head=field<Address>(source,offset);
        if(!source_head) return;
        for(auto node=field<Node>(source_head).next;node!=source_head;node=field<Node>(node).next) {
            const auto value=field<Node>(node).value;
            if(!retdec_act_append_list(dest+offset,value?key(value):0)) throw std::bad_alloc();
            ++field<uint32_t>(dest,offset+4);
        }
    }
    Address layer(Address source) {
        const auto result=copy(source,348);
        clone_array(result,source,layer_children);
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
    std::vector<Address> strings_;
    std::vector<Address> buffers_;
    std::vector<Address> arrays_;
    std::vector<Address> lists_;
    std::vector<void *> allocations_;
    std::vector<int32_t> textures_;
    std::vector<Address> chip_owners_;
    std::vector<Address> string_layouts_;
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
        name.destroy();
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

namespace {
std::mutex cloned_texture_mutex;
std::unordered_set<Address> cloned_texture_owners;
// 42F9D0/42FA50 and 446AF0/446B70/449B70 copy different resource fields.
// The MCD control is the actual Boost counter already used by archive clones.
int32_t clone_resource(int32_t source, const void* vtable, bool chip) {
    if (!source) return 0;
    auto destroy=[](unsigned char* value) {
        if (value) retdec_destroy_cact_resource(static_cast<int32_t>(address(value)));
    };
    std::unique_ptr<unsigned char,decltype(destroy)> owned(
        static_cast<unsigned char*>(std::calloc(1,100)),destroy);
    if (!owned) return 0;
    const auto result=address(owned.get());
    field<Address>(result)=address(vtable);
    field<int32_t>(result,4)=field<int32_t>(source,4);
    field<uint32_t>(result,28)=15;
    if (chip) field<uint32_t>(result,56)=field<uint32_t>(result,92)=15;
    else field<uint32_t>(result,60)=15;
    const auto copy_string=[&](size_t offset) {
        const kinoko::legacy::StringView input(static_cast<unsigned char*>(pointer(source))+offset);
        const kinoko::legacy::StringView output(owned.get()+offset);
        output.assign(input.data(),input.length());
        if (input.length()!=output.length()) throw std::bad_alloc();
    };
    copy_string(8);
    if (chip) {
        copy_string(36); copy_string(72);
        auto& control=field<Address>(source,68);
        if (!control) {
            auto* counted=up::create_callback_control(pointer(field<Address>(source,64)),dispose_chip_data);
            if (!counted) return 0;
            control=address(counted);
        }
        up::add_strong(static_cast<up::CountedControl*>(pointer(control)));
        field<Address>(result,64)=field<Address>(source,64);
        field<Address>(result,68)=control;
    } else {
        copy_string(40);
        field<uint8_t>(result,36)=1; // Both original clone methods mark borrowing.
        std::memcpy(owned.get()+72,static_cast<unsigned char*>(pointer(source))+72,25);
        const auto handle=field<int32_t>(source,68);
        // The reconstructed texture store refcounts native handles. Retain
        // here so the source and clone can each execute their native cleanup.
        if (handle) {
            // Keep the original borrowed bit while recording the additional
            // native-store reference, so explicit Unload also releases it.
            {
                std::lock_guard<std::mutex> lock(cloned_texture_mutex);
                cloned_texture_owners.insert(result);
            }
            if (!kinoko_texture_retain(handle)) return 0;
        }
        field<int32_t>(result,68)=handle;
    }
    return static_cast<int32_t>(address(owned.release()));
}
}
extern "C" int32_t kinoko_act_release_cloned_texture(int32_t resource) {
    {
        std::lock_guard<std::mutex> lock(cloned_texture_mutex);
        if (!cloned_texture_owners.erase(static_cast<Address>(resource))) return 0;
    }
    kinoko_texture_release(field<int32_t>(resource,68));
    return 1;
}
extern "C" int32_t __fastcall kinoko_method_clone_chip_resource(int32_t source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->chip_resource_vtable,true); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_clone_texture_resource(int32_t source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->texture_resource_vtable,false); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_clone_render_target(int32_t source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->render_target_vtable,false); }
    catch (...) { return 0; }
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
