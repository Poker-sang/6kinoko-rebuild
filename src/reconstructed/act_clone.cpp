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
#include <unordered_set>
#include <mutex>

namespace {
namespace up = kinoko::native::upstream;
using Address = uint32_t;
template<class T> T &field(Address address, size_t offset=0) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(address)+offset);
}
void *pointer(Address address) { return reinterpret_cast<void *>(static_cast<uintptr_t>(address)); }
Address address(const void *p) { return static_cast<Address>(reinterpret_cast<uintptr_t>(p)); }
void dispose_chip_data(void* data) { retdec_mcd_free(static_cast<retdec_mcd_data*>(data)); }

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

extern "C" int32_t kinoko_act_release_chip_data(int32_t resource) {
    auto &control=field<Address>(static_cast<Address>(resource),68);
    if(!control) return 1;
    const auto saved=control;
    control=0;
    up::release_strong(static_cast<up::CountedControl*>(pointer(saved)));
    // The upstream control invokes the MCD destructor on final release.
    return 0;
}
