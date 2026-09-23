#include "kinoko/squirrel_api_types.h"
#include "kinoko/squirrel_vm_bootstrap.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include <squirrel.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <list>

extern "C" {
extern char g642;
extern int32_t g643;
extern char* g644;
extern int32_t g645;
extern int32_t unk_5149EC[3];
int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
}
namespace {
using kinoko::script::address;
using kinoko::script::pointer;
// Borrowed ABI slots published to legacy SqPlus callers; the list below owns
// shared states and the cached root wrapper owns its external reference.
struct VmSlots {
    char& skip_owner_reset() const { return g642; }
    int32_t& newest_shared_state() const { return g643; }
    int32_t& cached_root() const { return g645; }
    int32_t* thread_wrapper() const { return unk_5149EC; }
    SQVM* current() const { return reinterpret_cast<SQVM*>(g644); }
    void publish(SQVM* vm) const { g644 = reinterpret_cast<char*>(vm); }
} slots;
HSQUIRRELVM current_vm() noexcept { return slots.current(); }
// Actual standard-list values own the deferred VM state identities. g643
// publishes only the newest identity token; no legacy next-pointer node exists.
std::list<int32_t>& owned_states() {
    static std::list<int32_t> values;
    return values;
}

}
extern "C" void kinoko_sq_release_owned_states(void) {
    // Original CRT exit handler 4D4B30 and node destructor 4A8D60.
    // Ordinary 4A8C50 only releases wrappers; it must not destroy these VMs.
    std::list<int32_t> pending;
    pending.splice(pending.end(),owned_states());
    slots.newest_shared_state()=0;
    for(const auto state:pending) kinoko_sq_delete_shared_state(state);
}
extern "C" int32_t kinoko_sqplus_release_vm_wrappers(void) {
    if (slots.cached_root() != 0) {
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy(pointer<void>(slots.cached_root())));
        std::free(pointer<void>(slots.cached_root()));
        slots.cached_root() = 0;
    }
    if (slots.skip_owner_reset() == 0) (int32_t)(intptr_t)(kinoko_sqplus_object_reset(slots.thread_wrapper()));
    slots.publish(nullptr);
    return 0;
}
extern "C" int32_t kinoko_sqplus_print(struct SQVM * , const char* format, ...) {
    char message[4096]{};
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format ? format : "", arguments);
    va_end(arguments);
    return std::puts(message);
}
extern "C" void * kinoko_sqplus_root_object(void) {
    if (slots.cached_root() != 0) return pointer<void>(slots.cached_root());
    auto* vm = current_vm();
    if (!vm) return 0;
    sq_pushroottable(vm);
    auto* storage = pointer<void>(_3f__3f_2_40_YAPAXI_40_Z(sizeof(kinoko::script::ObjectStorage)));
    if (storage != 0) (int32_t)(intptr_t)(kinoko_sqplus_object_initialize(storage));
    slots.cached_root() = address(storage);
    kinoko_sqplus_object_capture(storage, -1);
    sq_pop(vm, 1);
    return pointer<void>(slots.cached_root());
}
extern "C" int32_t kinoko_sqplus_select_vm(struct SQVM * requested_vm) {
    auto* current = requested_vm;
    if (requested_vm && current_vm() == requested_vm) return 1;
    // 4A8DD7 destroys the cached root with deleting-destructor flag 1,
    // while the outgoing VM is still current. This cache is allocated by
    // kinoko_sqplus_root_object; release its external root before freeing it.
    if (slots.cached_root() != 0) {
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy(pointer<void>(slots.cached_root())));
        std::free(pointer<void>(slots.cached_root()));
        slots.cached_root() = 0;
    }
    if (slots.skip_owner_reset() == 0) (int32_t)(intptr_t)(kinoko_sqplus_object_reset(slots.thread_wrapper()));
    slots.publish(nullptr);
    if (!requested_vm) {
        current = kinoko_script_open_primary_vm(1024);
        if (current == 0) return 0;
        try {
            auto& values=owned_states();
            values.push_front(kinoko_sq_shared_state(address(current)));
            static const int registered=std::atexit(kinoko_sq_release_owned_states);
            (void)registered;
            g643=address(&values.front());
        } catch(...) {
            kinoko_sq_delete_shared_state(kinoko_sq_shared_state(address(current)));
            return 0;
        }
        // The legacy printer returns puts' status; Squirrel's void callback ignores it.
        sq_setprintfunc(current, reinterpret_cast<SQPRINTFUNCTION>(&kinoko_sqplus_print));
        sq_pushroottable(current);
        sqstd_register_iolib(current);
        sqstd_register_bloblib(current);
        sqstd_register_mathlib(current);
        sqstd_register_stringlib(current);
        sqstd_seterrorhandlers(current);
        kinoko_sq_pop(address(current), 1);
    }
    slots.skip_owner_reset() = 0;
    slots.publish(current);
    const int32_t owner_result = (int32_t)(intptr_t)(kinoko_sqplus_object_assign_thread(slots.thread_wrapper(), current));
    return (owner_result & -256) | 1;
}
