#include "kinoko/legacy_string.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_property_bridge.h"
#include "kinoko/native_property_callbacks.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <algorithm>
#include <atomic>

extern "C" {
void kinoko_trace_i32(const char*, int32_t);
}

namespace {
using kinoko::script::pointer;
using kinoko::script::address;
namespace upstream = kinoko::script::upstream;
template<class T> T read(const void* storage) {
    T value; std::memcpy(&value, storage, sizeof(value)); return value;
}
template<class T> void write(void* storage, const T& value) {
    std::memcpy(storage, &value, sizeof(value));
}
unsigned char* field(int32_t object, int32_t offset) {
    // Native layout addresses and offsets have the original 32-bit wrap rules.
    return pointer<unsigned char>(static_cast<int32_t>(static_cast<uint32_t>(object) +
                                                       static_cast<uint32_t>(offset)));
}
int32_t descriptor(int32_t id, int32_t* offset, bool trace) {
    if (!id || !offset) return 0;
    auto vm = pointer<SQVM>(id);
    // 2.2.2 does not validate stack indices. A native closure appends its
    // captured userdata after the arguments: [instance, (value), descriptor].
    if (sq_gettop(vm) < 2) return 0;
    SQUserPointer target = nullptr, payload = nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm, 1, &target, nullptr)) || !target ||
        SQ_FAILED(sq_getuserdata(vm, -1, &payload, nullptr)) || !payload) return 0;
    if (sq_getsize(vm, -1) < static_cast<SQInteger>(sizeof(int32_t))) return 0;
    const auto value = read<int32_t>(payload);
    write(offset, value);
    if (trace && (value == 0x8c || (value >= 0x34 && value <= 0x44))) {
        static std::atomic<unsigned> count{0};
        if (count.fetch_add(1, std::memory_order_relaxed) < 256) {
            kinoko_trace_i32("cact:property-target", address(target));
            kinoko_trace_i32("cact:property-offset", value);
            kinoko_trace_i32("cact:property-vtable", read<int32_t>(target));
        }
    }
    return address(target);
}
class NativeField final {
public:
    NativeField(int32_t id, bool trace, bool indirect = false, bool setter = false)
        : vm_(pointer<SQVM>(id)), target_(0), offset_(0), storage_(nullptr) {
        if (!vm_ || (setter && sq_gettop(vm_) < 3)) return;
        target_ = descriptor(id, &offset_, trace);
        if (target_) storage_ = field(target_, offset_);
        if (storage_ && indirect) storage_ = read<unsigned char*>(storage_);
    }
    HSQUIRRELVM vm() const { return vm_; }
    void* storage() const { return storage_; }
    int32_t offset() const { return offset_; }
    int32_t target() const { return target_; }
    template<class T> T value() const { return read<T>(storage_); }
    template<class T> void value(T value) { write(storage_, value); }
private:
    HSQUIRRELVM vm_;
    int32_t target_, offset_;
    unsigned char* storage_;
};
template<class T> bool argument(HSQUIRRELVM vm, T& value);
template<> bool argument<SQInteger>(HSQUIRRELVM vm, SQInteger& value) { return upstream::sqrat_integer_argument(vm, 2, value); }
template<> bool argument<SQFloat>(HSQUIRRELVM vm, SQFloat& value) { return upstream::sqrat_float_argument(vm, 2, value); }
void push(HSQUIRRELVM vm, SQInteger value) { upstream::sqrat_push_integer(vm, value); }
void push(HSQUIRRELVM vm, SQFloat value) { upstream::sqrat_push_float(vm, value); }
template<class T> int32_t get_number(int32_t id, bool trace, bool indirect = false) {
    NativeField field(id, trace, indirect);
    if (!field.storage()) return 0;
    push(field.vm(), field.value<T>()); return 1;
}
template<class T> int32_t set_number(int32_t id, bool trace, bool indirect = false) {
    NativeField field(id, trace, indirect, true);
    T value = 0;
    if (!field.storage() || !argument(field.vm(), value)) return 0;
    field.value(value);
    return 0;
}
int32_t get_bool(int32_t id, bool trace) {
    NativeField field(id, trace);
    if (!field.storage()) return 0;
    upstream::sqrat_push_bool(field.vm(), field.value<uint8_t>() != 0); return 1;
}
int32_t set_bool(int32_t id, bool trace) {
    NativeField field(id, trace, false, true);
    if (!field.storage()) return 0;
    const bool value = upstream::sqrat_bool_argument(field.vm(), 2);
    field.value<uint8_t>(value != 0);
    if (trace && (field.offset() == 0x8c || field.offset() == 0x8d)) {
        static std::atomic<unsigned> count{0};
        if (count.fetch_add(1, std::memory_order_relaxed) < 128) {
            kinoko_trace_i32("cact:set-bool-target", field.target());
            kinoko_trace_i32("cact:set-bool-offset", field.offset());
            kinoko_trace_i32("cact:set-bool-value", value != 0);
        }
    }
    return 0;
}
void assign_string(void* field, const char* value) {
    kinoko_string_assign_cstr(static_cast<int32_t*>(field), value);
}
}

extern "C" int32_t kinoko_cact_layer_property_offset(int32_t id, int32_t* offset) { return descriptor(id, offset, true); }
extern "C" int32_t kinoko_c2dlayout_property_offset(int32_t id, int32_t* offset) { return descriptor(id, offset, false); }
extern "C" int32_t kinoko_cact_layer_get_int(int32_t id) { return get_number<SQInteger>(id, true); }
extern "C" int32_t kinoko_cact_layer_set_int(int32_t id) { return set_number<SQInteger>(id, true); }
extern "C" int32_t kinoko_cact_layer_get_float(int32_t id) { return get_number<SQFloat>(id, true); }
extern "C" int32_t kinoko_cact_layer_set_float(int32_t id) { return set_number<SQFloat>(id, true); }
extern "C" int32_t kinoko_cact_layer_get_pointer_int(int32_t id) { return get_number<SQInteger>(id, true, true); }
extern "C" int32_t kinoko_cact_layer_set_pointer_int(int32_t id) { return set_number<SQInteger>(id, true, true); }
extern "C" int32_t kinoko_cact_layer_get_pointer_float(int32_t id) { return get_number<SQFloat>(id, true, true); }
extern "C" int32_t kinoko_cact_layer_set_pointer_float(int32_t id) { return set_number<SQFloat>(id, true, true); }
extern "C" int32_t kinoko_c2dlayout_get_int(int32_t id) { return get_number<SQInteger>(id, false); }
extern "C" int32_t kinoko_c2dlayout_set_int(int32_t id) { return set_number<SQInteger>(id, false); }
extern "C" int32_t kinoko_c2dlayout_get_float(int32_t id) { return get_number<SQFloat>(id, false); }
extern "C" int32_t kinoko_c2dlayout_set_float(int32_t id) { return set_number<SQFloat>(id, false); }
extern "C" int32_t kinoko_cact_layer_get_bool(int32_t id) { return get_bool(id, true); }
extern "C" int32_t kinoko_cact_layer_set_bool(int32_t id) { return set_bool(id, true); }
extern "C" int32_t kinoko_cact_layer_get_string(int32_t id) {
    NativeField field(id, true);
    if (!field.storage()) return 0;
    const auto value = kinoko_string_data(field.storage());
    sq_pushstring(field.vm(), value ? value : "", -1);
    // Preserve the legacy signed stack-slot-address comparison, not an assumed
    // SQRESULT (sq_pushstring is void). Its unusual return ABI is not changed.
    return ((int32_t)(uintptr_t)kinoko_sq_get_up((SQVM*)(uintptr_t)(id), -1)) >= 0;
}
extern "C" int32_t kinoko_cact_layer_set_string(int32_t id) {
    NativeField field(id, true, false, true);
    const SQChar* value = nullptr;
    if (field.storage() && SQ_SUCCEEDED(sq_getstring(field.vm(), 2, &value))) assign_string(field.storage(), value);
    return 0;
}
extern "C" int32_t kinoko_c2dlayout_set_color(int32_t id) {
    NativeField field(id, false, false, true);
    SQInteger value = 0;
    if (field.storage() && argument(field.vm(), value)) field.value(std::clamp(value, 0, 255));
    return 0;
}
extern "C" int32_t kinoko_acting_player_property(int32_t id, int32_t* offset) {
    const auto target = descriptor(id, offset, false);
    if (!target) return 0;
    return *offset == 8 ? address(field(target, 8)) : read<int32_t>(field(target, *offset));
}
extern "C" int32_t kinoko_acting_player_get_property(int32_t id) {
    int32_t offset = 0;
    auto storage = pointer(kinoko_acting_player_property(id, &offset));
    if (!storage) return 0;
    auto vm = pointer<SQVM>(id);
    if (offset == 8 || offset == 132) sq_pushbool(vm, read<uint8_t>(storage) != 0);
    else if (offset == 124 || offset == 128) sq_pushfloat(vm, read<SQFloat>(storage));
    else if (offset == 148) sq_pushstring(vm, kinoko_string_data((const void*)(storage)), -1);
    else sq_pushinteger(vm, read<SQInteger>(storage));
    return 1;
}
extern "C" int32_t kinoko_acting_player_set_property(int32_t id) {
    if (!id || sq_gettop(pointer<SQVM>(id)) < 3) return 0;
    int32_t offset = 0;
    auto storage = pointer(kinoko_acting_player_property(id, &offset));
    if (!storage) return 0;
    auto vm = pointer<SQVM>(id);
    if (offset == 8 || offset == 132) {
        SQBool value; sq_tobool(vm, 2, &value); write<uint8_t>(storage, value != 0);
    } else if (offset == 148) {
        const SQChar* value = nullptr;
        if (SQ_SUCCEEDED(sq_getstring(vm, 2, &value))) assign_string(storage, value);
    } else if (offset == 124 || offset == 128) {
        SQFloat value = 0; if (argument(vm, value)) write(storage, value);
    } else {
        SQInteger value = 0; if (argument(vm, value)) write(storage, value);
    }
    return 0;
}
extern "C" int32_t kinoko_native_view_get_short(int32_t id) {
    NativeField field(id, false);
    if (!field.storage()) return 0;
    sq_pushinteger(field.vm(), field.value<int16_t>()); return 1;
}
extern "C" int32_t kinoko_native_view_set_short(int32_t id) {
    NativeField field(id, false, false, true);
    SQInteger value = 0;
    if (field.storage() && argument(field.vm(), value)) field.value(static_cast<uint16_t>(value));
    return 0;
}

// These entry points are assigned directly to SQFUNCTION. The older int32_t
// host APIs above remain for generated C callers; there is no address lookup
// table and no dependency on the original EXE image being mapped.
extern "C" SQInteger kinoko_sqrat_get_int(HSQUIRRELVM vm) { return get_number<SQInteger>(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_set_int(HSQUIRRELVM vm) { return set_number<SQInteger>(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_get_float(HSQUIRRELVM vm) { return get_number<SQFloat>(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_set_float(HSQUIRRELVM vm) { return set_number<SQFloat>(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_get_bool(HSQUIRRELVM vm) { return get_bool(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_set_bool(HSQUIRRELVM vm) { return set_bool(address(vm), false); }
extern "C" SQInteger kinoko_sqrat_get_short(HSQUIRRELVM vm) { return kinoko_native_view_get_short(address(vm)); }
extern "C" SQInteger kinoko_sqrat_set_short(HSQUIRRELVM vm) { return kinoko_native_view_set_short(address(vm)); }
extern "C" SQInteger kinoko_sqrat_get_pointer_int(HSQUIRRELVM vm) { return get_number<SQInteger>(address(vm), false, true); }
extern "C" SQInteger kinoko_sqrat_set_pointer_int(HSQUIRRELVM vm) { return set_number<SQInteger>(address(vm), false, true); }
extern "C" SQInteger kinoko_sqrat_get_pointer_float(HSQUIRRELVM vm) { return get_number<SQFloat>(address(vm), false, true); }
extern "C" SQInteger kinoko_sqrat_set_pointer_float(HSQUIRRELVM vm) { return set_number<SQFloat>(address(vm), false, true); }
extern "C" SQInteger kinoko_sqrat_noop(HSQUIRRELVM) { return 0; }

// Original 44BA90/44BB10 use string value conversion, not the strict Layer
// setter above. 41E0C0 calls sq_tostring(index 2) then gets the pushed string
// and discards it. In 2.2.2 ToString also defines metamethod-failure fallback.
extern "C" SQInteger kinoko_sqrat_get_string(HSQUIRRELVM vm) {
    NativeField field(address(vm), false);
    if (!field.storage()) return 0;
    const kinoko::legacy::StringView value(field.storage());
    sq_pushstring(vm, value.data(), -1); // Preserve first-NUL truncation.
    return 1;
}
extern "C" SQInteger kinoko_sqrat_set_string(HSQUIRRELVM vm) {
    NativeField field(address(vm), false, false, true);
    if (!field.storage()) return 0;
    const auto top = sq_gettop(vm);
    sq_tostring(vm, 2); // void in 2.2.2; always pushes its conversion result.
    const SQChar* converted = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &converted))) {
        // Unlike unresolved missing-length C callers, this is a live SQString
        // with a guaranteed terminator. Do not apply the 1 MiB address scanner.
        kinoko::legacy::StringView(field.storage()).assign(converted,
            static_cast<std::uint32_t>(std::strlen(converted)));
    }
    sq_settop(vm, top); // Pop only the conversion, retaining its last-error.
    return 0;
}
