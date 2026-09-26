#include "kinoko/squirrel_game_objects.h"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/upstream_bindings.hpp"
#include <algorithm>

extern "C" {
extern struct SQVM *kinoko_primary_vm;
void kinoko_trace_i32(const char*, int32_t);
}

namespace {
using namespace kinoko::script;
// These are byte-copyable ABI records, not overlaid C++ objects.
using MemoryReader = KinokoScriptMemoryReader;
struct ActCallback { SQVM* vm; HSQOBJECT environment; HSQOBJECT closure; };
struct ResourceRoot { SQVM* vm; HSQOBJECT root; };
static_assert(sizeof(MemoryReader) == 12 && sizeof(ActCallback) == 20);
static_assert(offsetof(ActCallback, closure) == 12 && sizeof(ResourceRoot) == 12);
constexpr size_t resource_root_offset = offsetof(kinoko::act::RuntimeRecord, vm);
constexpr size_t script_data_offset = offsetof(kinoko::act::ScriptStorageRecord, bytes);
constexpr size_t script_size_offset = offsetof(kinoko::act::ScriptStorageRecord, size);

template<class T> T read(const void* bytes) {
    T result; std::memcpy(&result, bytes, sizeof(result)); return result;
}
template<class T> void write(void* bytes, const T& value) {
    std::memcpy(bytes, &value, sizeof(value));
}
unsigned char* bytes(void* storage) { return static_cast<unsigned char*>(storage); }
HSQOBJECT empty() { HSQOBJECT value; sq_resetobject(&value); return value; }
class TrimStack final {
public:
    explicit TrimStack(HSQUIRRELVM vm) : vm_(vm), top_(sq_gettop(vm)) {}
    ~TrimStack() {
        const auto excess = sq_gettop(vm_) - top_;
        if (excess > 0) sq_pop(vm_, excess); // Never pad a depleted stack.
    }
    SQInteger top() const { return top_; }
    TrimStack(const TrimStack&) = delete;
    TrimStack& operator=(const TrimStack&) = delete;
private:
    HSQUIRRELVM vm_; SQInteger top_;
};
SQInteger read_bytecode(SQUserPointer state, SQUserPointer output, SQInteger count) {
    return kinoko_script_read_memory(state, output, count);
}
bool valid_class(HSQUIRRELVM vm, const int32_t* input, const void* native, int32_t* output) {
    if (!vm || !input || !native || !output) return false;
    const auto value = read<HSQOBJECT>(input);
    return value._type == OT_CLASS && data_bits(value) != 0;
}
bool create_instance(HSQUIRRELVM vm, const HSQOBJECT& type, void* native,
                     HSQOBJECT& result) {
    if (!upstream::sqrat_push_instance(vm, type, native)) return false;
    sq_getstackobj(vm, -1, &result);
    sq_addref(vm, &result);
    return true;
}
}

extern "C" void* kinoko_push_script_object(SQVM* machine, void* object) {
    if (!machine || !object) return 0;
    ObjectView(object).push(machine);
    // Return the pushed stack slot address, as in the original VM ABI.
    return pointer<void>(kinoko_sq_get_up(address(machine), -1));
}

extern "C" int32_t kinoko_script_read_memory(void* stream, void* destination, int32_t requested) {
    if (!stream || requested <= 0) return 0;
    auto state = read<MemoryReader>(stream);
    const auto base = reinterpret_cast<uintptr_t>(state.base);
    const auto cursor = reinterpret_cast<uintptr_t>(state.cursor);
    if (!base || state.size < 0 || cursor < base) return 0;
    const auto consumed = cursor - base;
    if (consumed > static_cast<uint32_t>(state.size)) return 0;
    const auto count = std::min(static_cast<uint32_t>(requested),
        static_cast<uint32_t>(state.size) - static_cast<uint32_t>(consumed));
    if (!count || !destination) return 0;
    std::memcpy(destination, state.cursor, count);
    state.cursor += count;
    write(stream, state);
    return static_cast<int32_t>(count);
}
extern "C" int32_t kinoko_create_bound_instance(struct SQVM* id, const int32_t* parent, const char* name, const int32_t* type, void* native, int32_t* output) {
    auto vm = id;
    auto* native_pointer = native;
    if (!parent || !name || !valid_class(vm, type, native_pointer, output)) return 0;
    const auto parent_value = read<HSQOBJECT>(parent);
    const auto class_value = read<HSQOBJECT>(type);
    write(output, empty());
    TrimStack restore(vm);
    sq_pushobject(vm, parent_value);
    sq_pushstring(vm, name, -1);
    HSQOBJECT result = empty();
    if (!create_instance(vm, class_value, native_pointer, result)) return 0;
    if (SQ_FAILED(sq_newslot(vm, -3, SQFalse))) {
        sq_release(vm, &result);
        return 0;
    }
    write(output, result);
    return result._type == OT_INSTANCE && data_bits(result) != 0;
}
extern "C" int32_t kinoko_create_unbound_instance(struct SQVM* id, const int32_t* type, void* native, int32_t* output) {
    auto vm = id;
    auto* native_pointer = native;
    if (!valid_class(vm, type, native_pointer, output)) return 0;
    const auto class_value = read<HSQOBJECT>(type);
    write(output, empty());
    TrimStack restore(vm);
    HSQOBJECT result = empty();
    if (!create_instance(vm, class_value, native_pointer, result)) return 0;
    write(output, result);
    return result._type == OT_INSTANCE && data_bits(result) != 0;
}
extern "C" void kinoko_release_act_callback(void* record) {
    if (!record) return;
    auto callback = read<ActCallback>(record);
    if (callback.closure._type == OT_NULL) return;
    if (callback.vm) upstream::sqrat_release_function(callback.vm,
        callback.environment, callback.closure);
    callback.environment = empty(); callback.closure = empty();
    write(record, callback);
}
extern "C" void kinoko_copy_act_callback(struct SQVM* id, void* script, int32_t offset, void* global, const char* name) {
    auto vm = id;
    if (!vm || !script || !global || !name) return;
    auto* destination = bytes(script) + offset;
    kinoko_release_act_callback(destination);
    auto callback = read<ActCallback>(destination);
    int32_t pair[2]; write(pair, empty());
    const auto found = kinoko_sqrat_get((void *)(intptr_t)(global), name, (void *)(pair));
    // Only metadata diagnostics: no additional scripted lookup or path dereference.
    kinoko_trace_i32("act:copy-update-script", address(script));
    kinoko_trace_i32("act:copy-update-result", found);
    if (found) {
        callback.vm = id;
        callback.environment = read<HSQOBJECT>(bytes(global) + 8);
        callback.closure = read<HSQOBJECT>(pair);
        upstream::sqrat_retain_function(vm, callback.environment, callback.closure);
        write(destination, callback);
    }
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(id), pair);
}
extern "C" int32_t kinoko_bind_act_resource_root(void* resource, struct SQVM* id, const int32_t* pair) {
    if (!resource || !id || !pair) return 0;
    auto incoming = read<HSQOBJECT>(pair);
    if (incoming._type != OT_TABLE || !data_bits(incoming)) return 0;
    auto vm = id;
    auto* storage = bytes(resource) + resource_root_offset;
    auto previous = read<ResourceRoot>(storage);
    // Snapshot and acquire BEFORE release: pair may be this resource's own root
    // and that root may have its last live handle in the resource.
    sq_addref(vm, &incoming);
    if ((previous.root._type & SQOBJECT_REF_COUNTED) && data_bits(previous.root))
        sq_release(vm, &previous.root);
    write(storage, ResourceRoot{id, incoming});
    kinoko_trace_i32("450e30:root-resource", address(resource));
    kinoko_trace_i32("450e30:root-vm", address(id));
    return 1;
}
static int32_t execute_embedded_act_script(struct SQVM* id, void* script, const int32_t* environment, int runs) {
    auto vm = id;
    if (!vm || !script || !environment) return 0;
    const auto data = read<const unsigned char*>(bytes(script) + script_data_offset);
    const auto size = read<int32_t>(bytes(script) + script_size_offset);
    if (!data || size < 2 || read<uint16_t>(data) != SQ_BYTECODE_STREAM_TAG)
        return 0;
    MemoryReader reader{data, size, data};
    TrimStack restore(vm);
    const auto load = sq_readclosure(vm, read_bytecode, &reader);
    kinoko_trace_i32("act-script:readclosure-result", load);
    if (SQ_FAILED(load) || sq_gettop(vm) <= restore.top()) return 0;
    HSQOBJECT closure = empty(); sq_getstackobj(vm, -1, &closure);
    if (closure._type != OT_CLOSURE || !data_bits(closure)) return 0;
    // Source LocalScript::Run leaves the read closure below its own call pair.
    bool result = false;
    for (int run = 0; run < runs; ++run) {
        result = upstream::sqrat_run_script(vm, closure, read<HSQOBJECT>(environment),
            [](HSQUIRRELVM target, SQInteger count, SQBool value, SQBool errors) -> SQRESULT {
                return kinoko_sq_call(address(target), count, value, errors);
            });
    }
    kinoko_trace_i32("act-script:execute-result", result ? SQ_OK : SQ_ERROR);
    return result;
}
extern "C" int32_t kinoko_execute_embedded_act_script(struct SQVM* vm, void* script, const int32_t* environment) {
    return execute_embedded_act_script(vm, script, environment, 1);
}
extern "C" int32_t kinoko_execute_act_file_bytecode(struct SQVM* vm, void* script, const int32_t* environment) {
    // 416A8D calls the loaded closure, then 416AE8 calls LocalScript::Run on
    // that same closure. The first call's failure is not used as a branch.
    return execute_embedded_act_script(vm, script, environment, 2);
}
