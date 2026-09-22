#include "kinoko/map_manager_records.hpp"
#include "kinoko/camera_records.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/legacy_abi.h"
#include <cstring>
extern "C" {
extern int32_t g611[3], g636[3];
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_table_entries(const char*, int32_t);
int32_t function_466890(int32_t);
}
namespace {
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
int32_t create_class(int32_t* output, int32_t vm, int32_t name, int32_t parent, int32_t* descriptor) {
    const auto top = sq_gettop(pointer<SQVM>(vm));
    kinoko_sqplus_object_initialize((void *)(output));
    if (kinoko_sqplus_create_class((struct SQVM *)(intptr_t)(vm), (void *)(output), descriptor, (const char *)(intptr_t)(name), (const char *)(intptr_t)(parent))) {
        int32_t temporary[3];
        kinoko_sqplus_object_initialize((void *)(temporary));
        ObjectView(output).push(current_vm());
        kinoko_sqplus_object_capture((void *)(temporary), -1);
        sq_pop(current_vm(), 1);
        kinoko_sqplus_setup_hierarchy(temporary); // Consumes its by-value object.
    }
    sq_settop(pointer<SQVM>(vm), top);
    return address(output);
}
void construct(int32_t state[12], const char* name, int32_t* descriptor) {
    state[0] = address(g644); state[1] = address(name); state[5] = 0;
    kinoko_sqplus_object_initialize((void *)(state + 2));
    kinoko_sqplus_object_new_table((void *)(intptr_t)(state + 6))); (int32_t*)(intptr_t)(kinoko_sqplus_object_new_table((void *)(intptr_t)(state + 9));
    int32_t temporary[3]{};
    create_class(temporary, state[0], state[1], 0, descriptor);
    kinoko_sqplus_object_assign((void *)(state + 2), (const void *)(temporary));
    kinoko_sqplus_object_destroy((void *)(temporary));
}
template<size_t N> void bind_fields(int32_t* object, int32_t* descriptor, const Field (&fields)[N]) {
    for (const auto& field : fields) {
        auto bind = field.integer ? kinoko_sqplus_bind_integer : kinoko_sqplus_bind_float;
        bind(object, descriptor, field.offset, const_cast<char*>(field.name), 0);
    }
}
void camera_receiver(int32_t result[2], int32_t vm_address) {
    auto* vm = pointer<SQVM>(vm_address);
    const auto top = sq_gettop(vm);
    SQUserPointer receiver = nullptr, payload = nullptr, tag = nullptr;
    result[0] = SQ_SUCCEEDED(sq_getinstanceup(vm, 1, &receiver, nullptr)) ? address(receiver) : 0;
    const auto status = top >= 1 ? sq_getuserdata(vm, top, &payload, &tag) : 0;
    result[1] = top >= 1 && SQ_SUCCEEDED(status) && !tag ? address(payload) : 0;
    // 466669..46668E uses a complete 8-byte SQObject, not one stack word.
    HSQOBJECT instance; sq_resetobject(&instance);
    if (top >= 1) sq_getstackobj(vm, 1, &instance);
    static int traces;
    if (traces < 16) {
        ++traces;
        retdec_trace_i32("4665e0:vm", vm_address);
        retdec_trace_i32("4665e0:result", address(result));
        retdec_trace_i32("4665e0:stack-count", top);
        retdec_trace_i32("4665e0:instance-up", result[0]);
        retdec_trace_i32("4665e0:userdata-status", status);
        retdec_trace_i32("4665e0:userdata-payload", address(payload));
        retdec_trace_i32("4665e0:userdata-tag", address(tag));
        retdec_trace_i32("4665e0:instance-type", instance._type);
        retdec_trace_i32("4665e0:instance-data", data_bits(instance));
    }
    int32_t object[3]{}, type = 0;
    kinoko_sqplus_object_construct_value((void *)(object), instance._type, data_bits(instance));
    kinoko_sqplus_object_typetag((void *)(object), &type);
    if (type != address(kinoko_camera_binding_type())) {
        int32_t inherited[3]{};
        kinoko_sqplus_object_get_value((void *)(object), (void *)(inherited), "__ot");
        result[0] = (int32_t)(intptr_t)(kinoko_sqplus_object_get_index_userpointer((void *)(inherited), address(kinoko_camera_binding_type())));
        kinoko_sqplus_object_destroy((void *)(inherited));
    }
    kinoko_sqplus_object_destroy((void *)(object));
}
} // namespace

extern "C" int32_t function_466770(int32_t* output, int32_t vm, int32_t name, int32_t parent) {
    return create_class(output, vm, name, parent, kinoko_camera_binding_type());
}
extern "C" int32_t function_46f200(int32_t* output, int32_t vm, int32_t name, int32_t parent) {
    return create_class(output, vm, name, parent, kinoko_map_binding_type());
}

extern "C" int32_t function_466890(int32_t a1) {
    static int32_t camera_native_trace_count;
    int32_t result[3] = { 0, 0, 0 };
    int32_t arguments[3] = { 0, 0, 0 };
    int32_t native_instance;
    int32_t type_info;
    int32_t method;

    // 0x466890: sub_4665E0(&result, a1)
    camera_receiver(result, a1);
    native_instance = result[0];
    type_info = result[1];
    if (native_instance == 0 || type_info == 0) {
        // 0x4668de
        return sq_throwerror(pointer<SQVM>(a1), "Invalid Instance Type");
    }
    method = *(int32_t *)(intptr_t)type_info;
    if (method == 0)
        return sq_throwerror(pointer<SQVM>(a1), "Invalid Instance Type");

    // 0x4668b3: the native callback is __thiscall with three SQ arguments.
    kinoko_sqplus_argument_object(arguments, 0, (struct SQVM *)(intptr_t)(a1));
    if (camera_native_trace_count < 16) {
        retdec_trace_i32("466890:vm", a1);
        retdec_trace_i32("466890:native-instance", native_instance);
        retdec_trace_i32("466890:type-info", type_info);
        retdec_trace_i32("466890:method", method);
        retdec_trace_i32("466890:arg0", arguments[0]);
        retdec_trace_i32("466890:arg1", arguments[1]);
        retdec_trace_i32("466890:arg2", arguments[2]);
        ++camera_native_trace_count;
    }
    retdec_call_thiscall3_result(
        (void *)(intptr_t)native_instance,
        (void *)(intptr_t)method,
        arguments[0], arguments[1], arguments[2]);
    return 0;
}

extern "C" int32_t function_4669d0(void) {
    int32_t root[3]{}, state[12]{};
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(root), kinoko_sqplus_root_object());
    construct(state, "Camera", kinoko_camera_binding_type());
    kinoko_sqplus_object_assign((void *)(g611), (const void *)(state + 2));
    auto* vm = pointer<SQVM>(state[0]);
    ObjectView(state + 2).push(vm);
    sq_pushstring(vm, "SetUpdateFunction", -1);
    const auto target = entry(kinoko_camera_set_update_callback);
    std::memcpy(sq_newuserdata(vm, 4), &target, 4);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(function_466890), 1);
    sq_newslot(vm, -3, SQFalse); sq_pop(vm, 1);
    bind_fields(state + 2, kinoko_camera_binding_type(), camera_fields);
    if (state[4]) {
        retdec_trace_i32("camera-class:type", state[3]);
        retdec_trace_i32("camera-class:data", state[4]);
        retdec_trace_squirrel_table_entries("camera-class-members", load<int32_t>(state[4]+24));
    }
    for (int offset : {9,6,2}) kinoko_sqplus_object_destroy((void *)(state + offset));
    kinoko_sqplus_object_destroy((void *)(root));
    return 0;
}
extern "C" int32_t function_46fac0(void) {
    int32_t state[12]{}, layer[3]{};
    construct(state, "Map", kinoko_map_binding_type());
    kinoko_sqplus_object_assign((void *)(g636), (const void *)(state + 2));
    bind_fields(state + 2, kinoko_map_binding_type(), map_fields);
    // Original 46FD05 constructs this local exactly once with an explicit receiver.
    kinoko_sqplus_object_initialize((void *)(layer));
    kinoko_sqplus_object_raw_set_name((void *)(g636), "layer_name", (const void *)(layer));
    kinoko_sqplus_object_destroy((void *)(layer));
    kinoko_sqplus_object_destroy((void *)(state + 9));
    kinoko_sqplus_object_destroy((void *)(state + 6));
    return kinoko_sqplus_object_destroy((void *)(state + 2));
}
