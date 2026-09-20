#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "squserdata.h"
#include <atomic>
#include <cstdlib>

extern "C" {
extern char g560;
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
}

namespace {
using kinoko::script::address;
using kinoko::script::pointer;
using kinoko::script::data_bits;

// Sqrat != SqPlus: the former stores its VM before the externally-owned pair.
// Do not overlay an SQObjectPtr on either layout: its ownership is internal.
struct SqratStorage {
    uint32_t vtable;
    HSQUIRRELVM vm;
    HSQOBJECT value;
    uint8_t owns;
};
static_assert(sizeof(SqratStorage) == 20);
static_assert(offsetof(SqratStorage, vm) == 4);
static_assert(offsetof(SqratStorage, value) == 8);
static_assert(offsetof(SqratStorage, owns) == 16);
struct CallbackStorage { HSQUIRRELVM vm; HSQOBJECT environment, closure; };
static_assert(sizeof(CallbackStorage) == 20);

template<class T> T read(const void* bytes) {
    T value;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}
template<class T> void write(void* bytes, const T& value) {
    std::memcpy(bytes, &value, sizeof(value));
}
class ObjectView final {
public:
    explicit ObjectView(int32_t storage) : record_(pointer(storage)) {}
    HSQUIRRELVM vm() const { return record_.get(&SqratStorage::vm); }
    HSQOBJECT value() const { return record_.get(&SqratStorage::value); }
    void value(const HSQOBJECT& value) { record_.set(&SqratStorage::value, value); }
    void reset() { HSQOBJECT empty; sq_resetobject(&empty); value(empty); }
    void vm(HSQUIRRELVM vm) { record_.set(&SqratStorage::vm, vm); }
    bool owns() const { return record_.get(&SqratStorage::owns) != 0; }
    void owns(bool flag) { record_.set(&SqratStorage::owns, uint8_t(flag ? 1 : 0)); }
    void vtable(int32_t value) { record_.set(&SqratStorage::vtable, uint32_t(value)); }
    int32_t payload_address() const {
        return address(record_.bytes(&SqratStorage::value));
    }
private:
    kinoko::native::RecordView<SqratStorage> record_;
};
// Unlike sq_settop, the original trim helper NEVER grows a depleted stack.
class TrimStack final {
public:
    explicit TrimStack(HSQUIRRELVM vm) : vm_(vm), top_(sq_gettop(vm)) {}
    ~TrimStack() { retdec_sqrat_trim_stack(address(vm_), top_); }
    TrimStack(const TrimStack&) = delete;
    TrimStack& operator=(const TrimStack&) = delete;
private:
    HSQUIRRELVM vm_;
    SQInteger top_;
};
void reset_pair(void* storage) {
    HSQOBJECT empty; sq_resetobject(&empty); write(storage, empty);
}
void trace_pair(const char* type_label, const char* data_label, const HSQOBJECT& value) {
    retdec_trace_i32(type_label, value._type);
    retdec_trace_i32(data_label, data_bits(value));
}
int32_t set_pair(int32_t id, const int32_t* object, const char* name,
                 const int32_t* value, bool raw) {
    if (!id || !object || !name || !value) return 0;
    auto vm = pointer<SQVM>(id);
    // Snapshot before pushing: the caller's borrowed pair may be stack-backed.
    const auto receiver = read<HSQOBJECT>(object), incoming = read<HSQOBJECT>(value);
    TrimStack stack(vm);
    if (!raw) {
        static std::atomic<unsigned> traces{0};
        if (traces.fetch_add(1, std::memory_order_relaxed) < 96) {
            retdec_trace_squirrel_name("sqrat:set-name", address(name));
            trace_pair("sqrat:set-object-type", "sqrat:set-object-data", receiver);
            trace_pair("sqrat:set-value-type", "sqrat:set-value-data", incoming);
        }
    }
    // Logging must not do a second scripted get (especially _get on instances).
    return kinoko::script::upstream::sqrat_bind_value(vm, receiver, name, incoming, raw);
}
int32_t set_string(int32_t id, const int32_t* object, const char* name,
                   const char* value, bool raw) {
    if (!id || !object || !name) return 0;
    auto vm = pointer<SQVM>(id);
    const auto receiver = read<HSQOBJECT>(object);
    TrimStack stack(vm);
    return kinoko::script::upstream::sqrat_bind_string(vm, receiver, name, value ? value : "", raw);
}
int32_t set_value(int32_t vm, const int32_t* object, const char* name,
                  const HSQOBJECT& value, bool raw) {
    int32_t pair[2]; write(pair, value);
    return set_pair(vm, object, name, pair, raw);
}
HSQOBJECT integer(SQInteger value) { HSQOBJECT o; o._type = OT_INTEGER; o._unVal.nInteger = value; return o; }
HSQOBJECT boolean(int32_t value) { HSQOBJECT o; o._type = OT_BOOL; o._unVal.nInteger = value != 0; return o; }
}

extern "C" void retdec_sqrat_trim_stack(int32_t id, int32_t base) {
    if (!id) return;
    auto vm = pointer<SQVM>(id);
    const SQInteger top = sq_gettop(vm);
    if (top > base) kinoko_sq_pop(id, top - base);
}
extern "C" int32_t retdec_sqrat_root_construct(int32_t storage, int32_t id) {
    if (!storage || !id) return 0;
    auto vm = pointer<SQVM>(id);
    ObjectView object(storage);
    retdec_trace_i32("450e30:construct-object", storage);
    retdec_trace_i32("450e30:construct-pair", object.payload_address());
    object.vtable(kinoko_sqrat_object_vtable());
    object.vm(vm); object.owns(true); object.reset();
    object.vtable(kinoko_sqrat_root_vtable());
    const auto root = kinoko::script::upstream::sqrat_root(vm);
    object.value(root);
    trace_pair("450e30:after-pop-type", "450e30:after-pop-data", root);
    return storage;
}
extern "C" void retdec_sqrat_object_release(int32_t storage) {
    if (!storage) return;
    ObjectView object(storage);
    if (object.owns() && object.vm())
        kinoko::script::upstream::sqrat_release(object.vm(), object.value());
    object.owns(false); object.reset(); object.vtable(kinoko_sqrat_object_vtable());
}
extern "C" int32_t retdec_sqrat_get(int32_t storage, const char* name, int32_t out) {
    if (!storage || !name || !out) return 0;
    ObjectView object(storage);
    const auto vm = object.vm();
    if (!vm) return 0;
    reset_pair(pointer(out));
    TrimStack stack(vm);
    HSQOBJECT value;
    const bool found = kinoko::script::upstream::sqrat_get(vm, object.value(), name, value);
    write(pointer(out), value);
    return found;
}
extern "C" void retdec_sqrat_release_pair(int32_t id, int32_t* pair) {
    if (!pair) return;
    auto value = read<HSQOBJECT>(pair);
    if (value._type != OT_NULL || data_bits(value) != 0) kinoko::script::upstream::sqrat_release(pointer<SQVM>(id), value);
    reset_pair(pair);
}
extern "C" int32_t retdec_sqrat_set_pair(int32_t vm, const int32_t* object, const char* name, const int32_t* value) { return set_pair(vm, object, name, value, false); }
extern "C" int32_t retdec_sqrat_raw_set_pair(int32_t vm, const int32_t* object, const char* name, const int32_t* value) { return set_pair(vm, object, name, value, true); }
extern "C" int32_t retdec_sqrat_set_int(int32_t vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, integer(value), false); }
extern "C" int32_t retdec_sqrat_set_bool(int32_t vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, boolean(value), false); }
extern "C" int32_t retdec_sqrat_raw_set_int(int32_t vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, integer(value), true); }
extern "C" int32_t retdec_sqrat_raw_set_bool(int32_t vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, boolean(value), true); }
extern "C" int32_t retdec_sqrat_raw_set_float(int32_t vm, const int32_t* object, const char* name, float value) {
    HSQOBJECT o; o._type = OT_FLOAT; o._unVal.fFloat = value;
    return set_value(vm, object, name, o, true);
}
extern "C" int32_t retdec_sqrat_set_string(int32_t vm, const int32_t* object, const char* name, const char* value) { return set_string(vm, object, name, value, false); }
extern "C" int32_t retdec_sqrat_raw_set_string(int32_t vm, const int32_t* object, const char* name, const char* value) { return set_string(vm, object, name, value, true); }
extern "C" int32_t retdec_sqrat_set_native_closure(int32_t id, const int32_t* object,
    const char* name, int32_t function, const int32_t* free_pair, int32_t free_count) {
    // The recovered interface accepts ONE optional pair, not an array of pairs.
    // Larger counts used to consume the name/receiver and underflow the stack.
    if (!id || !object || !name || !function || free_count < 0 || free_count > 1 ||
        (free_count && !free_pair)) return 0;
    auto vm = pointer<SQVM>(id);
    const auto receiver = read<HSQOBJECT>(object);
    HSQOBJECT capture; sq_resetobject(&capture);
    if (free_count) capture = read<HSQOBJECT>(free_pair);
    TrimStack stack(vm);
    sq_pushobject(vm, receiver); sq_pushstring(vm, name, -1);
    if (free_count) sq_pushobject(vm, capture);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(pointer(function)), free_count);
    return SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse));
}
extern "C" int32_t retdec_sqrat_set_offset_closure(int32_t id, const int32_t* table,
    const char* name, int32_t offset, int32_t function) {
    if (!id || !table || !name || !function) return 0;
    auto vm = pointer<SQVM>(id);
    const auto receiver = read<HSQOBJECT>(table);
    TrimStack stack(vm);
    return kinoko::script::upstream::sqrat_bind_function(vm, receiver, name,
        &offset, sizeof(offset), reinterpret_cast<SQFUNCTION>(pointer(function)), false);
}
extern "C" int32_t retdec_sqrat_new_class(int32_t id, int32_t* output) {
    if (!id || !output) return 0;
    const auto value = kinoko::script::upstream::sqrat_new_class(pointer<SQVM>(id), true);
    write(output, value);
    return value._type == OT_CLASS;
}
extern "C" int32_t retdec_sqrat_new_table(int32_t id, int32_t* out) {
    if (!id || !out) return 0;
    auto vm = pointer<SQVM>(id);
    reset_pair(out);
    TrimStack stack(vm);
    const auto value = kinoko::script::upstream::sqrat_table(vm);
    write(out, value);
    static std::atomic<unsigned> traces{0};
    if (traces.fetch_add(1, std::memory_order_relaxed) < 32)
        trace_pair("sqrat:new-table-type", "sqrat:new-table-data", value);
    return value._type == OT_TABLE && _table(value) != nullptr;
}
extern "C" int32_t retdec_sqrat_set_delegate(int32_t id, const int32_t* object, const int32_t* delegate) {
    if (!id || !object || !delegate) return 0;
    const auto receiver = read<HSQOBJECT>(object), incoming = read<HSQOBJECT>(delegate);
    auto vm = pointer<SQVM>(id);
    TrimStack stack(vm);
    sq_pushobject(vm, receiver); sq_pushobject(vm, incoming);
    // sq_setdelegate would change _lasterror on cycles and invalid types. This
    // recovered path uses SetDelegate's boolean contract for both host types.
    if (incoming._type != OT_NULL && incoming._type != OT_TABLE) return 0;
    SQDelegable* target = nullptr;
    if (receiver._type == OT_TABLE) target = _table(receiver);
    else if (receiver._type == OT_USERDATA) target = _userdata(receiver);
    if (!target) return 0;
    return target->SetDelegate(incoming._type == OT_NULL ? nullptr : _table(incoming));
}
extern "C" int32_t function_415550_this(int32_t storage, int32_t name, int32_t source,
    int32_t size, int32_t function, int32_t static_slot) {
    if (!storage) return 0;
    ObjectView object(storage);
    auto vm = object.vm();
    if (!vm || !function || size < 0 || (size && !source)) return 0;
    retdec_trace_squirrel_name("415550:name", name);
    retdec_trace_i32("415550:size", size);
    retdec_trace_i32("415550:native", function);
    // Execute Sqrat's actual BindFunc body, including userdata copy, closure,
    // publication and pop. The legacy entry returns VM, not BindFunc's void.
    kinoko::script::upstream::sqrat_bind_function(vm, object.value(),
        pointer<const char>(name), pointer<const void>(source), static_cast<size_t>(size),
        reinterpret_cast<SQFUNCTION>(pointer(function)), (static_slot & 255) != 0);
    trace_pair("415550:after-pop-type", "415550:after-pop-data", object.value());
    return address(vm);
}
extern "C" int32_t __fastcall kinoko_sqrat_copy_object(int32_t receiver, void*, int32_t output) {
    const ObjectView object(receiver);
    write(pointer(output), kinoko::script::upstream::sqrat_object_value(object.vm(), object.value()));
    return output;
}
extern "C" int32_t __fastcall kinoko_sqrat_object_reference(int32_t receiver, void*) {
    // This slot returns a reference into the host record, not a temporary
    // source Object. Only the legacy storage address crosses this ABI bridge.
    return ObjectView(receiver).payload_address();
}
extern "C" int32_t __fastcall kinoko_sqrat_delete_object(int32_t receiver, void*, int32_t flags) {
    ObjectView object(receiver);
    object.vtable(kinoko_sqrat_object_vtable()); // visible during a release hook
    kinoko::script::upstream::sqrat_destroy_object(object.vm(), object.value(), object.owns());
    if (flags & 1) std::free(pointer(receiver));
    return receiver;
}
extern "C" int32_t function_415810_this(int32_t storage) {
    if (!storage) return -1;
    const auto callback = read<CallbackStorage>(pointer(storage));
    if (!callback.vm) return -1;
    static std::atomic<unsigned> traces{0};
    if (traces.fetch_add(1, std::memory_order_relaxed) < 96) {
        retdec_trace_i32("415810:self", storage);
        trace_pair("415810:env-type", "415810:env-data", callback.environment);
        trace_pair("415810:closure-type", "415810:closure-data", callback.closure);
    }
    kinoko::script::upstream::sqrat_execute(callback.vm, callback.environment,
        callback.closure, g560 != 0,
        [](HSQUIRRELVM vm, SQInteger count, SQBool result, SQBool errors) -> SQRESULT {
            return kinoko_sq_call(address(vm), count, result, errors);
        });
    return address(callback.vm);
}
