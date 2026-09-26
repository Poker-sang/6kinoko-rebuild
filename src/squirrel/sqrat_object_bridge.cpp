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
extern char kinoko_sqrat_trace_enabled;
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
}

namespace {
inline char& native_trace_slot = kinoko_sqrat_trace_enabled;
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
    explicit ObjectView(void* storage) : record_(storage) {}
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
    ~TrimStack() { kinoko_sqrat_trim_stack(vm_, top_); }
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
int32_t set_pair(SQVM* id, const int32_t* object, const char* name,
                 const int32_t* value, bool raw) {
    if (!id || !object || !name || !value) return 0;
    auto vm = id;
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
int32_t set_string(SQVM* id, const int32_t* object, const char* name,
                   const char* value, bool raw) {
    if (!id || !object || !name) return 0;
    auto vm = id;
    const auto receiver = read<HSQOBJECT>(object);
    TrimStack stack(vm);
    return kinoko::script::upstream::sqrat_bind_string(vm, receiver, name, value ? value : "", raw);
}
int32_t set_value(SQVM* vm, const int32_t* object, const char* name,
                  const HSQOBJECT& value, bool raw) {
    int32_t pair[2]; write(pair, value);
    return set_pair(vm, object, name, pair, raw);
}
HSQOBJECT integer(SQInteger value) { HSQOBJECT o; o._type = OT_INTEGER; o._unVal.nInteger = value; return o; }
HSQOBJECT boolean(int32_t value) { HSQOBJECT o; o._type = OT_BOOL; o._unVal.nInteger = value != 0; return o; }
}

extern "C" void kinoko_sqrat_trim_stack(struct SQVM * id, int32_t base) {
    if (!id) return;
    auto vm = static_cast<SQVM *>(id);
    const SQInteger top = sq_gettop(vm);
    if (top > base) kinoko_sq_pop(address(id), top - base);
}
extern "C" void * kinoko_sqrat_root_construct(void * storage, struct SQVM * id) {
    if (!storage || !id) return 0;
    auto vm = static_cast<SQVM *>(id);
    ObjectView object(storage);
    retdec_trace_i32("450e30:construct-object", address(storage));
    retdec_trace_i32("450e30:construct-pair", object.payload_address());
    object.vtable(kinoko_sqrat_object_vtable());
    object.vm(vm); object.owns(true); object.reset();
    object.vtable(kinoko_sqrat_root_vtable());
    const auto root = kinoko::script::upstream::sqrat_root(vm);
    object.value(root);
    trace_pair("450e30:after-pop-type", "450e30:after-pop-data", root);
    return storage;
}
extern "C" void kinoko_sqrat_object_release(void * storage) {
    if (!storage) return;
    ObjectView object(storage);
    if (object.owns() && object.vm())
        kinoko::script::upstream::sqrat_release(object.vm(), object.value());
    object.owns(false); object.reset(); object.vtable(kinoko_sqrat_object_vtable());
}
extern "C" int32_t kinoko_sqrat_get(void * storage, const char* name, void * out) {
    if (!storage || !name || !out) return 0;
    ObjectView object(storage);
    const auto vm = object.vm();
    if (!vm) return 0;
    reset_pair(out);
    TrimStack stack(vm);
    HSQOBJECT value;
    const bool found = kinoko::script::upstream::sqrat_get(vm, object.value(), name, value);
    write(out, value);
    return found;
}
extern "C" void kinoko_sqrat_retain_pair(struct SQVM * machine, const int32_t pair[2]) {
    if (!machine || !pair) return;
    // sq_addref reads the pair but does not mutate it (Squirrel 2.2.2 sqapi.cpp).
    // Copy the byte-backed ABI record before handing it to the source VM.
    auto value = read<HSQOBJECT>(pair);
    sq_addref(static_cast<SQVM *>(machine), &value);
}
extern "C" void kinoko_sqrat_assign_pair(struct SQVM * id, int32_t* destination, const int32_t* source) {
    if (!id || !destination || !source) return;
    const auto next = read<HSQOBJECT>(source);
    const auto previous = read<HSQOBJECT>(destination);
    auto vm = static_cast<SQVM *>(id);
    // These are external Sqrat class/property handles. Internal SQObjectPtr
    // counts do not register GC roots; retain before release also permits aliasing.
    kinoko::script::upstream::sqrat_retain(vm, next);
    kinoko::script::upstream::sqrat_release(vm, previous);
    write(destination, next);
}
extern "C" void kinoko_sqrat_release_pair(struct SQVM * id, int32_t* pair) {
    if (!pair) return;
    auto value = read<HSQOBJECT>(pair);
    if (value._type != OT_NULL || data_bits(value) != 0) kinoko::script::upstream::sqrat_release(static_cast<SQVM *>(id), value);
    reset_pair(pair);
}
extern "C" int32_t kinoko_sqrat_set_pair(struct SQVM * vm, const int32_t* object, const char* name, const int32_t* value) { return set_pair(vm, object, name, value, false); }
extern "C" int32_t kinoko_sqrat_raw_set_pair(struct SQVM * vm, const int32_t* object, const char* name, const int32_t* value) { return set_pair(vm, object, name, value, true); }
extern "C" int32_t kinoko_sqrat_bind_int(struct SQVM * vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, integer(value), false); }
extern "C" int32_t kinoko_sqrat_bind_bool(struct SQVM * vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, boolean(value), false); }
extern "C" int32_t kinoko_sqrat_raw_set_int(struct SQVM * vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, integer(value), true); }
extern "C" int32_t kinoko_sqrat_raw_set_bool(struct SQVM * vm, const int32_t* object, const char* name, int32_t value) { return set_value(vm, object, name, boolean(value), true); }
extern "C" int32_t kinoko_sqrat_raw_set_float(struct SQVM * vm, const int32_t* object, const char* name, float value) {
    HSQOBJECT o; o._type = OT_FLOAT; o._unVal.fFloat = value;
    return set_value(vm, object, name, o, true);
}
extern "C" int32_t kinoko_sqrat_bind_string(struct SQVM * vm, const int32_t* object, const char* name, const char* value) { return set_string(vm, object, name, value, false); }
extern "C" int32_t kinoko_sqrat_raw_set_string(struct SQVM * vm, const int32_t* object, const char* name, const char* value) { return set_string(vm, object, name, value, true); }
extern "C" int32_t kinoko_sqrat_set_native_closure(struct SQVM * id, const int32_t* object, const char* name, void * function, const int32_t* free_pair, int32_t free_count) {
    // The recovered interface accepts ONE optional pair, not an array of pairs.
    // Larger counts used to consume the name/receiver and underflow the stack.
    if (!id || !object || !name || !function || free_count < 0 || free_count > 1 ||
        (free_count && !free_pair)) return 0;
    auto vm = static_cast<SQVM *>(id);
    const auto receiver = read<HSQOBJECT>(object);
    HSQOBJECT capture; sq_resetobject(&capture);
    if (free_count) capture = read<HSQOBJECT>(free_pair);
    TrimStack stack(vm);
    sq_pushobject(vm, receiver); sq_pushstring(vm, name, -1);
    if (free_count) sq_pushobject(vm, capture);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(function), free_count);
    return SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse));
}
extern "C" int32_t kinoko_sqrat_set_offset_closure(struct SQVM * id, const int32_t* table, const char* name, int32_t offset, void * function) {
    if (!id || !table || !name || !function) return 0;
    auto vm = static_cast<SQVM *>(id);
    const auto receiver = read<HSQOBJECT>(table);
    TrimStack stack(vm);
    return kinoko::script::upstream::sqrat_bind_function(vm, receiver, name,
        &offset, sizeof(offset), reinterpret_cast<SQFUNCTION>(function), false);
}
extern "C" int32_t kinoko_sqrat_no_constructor(struct SQVM * id) {
    return kinoko::script::upstream::sqrat_no_constructor(static_cast<SQVM *>(id));
}
extern "C" int32_t kinoko_sqrat_initialize_class(struct SQVM * id, const int32_t* type, const int32_t* set_table, const int32_t* get_table, void * constructor, void * setter, void * getter, void * weakref) {
    if (!id || !type || !set_table || !get_table || !setter || !getter) return 0;
    auto callback = [](void* value) { return reinterpret_cast<SQFUNCTION>(value); };
    return kinoko::script::upstream::sqrat_initialize_class(static_cast<SQVM *>(id),
        read<HSQOBJECT>(type), read<HSQOBJECT>(set_table), read<HSQOBJECT>(get_table),
        callback(constructor), callback(setter), callback(getter), callback(weakref));
}
extern "C" int32_t kinoko_sqrat_new_class(struct SQVM * id, int32_t* output) {
    if (!id || !output) return 0;
    const auto value = kinoko::script::upstream::sqrat_new_class(static_cast<SQVM *>(id), true);
    write(output, value);
    return value._type == OT_CLASS;
}
extern "C" int32_t kinoko_sqrat_new_table(struct SQVM * id, int32_t* out) {
    if (!id || !out) return 0;
    auto vm = static_cast<SQVM *>(id);
    reset_pair(out);
    TrimStack stack(vm);
    const auto value = kinoko::script::upstream::sqrat_table(vm);
    write(out, value);
    static std::atomic<unsigned> traces{0};
    if (traces.fetch_add(1, std::memory_order_relaxed) < 32)
        trace_pair("sqrat:new-table-type", "sqrat:new-table-data", value);
    return value._type == OT_TABLE && _table(value) != nullptr;
}
extern "C" int32_t kinoko_sqrat_set_delegate(struct SQVM * id, const int32_t* object, const int32_t* delegate) {
    if (!id || !object || !delegate) return 0;
    const auto receiver = read<HSQOBJECT>(object), incoming = read<HSQOBJECT>(delegate);
    auto vm = static_cast<SQVM *>(id);
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
extern "C" struct SQVM * kinoko_sqrat_bind_object_function(void * storage, const char * name, const void * source, int32_t size, void * function, int32_t static_slot) {
    if (!storage) return nullptr;
    ObjectView object(storage);
    auto vm = object.vm();
    if (!vm || !function || size < 0 || (size && !source)) return nullptr;
    retdec_trace_squirrel_name("415550:name", address(name));
    retdec_trace_i32("415550:size", size);
    retdec_trace_i32("415550:native", address(function));
    // Execute Sqrat's actual BindFunc body, including userdata copy, closure,
    // publication and pop. The legacy entry returns VM, not BindFunc's void.
    kinoko::script::upstream::sqrat_bind_function(vm, object.value(),
        static_cast<const char *>(name), static_cast<const void *>(source), static_cast<size_t>(size),
        reinterpret_cast<SQFUNCTION>(function), (static_slot & 255) != 0);
    trace_pair("415550:after-pop-type", "415550:after-pop-data", object.value());
    return vm;
}
extern "C" void * __fastcall kinoko_sqrat_copy_object(void * receiver, void*, void * output) {
    const ObjectView object(receiver);
    write(output, kinoko::script::upstream::sqrat_object_value(object.vm(), object.value()));
    return output;
}
extern "C" void * __fastcall kinoko_sqrat_object_reference(void * receiver, void*) {
    // This slot returns a reference into the host record, not a temporary
    // source Object. Only the legacy storage address crosses this ABI bridge.
    return pointer(ObjectView(receiver).payload_address());
}
extern "C" void * __fastcall kinoko_sqrat_delete_object(void * receiver, void*, int32_t flags) {
    ObjectView object(receiver);
    object.vtable(kinoko_sqrat_object_vtable()); // visible during a release hook
    kinoko::script::upstream::sqrat_destroy_object(object.vm(), object.value(), object.owns());
    if (flags & 1) std::free(receiver);
    return receiver;
}
extern "C" int32_t kinoko_sqrat_invoke_callback(const void * storage) {
    if (!storage) return -1;
    const auto callback = read<CallbackStorage>(storage);
    if (!callback.vm) return -1;
    static std::atomic<unsigned> traces{0};
    if (traces.fetch_add(1, std::memory_order_relaxed) < 96) {
        retdec_trace_i32("415810:self", address(storage));
        trace_pair("415810:env-type", "415810:env-data", callback.environment);
        trace_pair("415810:closure-type", "415810:closure-data", callback.closure);
    }
    kinoko::script::upstream::sqrat_execute(callback.vm, callback.environment,
        callback.closure, native_trace_slot != 0,
        [](HSQUIRRELVM vm, SQInteger count, SQBool result, SQBool errors) -> SQRESULT {
            return kinoko_sq_call(address(vm), count, result, errors);
        });
    return address(callback.vm);
}
