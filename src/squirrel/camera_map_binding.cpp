#include "kinoko/map_manager_records.hpp"
#include "kinoko/camera_records.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/legacy_abi.h"
#include <cstring>
extern "C" {
extern struct SQVM *kinoko_primary_vm;
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_table_entries(const char*, int32_t);
int32_t kinoko_camera_update_entry(int32_t);
}
namespace {
inline SQVM*& camera_vm_slot = kinoko_primary_vm;
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class T> int32_t entry(T target) { return static_cast<int32_t>(reinterpret_cast<intptr_t>(target)); }
struct Field { const char* name; int32_t offset; bool integer; };
constexpr Field camera_fields[] = {
    {"x", offsetof(kinoko::camera::Record, x), false},
    {"y", offsetof(kinoko::camera::Record, y), false},
    {"cx", offsetof(kinoko::camera::Record, center_x), false},
    {"cy", offsetof(kinoko::camera::Record, center_y), false},
    {"offset_x", offsetof(kinoko::camera::Record, offset_x), false},
    {"offset_y", offsetof(kinoko::camera::Record, offset_y), false},
    {"left", offsetof(kinoko::camera::Record, bounds) + offsetof(kinoko::camera::Bounds, left), false},
    {"top", offsetof(kinoko::camera::Record, bounds) + offsetof(kinoko::camera::Bounds, top), false},
    {"right", offsetof(kinoko::camera::Record, bounds) + offsetof(kinoko::camera::Bounds, right), false},
    {"bottom", offsetof(kinoko::camera::Record, bounds) + offsetof(kinoko::camera::Bounds, bottom), false},
    {"width", offsetof(kinoko::camera::Record, width), false},
    {"height", offsetof(kinoko::camera::Record, height), false},
};
constexpr Field map_fields[] = {
    {"width", offsetof(kinoko::map::ManagerRecord, width), true},
    {"height", offsetof(kinoko::map::ManagerRecord, height), true},
    {"last_id", offsetof(kinoko::map::ManagerRecord, last_id), true},
    {"last_left", offsetof(kinoko::map::ManagerRecord, last_bounds) + offsetof(kinoko::camera::Bounds, left), false},
    {"last_top", offsetof(kinoko::map::ManagerRecord, last_bounds) + offsetof(kinoko::camera::Bounds, top), false},
    {"last_right", offsetof(kinoko::map::ManagerRecord, last_bounds) + offsetof(kinoko::camera::Bounds, right), false},
    {"last_bottom", offsetof(kinoko::map::ManagerRecord, last_bounds) + offsetof(kinoko::camera::Bounds, bottom), false},
};
// A 48-byte registration frame: VM/name, three SqPlus objects and the
// original unused word at +20. Each object retains its own external handle.
struct ClassState {
    SQVM *vm;
    const char *name;
    ObjectStorage klass;
    int32_t unused20;
    ObjectStorage first_table, second_table;
};
static_assert(sizeof(ClassState) == 48);
static_assert(offsetof(ClassState, klass) == 8);
static_assert(offsetof(ClassState, first_table) == 24);
static_assert(offsetof(ClassState, second_table) == 36);

int32_t create_class(ObjectStorage *output, SQVM *vm, const char *name,
                     const char *parent, int32_t *descriptor) {
    const auto top = sq_gettop(vm);
    kinoko_sqplus_object_initialize(output);
    if (kinoko_sqplus_create_class(vm, output, descriptor, name, parent)) {
        ObjectStorage temporary{};
        kinoko_sqplus_object_initialize(&temporary);
        ObjectView(output).push(current_vm());
        kinoko_sqplus_object_capture(&temporary, -1);
        sq_pop(current_vm(), 1);
        kinoko_sqplus_setup_hierarchy(reinterpret_cast<int32_t *>(&temporary)); // Consumes the by-value object.
    }
    sq_settop(vm, top);
    return address(output);
}
void construct(ClassState &state, const char *name, int32_t *descriptor) {
    state.vm = reinterpret_cast<SQVM *>(camera_vm_slot);
    state.name = name;
    state.unused20 = 0;
    kinoko_sqplus_object_initialize(&state.klass);
    kinoko_sqplus_object_new_table(&state.first_table);
    kinoko_sqplus_object_new_table(&state.second_table);
    ObjectStorage temporary{};
    create_class(&temporary, state.vm, state.name, nullptr, descriptor);
    kinoko_sqplus_object_assign(&state.klass, &temporary);
    kinoko_sqplus_object_destroy(&temporary);
}
template<size_t N> void bind_fields(int32_t* object, int32_t* descriptor, const Field (&fields)[N]) {
    for (const auto& field : fields) {
        auto bind = field.integer ? kinoko_sqplus_bind_integer : kinoko_sqplus_bind_float;
        bind(object, descriptor, field.offset, const_cast<char*>(field.name), 0);
    }
}
struct CameraTarget { void *instance; void *method_slot; };
static_assert(sizeof(CameraTarget) == 8);
CameraTarget camera_receiver(SQVM *vm) {
    const auto top = sq_gettop(vm);
    SQUserPointer receiver = nullptr, payload = nullptr, tag = nullptr;
    CameraTarget result{
        SQ_SUCCEEDED(sq_getinstanceup(vm, 1, &receiver, nullptr)) ? receiver : nullptr,
        nullptr};
    const auto status = top >= 1 ? sq_getuserdata(vm, top, &payload, &tag) : 0;
    if (top >= 1 && SQ_SUCCEEDED(status) && !tag) result.method_slot = payload;
    // 466669..46668E uses the complete eight-byte SQObject.
    HSQOBJECT instance; sq_resetobject(&instance);
    if (top >= 1) sq_getstackobj(vm, 1, &instance);
    static int traces;
    if (traces < 16) {
        ++traces;
        retdec_trace_i32("4665e0:vm", address(vm));
        retdec_trace_i32("4665e0:result", address(&result));
        retdec_trace_i32("4665e0:stack-count", top);
        retdec_trace_i32("4665e0:instance-up", address(result.instance));
        retdec_trace_i32("4665e0:userdata-status", status);
        retdec_trace_i32("4665e0:userdata-payload", address(payload));
        retdec_trace_i32("4665e0:userdata-tag", address(tag));
        retdec_trace_i32("4665e0:instance-type", instance._type);
        retdec_trace_i32("4665e0:instance-data", data_bits(instance));
    }
    ObjectStorage object{};
    kinoko_sqplus_object_construct_value(&object, instance._type, data_bits(instance));
    int32_t type = 0;
    kinoko_sqplus_object_typetag(&object, &type);
    if (type != address(kinoko_camera_binding_type())) {
        ObjectStorage inherited{};
        kinoko_sqplus_object_get_value(&object, &inherited, "__ot");
        result.instance = kinoko_sqplus_object_get_index_userpointer(
            &inherited, address(kinoko_camera_binding_type()));
        kinoko_sqplus_object_destroy(&inherited);
    }
    kinoko_sqplus_object_destroy(&object);
    return result;
}

} // namespace

extern "C" int32_t function_466770(int32_t *output, int32_t vm,
                                      int32_t name, int32_t parent) {
    return create_class(reinterpret_cast<ObjectStorage *>(output), pointer<SQVM>(vm),
        pointer<const char>(name), pointer<const char>(parent), kinoko_camera_binding_type());
}
extern "C" int32_t function_46f200(int32_t *output, int32_t vm,
                                      int32_t name, int32_t parent) {
    return create_class(reinterpret_cast<ObjectStorage *>(output), pointer<SQVM>(vm),
        pointer<const char>(name), pointer<const char>(parent), kinoko_map_binding_type());
}

namespace {
int32_t kinoko_call_camera_update(SQVM *vm) {
    static int32_t camera_native_trace_count;
    const auto target = camera_receiver(vm);
    if (!target.instance || !target.method_slot)
        return sq_throwerror(vm, "Invalid Instance Type");
    const auto method = load<void *>(target.method_slot);
    if (!method) return sq_throwerror(vm, "Invalid Instance Type");
    // 4668B3 passes the three words of a by-value SqPlus argument. The callee
    // consumes its reference; this caller does not destroy it again.
    int32_t arguments[3]{};
    kinoko_sqplus_argument_object(arguments, 0, vm);
    if (camera_native_trace_count < 16) {
        retdec_trace_i32("466890:vm", address(vm));
        retdec_trace_i32("466890:native-instance", address(target.instance));
        retdec_trace_i32("466890:type-info", address(target.method_slot));
        retdec_trace_i32("466890:method", address(method));
        retdec_trace_i32("466890:arg0", arguments[0]);
        retdec_trace_i32("466890:arg1", arguments[1]);
        retdec_trace_i32("466890:arg2", arguments[2]);
        ++camera_native_trace_count;
    }
    retdec_call_thiscall3_result(target.instance, method,
                                arguments[0], arguments[1], arguments[2]);
    return 0;
}
int32_t register_camera_binding_impl() {
    ObjectStorage root{};
    ClassState state{};
    kinoko_sqplus_object_copy_construct(&root, kinoko_sqplus_root_object());
    construct(state, "Camera", kinoko_camera_binding_type());
    kinoko_sqplus_object_assign(kinoko_camera_map_script_symbols()->camera_class, &state.klass);
    auto *vm = state.vm;
    ObjectView(&state.klass).push(vm);
    sq_pushstring(vm, "SetUpdateFunction", -1);
    const auto target = entry(kinoko_camera_set_update_callback);
    std::memcpy(sq_newuserdata(vm, sizeof(target)), &target, sizeof(target));
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(kinoko_camera_update_entry), 1);
    sq_newslot(vm, -3, SQFalse); sq_pop(vm, 1);
    bind_fields(reinterpret_cast<int32_t *>(&state.klass), kinoko_camera_binding_type(), camera_fields);
    const auto value = ObjectView(&state.klass).value();
    if (data_bits(value)) {
        retdec_trace_i32("camera-class:type", value._type);
        retdec_trace_i32("camera-class:data", data_bits(value));
        retdec_trace_squirrel_table_entries("camera-class-members", load<int32_t>(data_bits(value)+24));
    }
    kinoko_sqplus_object_destroy(&state.second_table);
    kinoko_sqplus_object_destroy(&state.first_table);
    kinoko_sqplus_object_destroy(&state.klass);
    kinoko_sqplus_object_destroy(&root);
    return 0;
}
int32_t register_map_binding_impl() {
    ClassState state{};
    ObjectStorage layer{};
    construct(state, "Map", kinoko_map_binding_type());
    kinoko_sqplus_object_assign(kinoko_camera_map_script_symbols()->map_class, &state.klass);
    bind_fields(reinterpret_cast<int32_t *>(&state.klass), kinoko_map_binding_type(), map_fields);
    // Original 46FD05 constructs exactly one local layer_name object.
    kinoko_sqplus_object_initialize(&layer);
    kinoko_sqplus_object_raw_set_name(kinoko_camera_map_script_symbols()->map_class, "layer_name", &layer);
    kinoko_sqplus_object_destroy(&layer);
    kinoko_sqplus_object_destroy(&state.second_table);
    kinoko_sqplus_object_destroy(&state.first_table);
    return address(kinoko_sqplus_object_destroy(&state.klass));
}
} // namespace
extern "C" int32_t kinoko_camera_update_entry(int32_t vm) {
    return kinoko_call_camera_update(pointer<SQVM>(vm));
}
extern "C" int32_t kinoko_register_camera_binding(void) { return register_camera_binding_impl(); }
extern "C" int32_t function_4669d0(void) { return kinoko_register_camera_binding(); }
extern "C" int32_t kinoko_register_map_binding(void) { return register_map_binding_impl(); }
extern "C" int32_t function_46fac0(void) { return kinoko_register_map_binding(); }
