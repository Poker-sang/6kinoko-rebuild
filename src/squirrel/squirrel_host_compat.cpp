#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_source_runtime.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include <cstdio>

extern "C" {
extern struct SQVM *kinoko_primary_vm;
void kinoko_trace(const char* message);
void kinoko_trace_i32(const char* message, int32_t value);
void kinoko_trace_squirrel_name(const char* message, int32_t name);
}

namespace {
using namespace kinoko::script;
inline SQVM*& current_vm_storage = kinoko_primary_vm;
HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(current_vm_storage); }
int32_t pop(HSQUIRRELVM vm, SQInteger count = 1) {
    // Keep the embedding's existing underflow guard; ownership is source Pop.
    return kinoko_sq_pop(address(vm), count);
}
void cannot_release(ObjectView object) {
    const auto value = object.value();
    if (value._type != OT_NULL && data_bits(value))
        std::printf("SquirrelObject::~SquirrelObject - Cannot release\n");
}
void trace_value(const char* type_label, const char* data_label, ObjectView object) {
    const auto value = object.value();
    kinoko_trace_i32(type_label, value._type);
    kinoko_trace_i32(data_label, data_bits(value));
}
// The snapshot returns lookup success, independently of userdata conversion.
// Aligned locals bridge the original possibly unaligned 32-bit output slots.
// A real Squirrel userdata payload is never null; failed conversions leave it
// null and must leave both caller outputs untouched.
bool get_userdata(HSQUIRRELVM vm, HSQOBJECT receiver, const char* key,
                  int32_t* output, void* tag_output, bool raw) {
    SQUserPointer data = nullptr, tag = nullptr;
    const bool found = upstream::sqplus_get_userdata(vm, receiver, key, &data,
        tag_output ? &tag : nullptr, raw);
    if (data) {
        const auto bits = address(data);
        std::memcpy(output, &bits, sizeof(bits));
        if (tag_output) {
            const auto tag_bits = address(tag);
            std::memcpy(tag_output, &tag_bits, sizeof(tag_bits));
        }
    }
    return found;
}
} // namespace

extern "C" void * kinoko_sqplus_object_initialize(void * object) {
    if (object) ObjectView(object).initialize(kinoko_squirrel_object_vtable());
    return object;
}
extern "C" void * kinoko_sqplus_object_copy_construct(void * object, const void * source) {
    kinoko_trace("4a9500:this-begin");
    kinoko_trace_i32("4a9500:this", address(object));
    kinoko_trace_i32("4a9500:source", address(source));
    if (source) trace_value("4a9500:source-type", "4a9500:source-data", ObjectView(source));
    ObjectView destination(object);
    destination.set_vtable(kinoko_squirrel_object_vtable());
    destination.write(ObjectView(source).value());
    destination.retain(current_vm());
    return object;
}
extern "C" void * kinoko_sqplus_object_construct_value(void * object, int32_t type, int32_t data) {
    static int trace_count;
    if (!object) return 0;
    if (trace_count < 32) {
        kinoko_trace_i32("4a9540:this", address(object));
        kinoko_trace_i32("4a9540:arg0", type);
        kinoko_trace_i32("4a9540:arg1", data);
    }
    ObjectView destination(object);
    destination.set_vtable(kinoko_squirrel_object_vtable());
    destination.write(borrowed_value(type, data));
    destination.retain(current_vm());
    if (trace_count < 32) {
        trace_value("4a9540:type", "4a9540:data", destination);
        ++trace_count;
    }
    return object;
}
extern "C" void * kinoko_sqplus_object_reset(void * object) {
    if (!object) return 0;
    ObjectView destination(object);
    kinoko_trace("4a9570:begin");
    if (current_vm()) {
        kinoko_trace("4a9570:before-release");
        destination.release(current_vm());
        kinoko_trace("4a9570:after-release");
    } else cannot_release(destination);
    kinoko_trace("4a9570:before-clear");
    destination.reset();
    kinoko_trace("4a9570:after-clear");
    return pointer(destination.payload_address());
}
extern "C" void * kinoko_sqplus_object_assign(void * object, const void * source) {
    kinoko_trace("4a95c0:begin");
    kinoko_trace_i32("4a95c0:this", address(object));
    kinoko_trace_i32("4a95c0:source", address(source));
    // Snapshot before releasing the old destination, including self-assignment.
    auto incoming = ObjectView(source).value();
    upstream::sqplus_retain(current_vm(), incoming);
    kinoko_trace("4a95c0:after-addref");
    ObjectView(object).release(current_vm());
    kinoko_trace("4a95c0:after-release");
    ObjectView(object).write(incoming);
    kinoko_trace("4a95c0:after-copy");
    return object;
}
extern "C" int32_t kinoko_sqplus_object_capture(void * object, int32_t index) {
    kinoko_trace("4a9660:this-begin");
    kinoko_trace_i32("4a9660:this", address(object));
    kinoko_trace_i32("4a9660:this-index", index);
    return object ? ObjectView(object).capture(current_vm(), index) : 0;
}
extern "C" struct SQVM * kinoko_sqplus_object_append(void * object, const void * source) {
    const auto receiver = ObjectView(object).value();
    if (receiver._type != OT_ARRAY) return nullptr;
    auto* vm = current_vm();
    upstream::sqplus_append(vm, receiver, ObjectView(source).value());
    return vm; // Retained legacy return, not the upstream void result.
}
extern "C" void * kinoko_sqplus_object_new_table(void * object) {
    kinoko_sqplus_object_initialize(object);
    ObjectView(object).write(upstream::sqplus_new_table(current_vm()));
    return object;
}
extern "C" void * kinoko_sqplus_object_new_array(void * object, int32_t size) {
    kinoko_sqplus_object_initialize(object);
    ObjectView(object).write(upstream::sqplus_new_array(current_vm(), size));
    return object;
}
extern "C" int32_t kinoko_sqplus_object_is_null(void * object) {
    return ObjectView(object).value()._type == OT_NULL;
}
extern "C" int32_t kinoko_sqplus_object_size(void * object) {
    return kinoko_squirrel_object_size(address(object), address(current_vm()));
}
extern "C" int32_t kinoko_sqplus_object_reverse(void * object) {
    return kinoko_squirrel_object_reverse(address(object), address(current_vm()));
}
extern "C" int32_t kinoko_sqplus_object_set_index_string(void * object, int32_t key, const char * text) {
    kinoko_trace("4a9730:begin");
    kinoko_trace_i32("4a9730:this", address(object));
    kinoko_trace_i32("4a9730:a2", key);
    kinoko_trace_i32("4a9730:source", address(text));
    // key is an SQInteger, not a pointer. The old diagnostic dereferenced it
    // even in quiet builds, making valid small integer keys crash.
    if (object) trace_value("4a9730:this-type", "4a9730:this-data", ObjectView(object));
    auto* vm = current_vm();
    const SQInteger top = sq_gettop(vm);
    kinoko_trace_i32("4a9730:stack-base", top);
    const int32_t result = upstream::sqplus_set_string(vm, ObjectView(object).value(),
        key, static_cast<const char *>(text));
    kinoko_trace_i32("4a9730:result", result);
    kinoko_trace("4a9730:after-c910");
    return result;
}
extern "C" int32_t kinoko_sqplus_object_raw_set_object(void * object, const void * key, const void * value) {
    if (!object || !key || !value) return 0;
    return upstream::sqplus_raw_set(current_vm(), ObjectView(object).value(),
                                   ObjectView(key).value(), ObjectView(value).value());
}
extern "C" int32_t kinoko_sqplus_object_raw_set_name(void * object, const char* key, const void * value) {
    return upstream::sqplus_raw_set(current_vm(), ObjectView(object).value(),
                                   key, ObjectView(value).value());
}
extern "C" int32_t kinoko_sqplus_object_new_userdata(void * object, const char * key, int32_t size, void * tag) {
    auto* vm = current_vm();
    const auto top = sq_gettop(vm);
    kinoko_trace("4a9950:begin");
    kinoko_trace_i32("4a9950:this", address(object));
    kinoko_trace_i32("4a9950:name", address(key));
    kinoko_trace_i32("4a9950:size", size);
    kinoko_trace_i32("4a9950:aux", address(tag));
    kinoko_trace_i32("4a9950:stack-before", top);
    if (object) trace_value("4a9950:this-type", "4a9950:this-data", ObjectView(object));
    const int32_t result = upstream::sqplus_new_userdata(vm, ObjectView(object).value(),
        static_cast<const char *>(key), size, tag);
    kinoko_trace_i32("4a9950:result", result);
    kinoko_trace_i32("4a9950:stack-after", sq_gettop(vm));
    return result;
}
extern "C" int32_t kinoko_sqplus_object_type(void * object) {
    return object ? ObjectView(object).value()._type : 0;
}
extern "C" int32_t kinoko_sqplus_object_get_index_integer(void * object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_get_integer(vm, ObjectView(object).value(), key);
}
extern "C" const char * kinoko_sqplus_object_get_index_string(void * object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return nullptr;
    return upstream::sqplus_get_string(vm, ObjectView(object).value(), key);
}
extern "C" void * kinoko_sqplus_object_get_index_userpointer(void * object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_get_userpointer(vm, ObjectView(object).value(), key);
}
extern "C" void * kinoko_sqplus_object_instance(void * object, void * tag) {
    if (!object) return 0;
    return upstream::sqplus_get_instance_up(current_vm(), ObjectView(object).value(), tag);
}
extern "C" int32_t kinoko_sqplus_object_set_instance(void * object, void * native_pointer) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_set_instance_up(vm, ObjectView(object).value(), native_pointer);
}
extern "C" int32_t kinoko_sqplus_object_begin_iteration(void * object) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_begin_iteration(vm, ObjectView(object).value());
}
extern "C" int32_t kinoko_sqplus_object_next(int32_t* key, int32_t* value) {
    auto* vm = current_vm();
    if (SQ_FAILED(sq_next(vm, -2))) return 0;
    ObjectView(key).capture(vm, -2);
    ObjectView(value).capture(vm, -1);
    pop(vm, 2); // Leave container and iterator for the next call.
    return 1;
}
extern "C" int32_t kinoko_sqplus_object_end_iteration(void) { return pop(current_vm(), 2); }
extern "C" int32_t kinoko_sqplus_object_typetag(void * object, int32_t* tag) {
    static int trace_count;
    if (!object || !tag) return 0;
    auto value = ObjectView(object).value();
    const bool trace = trace_count < 128 || value._type == OT_CLASS;
    if (trace) {
        kinoko_trace_i32("4a9d30:this", address(object));
        trace_value("4a9d30:type", "4a9d30:data", ObjectView(object));
        kinoko_trace_i32("4a9d30:out", address(tag));
    }
    SQUserPointer native_tag = nullptr;
    const int32_t result = upstream::sqplus_get_typetag(current_vm(), value, &native_tag);
    if (result) {
        const auto bits = address(native_tag);
        std::memcpy(tag, &bits, sizeof(bits));
    }
    if (trace) {
        kinoko_trace_i32("4a9d30:result", result);
        kinoko_trace_i32("4a9d30:value", result ? address(native_tag) : 0);
        ++trace_count;
    }
    return result;
}
extern "C" void* kinoko_sqplus_object_destroy(void * object) {
    return (void*)(intptr_t)(kinoko_squirrel_object_destroy(address(object), address(current_vm()), kinoko_squirrel_object_vtable()));
}
extern "C" void * kinoko_sqplus_object_assign_thread(void * object, struct SQVM * thread_address) {
    kinoko_trace("4a9e30:begin");
    kinoko_trace_i32("4a9e30:this", address(object));
    kinoko_trace_i32("4a9e30:source", address(thread_address));
    kinoko_trace_i32("4a9e30:gvm-before", address(current_vm()));
    if (!object) return 0;
    auto* vm = current_vm();
    ObjectView destination(object);
    if (!thread_address || !vm) {
        if (vm) destination.release(vm); else cannot_release(destination);
        destination.reset();
        return object;
    }
    auto* thread = static_cast<SQVM *>(thread_address);
    kinoko_trace_i32("4a9e30:source-ref-before", thread->_uiRef);
    {
        // Replace manual +4 refcount writes/virtual Release dispatch with
        // the exact temporary lifetime in the supplied SQObjectPtr source.
        SQObjectPtr retained(thread);
        {
            SQObjectPtr pushed(retained);
            if (static_cast<SQUnsignedInteger>(vm->_top) >= vm->_stack.size()) sq_reservestack(vm, 1);
            vm->Push(pushed);
        }
        kinoko_trace_i32("4a9e30:gvm-after-push", address(current_vm()));
    }
    kinoko_trace_i32("4a9e30:source-ref-after", thread->_uiRef);
    HSQOBJECT result;
    sq_resetobject(&result);
    sq_getstackobj(current_vm(), -1, &result);
    kinoko_trace_i32("4a9e30:result-type", result._type);
    kinoko_trace_i32("4a9e30:result-data", data_bits(result));
    sq_addref(current_vm(), &result);
    destination.release(current_vm());
    kinoko_trace_i32("4a9e30:gvm-after-release", address(current_vm()));
    destination.write(result);
    pop(current_vm());
    kinoko_trace_i32("4a9e30:gvm-after-pop", address(current_vm()));
    return object;
}
extern "C" int32_t kinoko_sqplus_object_set_delegate(void * object, const void * delegate) {
    const auto type = ObjectView(object).value()._type;
    const auto delegate_type = ObjectView(delegate).value()._type;
    kinoko_trace_i32("4a9f60:this-type", type);
    kinoko_trace_i32("4a9f60:source-type", delegate_type);
    kinoko_trace_i32("4a9f60:this-data", data_bits(ObjectView(object).value()));
    kinoko_trace_i32("4a9f60:source-data", data_bits(ObjectView(delegate).value()));
    const auto result = upstream::sqplus_set_delegate(current_vm(),
        ObjectView(object).value(), ObjectView(delegate).value());
    kinoko_trace_i32("4a9f60:setdelegate-result", result);
    return result;
}
extern "C" int32_t kinoko_sqplus_object_get_userdata(void * object, const char * key, void * output, void * tag_output) {
    kinoko_trace("4aa080:begin");
    kinoko_trace_i32("4aa080:this", address(object));
    kinoko_trace_squirrel_name("4aa080:name", address(key));
    kinoko_trace_i32("4aa080:out", address(output));
    kinoko_trace_i32("4aa080:aux", address(tag_output));
    if (object) trace_value("4aa080:this-type", "4aa080:this-data", ObjectView(object));
    auto* vm = current_vm();
    kinoko_trace_i32("4aa080:stack-before", sq_gettop(vm));
    const int32_t result = get_userdata(vm, ObjectView(object).value(),
        static_cast<const char *>(key), static_cast<int32_t *>(output), tag_output, false);
    kinoko_trace_i32("4aa080:result", result);
    kinoko_trace_i32("4aa080:stack-after", sq_gettop(vm));
    return result;
}
extern "C" int32_t kinoko_sqplus_object_raw_get_userdata(void * object, const char* key, int32_t* output, void * tag_output) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return get_userdata(vm, ObjectView(object).value(), key, output, tag_output, true);
}
extern "C" int32_t kinoko_sqplus_object_exists(void * object, const char* key) {
    return upstream::sqplus_exists(current_vm(), ObjectView(object).value(), key);
}
extern "C" void * kinoko_sqplus_object_get_delegate(void * object, void * output) {
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    // Snapshot AFTER initialization, preserving the legacy output==receiver case.
    destination.write(upstream::sqplus_get_delegate(current_vm(), ObjectView(object).value()));
    return output;
}
extern "C" void * kinoko_sqplus_object_get_value(void * object, void * output, const char* key) {
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    destination.write(upstream::sqplus_get_value(current_vm(), ObjectView(object).value(), key));
    return output;
}
extern "C" void * kinoko_sqplus_object_new_instance(void * object, const void * klass) {
    if (!object) return 0;
    kinoko_sqplus_object_initialize(object);
    auto* vm = current_vm();
    if (!klass || !vm) return object;
    trace_value("4a90c0:source-type", "4a90c0:source-data", ObjectView(klass));
    kinoko_trace_i32("4a90c0:vm", address(vm));
    const auto top = sq_gettop(vm);
    kinoko_trace_i32("4a90c0:stack-before", top);
    HSQOBJECT result;
    sq_resetobject(&result);
    if (!upstream::sqplus_new_instance(vm, ObjectView(klass).value(), result)) {
        kinoko_trace("4a90c0:instance-get-failed");
        return object;
    }
    kinoko_trace("4a90c0:after-instance-get");
    ObjectView(object).write(result); // transfer the source factory's owned ref
    kinoko_trace("4a90c0:after-copy");
    return object;
}
