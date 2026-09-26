#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/act_clone.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_layout_records.hpp"
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
using namespace kinoko::act;
int32_t legacy_address(const void* p) { return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p)); }
void dispose_chip_data(void* data) { kinoko_mcd_free(static_cast<kinoko_mcd_data*>(data)); }

}

// Original 42B580/42B5C0/42B3F0. Keep both constructor vtables; the sprite
// assignment copies offsets 8..235, and layout assignment copies 236..312.
// Padding 313..315 is not part of the source assignment.
extern "C" KinokoActLayout* __fastcall kinoko_method_clone_c2d_layout(KinokoActLayout* source, void*) {
    if (!source) return nullptr;
    auto* result=static_cast<unsigned char*>(std::calloc(1,sizeof(Layout2DRecord)));
    if (!result) return nullptr;
    (int32_t)(intptr_t)kinoko_construct_c2dlayout((KinokoActLayout*)(uintptr_t)(legacy_address(result)));
    constexpr auto begin=offsetof(Layout2DRecord,quad)+sizeof(void*);
    constexpr auto end=offsetof(Layout2DRecord,pivots_initialized)+sizeof(uint8_t);
    std::memcpy(result+begin,reinterpret_cast<const unsigned char*>(source)+begin,end-begin);
    return reinterpret_cast<KinokoActLayout*>(result);
}

// Original 4265E0 copies the string and dispatches the layout's clone virtual.
// Trailing key padding is not assigned.
extern "C" KinokoActKey* __fastcall kinoko_method_clone_act_key(KinokoActKey* source, void*) {
    if (!source) return nullptr;
    auto release=[](unsigned char* key) {
        if (!key) return;
        kinoko::legacy::StringView(KeyView(key).bytes(&KeyRecord::script_name)).destroy();
        std::free(key);
    };
    std::unique_ptr<unsigned char,decltype(release)> result(
        static_cast<unsigned char*>(std::calloc(1,sizeof(KeyRecord))),release);
    if (!result) return nullptr;
    const KeyView input(source), output(result.get());
    output.set(&KeyRecord::methods,kinoko_act_host_symbols()->key_vtable);
    output.view(&KeyRecord::script_name).set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
    const kinoko::legacy::StringView input_name(input.bytes(&KeyRecord::script_name));
    const kinoko::legacy::StringView output_name(output.bytes(&KeyRecord::script_name));
    output_name.assign(input_name.data(),input_name.length());
    if (output_name.length()!=input_name.length()) return nullptr;
    if (auto* layout=input.get(&KeyRecord::layout)) {
        const auto* methods=kinoko::legacy::load<const unsigned char*>(layout);
        using Clone=KinokoActLayout* (__thiscall*)(KinokoActLayout*);
        output.set(&KeyRecord::layout,kinoko::legacy::load<Clone>(methods+5*sizeof(void*))(layout));
    }
    return reinterpret_cast<KinokoActKey*>(result.release());
}

namespace {
std::mutex cloned_texture_mutex;
std::unordered_set<KinokoActResource*> cloned_texture_owners;
// 42F9D0/42FA50 and 446AF0/446B70/449B70 copy different resource fields.
// The MCD control is the actual Boost counter already used by archive clones.
KinokoActResource* clone_resource(KinokoActResource* source, const void* vtable, bool chip) {
    if (!source) return 0;
    auto destroy=[](unsigned char* value) {
        if (value) kinoko_destroy_cact_resource((KinokoActResource*)(uintptr_t)(legacy_address(value)));
    };
    std::unique_ptr<unsigned char,decltype(destroy)> owned(
        static_cast<unsigned char*>(std::calloc(1,100)),destroy);
    if (!owned) return 0;
    auto* result=reinterpret_cast<KinokoActResource*>(owned.get());
    const ChipResourceFields chip_input(source), chip_output(result);
    const TextureResourceFields texture_input(source), texture_output(result);
    chip_output.set(&ChipResourceRecord::methods,vtable);
    chip_output.set(&ChipResourceRecord::id,chip_input.get(&ChipResourceRecord::id));
    chip_output.view(&ChipResourceRecord::name).set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
    if (chip) {
        chip_output.view(&ChipResourceRecord::source_name).set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
        chip_output.view(&ChipResourceRecord::loaded_path).set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
    } else texture_output.view(&TextureResourceRecord::texture_name).set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
    const auto copy_string=[&](size_t offset) {
        const kinoko::legacy::StringView input(reinterpret_cast<unsigned char*>(source)+offset);
        const kinoko::legacy::StringView output(owned.get()+offset);
        output.assign(input.data(),input.length());
        if (input.length()!=output.length()) throw std::bad_alloc();
    };
    copy_string(offsetof(ChipResourceRecord,name));
    if (chip) {
        copy_string(offsetof(ChipResourceRecord,source_name)); copy_string(offsetof(ChipResourceRecord,loaded_path));
        const kinoko::act::ChipResourceFields input(source), output(owned.get());
        auto *control = input.get(&kinoko::act::ChipResourceRecord::shared_data);
        if (!control) {
            control = up::create_callback_control(input.get(&kinoko::act::ChipResourceRecord::data),dispose_chip_data);
            if (!control) return 0;
            input.set(&kinoko::act::ChipResourceRecord::shared_data, control);
        }
        up::add_strong(control);
        output.set(&kinoko::act::ChipResourceRecord::data, input.get(&kinoko::act::ChipResourceRecord::data));
        output.set(&kinoko::act::ChipResourceRecord::shared_data, control);
    } else {
        copy_string(offsetof(TextureResourceRecord,texture_name));
        texture_output.set(&TextureResourceRecord::borrows_texture,uint8_t{1}); // Both original clone methods mark borrowing.
        constexpr auto begin=offsetof(TextureResourceRecord,width);
        constexpr auto end=offsetof(TextureResourceRecord,auto_size)+sizeof(uint8_t);
        std::memcpy(owned.get()+begin,reinterpret_cast<unsigned char*>(source)+begin,end-begin);
        const auto handle=texture_input.get(&TextureResourceRecord::texture);
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
        texture_output.set(&TextureResourceRecord::texture,handle);
    }
    return reinterpret_cast<KinokoActResource*>(owned.release());
}
}
extern "C" int32_t kinoko_act_release_cloned_texture(KinokoActResource* resource) {
    {
        std::lock_guard<std::mutex> lock(cloned_texture_mutex);
        if (!cloned_texture_owners.erase(resource)) return 0;
    }
    kinoko_texture_release(TextureResourceFields(resource).get(&TextureResourceRecord::texture));
    return 1;
}
extern "C" KinokoActResource* __fastcall kinoko_method_clone_chip_resource(KinokoActResource* source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->chip_resource_vtable,true); }
    catch (...) { return 0; }
}
extern "C" KinokoActResource* __fastcall kinoko_method_clone_texture_resource(KinokoActResource* source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->texture_resource_vtable,false); }
    catch (...) { return 0; }
}
extern "C" KinokoActResource* __fastcall kinoko_method_clone_render_target(KinokoActResource* source, void*) {
    try { return clone_resource(source,kinoko_act_host_symbols()->render_target_vtable,false); }
    catch (...) { return 0; }
}

extern "C" int32_t kinoko_act_release_chip_data(KinokoActResource* resource) {
    const kinoko::act::ChipResourceFields fields(resource);
    auto *control = fields.get(&kinoko::act::ChipResourceRecord::shared_data);
    if (!control) return 1;
    fields.set(&kinoko::act::ChipResourceRecord::shared_data, static_cast<up::CountedControl *>(nullptr));
    up::release_strong(control);
    // The upstream control invokes the MCD destructor on final release.
    return 0;
}
