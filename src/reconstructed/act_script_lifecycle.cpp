#include "kinoko/act_runtime.h"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_host.h"
#include "kinoko/squirrel_game_objects.h"
#include <squirrel.h>
#include <cstdlib>
#include <cstring>

using kinoko::legacy::pointer;
using kinoko::legacy::address;

void* kinoko_construct_cact_script(void* this_ptr)
{
    using namespace kinoko::act;
    if (!this_ptr) return 0;
    const ScriptStorageView script(this_ptr);
    script.set(&ScriptStorageRecord::methods, kinoko_act_host_symbols()->script_vtable);
    HSQOBJECT empty;
    sq_resetobject(&empty);
    // Callback VM slots and unknown bytes are deliberately untouched, as in
    // the preceding implementation. A standalone script is not a whole clear.
    for (const auto member : {&ScriptStorageRecord::initialize, &ScriptStorageRecord::update,
                              &ScriptStorageRecord::release}) {
        const auto callback = script.view(member);
        std::memcpy(callback.bytes(&ActCallbackRecord::environment), &empty, sizeof(empty));
        std::memcpy(callback.bytes(&ActCallbackRecord::closure), &empty, sizeof(empty));
    }
    const auto name = script.view(&ScriptStorageRecord::file_name);
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, kinoko::legacy::StringView::inline_capacity);
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    script.set(&ScriptStorageRecord::bytes, static_cast<void*>(nullptr));
    script.set(&ScriptStorageRecord::size, uint32_t{1});
    script.set(&ScriptStorageRecord::loaded, uint8_t{0});
    script.set(&ScriptStorageRecord::compiled, uint8_t{0});
    auto* buffer = pointer<unsigned char>(_3f__3f_2_40_YAPAXI_40_Z(1));
    script.set(&ScriptStorageRecord::bytes, static_cast<void*>(buffer));
    if (buffer) *buffer = 0;
    return this_ptr;
}

void kinoko_destroy_cact_script(int32_t script_ptr)
{
    using namespace kinoko::act;
    if (!script_ptr) return;
    kinoko_forget_act_script(script_ptr);
    const ScriptStorageView script(pointer<void>(script_ptr));
    script.set(&ScriptStorageRecord::methods, kinoko_act_host_symbols()->script_vtable);
    // Reverse callback release, then payload and filename: keep the order.
    for (const auto member : {&ScriptStorageRecord::release, &ScriptStorageRecord::update,
                              &ScriptStorageRecord::initialize})
        kinoko_release_act_callback((void*)(uintptr_t)(address(script.bytes(member))));
    std::free(script.get(&ScriptStorageRecord::bytes));
    script.set(&ScriptStorageRecord::bytes, static_cast<void*>(nullptr));
    script.set(&ScriptStorageRecord::size, uint32_t{0});
    kinoko_string_destroy(script.bytes(&ScriptStorageRecord::file_name));
    const auto name = script.view(&ScriptStorageRecord::file_name);
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, kinoko::legacy::StringView::inline_capacity);
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    script.set(&ScriptStorageRecord::loaded, uint8_t{0});
    script.set(&ScriptStorageRecord::compiled, uint8_t{0});
}
