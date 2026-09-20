#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_source_runtime.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include <cstdio>

extern "C" {
extern char* g644;
void retdec_trace(const char* message);
void retdec_trace_i32(const char* message, int32_t value);
void retdec_trace_squirrel_name(const char* message, int32_t name);
}

namespace {
using namespace kinoko::script;
HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(g644); }
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
    retdec_trace_i32(type_label, value._type);
    retdec_trace_i32(data_label, data_bits(value));
}
void push_key(HSQUIRRELVM vm, SQInteger key) { sq_pushinteger(vm, key); }
void push_key(HSQUIRRELVM vm, const char* key) { sq_pushstring(vm, key, -1); }

// Lookup success is the original return contract, even when the subsequent
// userdata conversion fails. A failed conversion must not overwrite outputs.
SQRESULT read_userdata(HSQUIRRELVM vm, int32_t* output, int32_t tag_output) {
    SQUserPointer data = nullptr, tag = nullptr;
    const auto status = sq_getuserdata(vm, -1, &data, tag_output ? &tag : nullptr);
    if (SQ_SUCCEEDED(status)) {
        const int32_t bits = address(data);
        std::memcpy(output, &bits, sizeof(bits));
        if (tag_output) {
            const int32_t tag_bits = address(tag);
            std::memcpy(pointer(tag_output), &tag_bits, sizeof(tag_bits));
        }
    }
    return status;
}
} // namespace

extern "C" int32_t function_4a94e0_this(int32_t object) {
    if (object) ObjectView(object).initialize(kinoko_squirrel_object_vtable());
    return object;
}
extern "C" int32_t retdec_msvc_0_Init_locks_std__QAE_XZ5_this(int32_t object) {
    return function_4a94e0_this(object);
}
extern "C" int32_t* function_4a9500_this(int32_t* object, int32_t source) {
    retdec_trace("4a9500:this-begin");
    retdec_trace_i32("4a9500:this", address(object));
    retdec_trace_i32("4a9500:source", source);
    if (source) trace_value("4a9500:source-type", "4a9500:source-data", ObjectView(source));
    ObjectView destination(object);
    destination.set_vtable(kinoko_squirrel_object_vtable());
    destination.write(ObjectView(source).value());
    destination.retain(current_vm());
    return object;
}
extern "C" int32_t function_4a9540_this(int32_t object, int32_t type, int32_t data) {
    static int trace_count;
    if (!object) return 0;
    if (trace_count < 32) {
        retdec_trace_i32("4a9540:this", object);
        retdec_trace_i32("4a9540:arg0", type);
        retdec_trace_i32("4a9540:arg1", data);
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
extern "C" int32_t function_4a9570_this(int32_t object) {
    if (!object) return 0;
    ObjectView destination(object);
    retdec_trace("4a9570:begin");
    if (current_vm()) {
        retdec_trace("4a9570:before-release");
        destination.release(current_vm());
        retdec_trace("4a9570:after-release");
    } else cannot_release(destination);
    retdec_trace("4a9570:before-clear");
    destination.reset();
    retdec_trace("4a9570:after-clear");
    return destination.payload_address();
}
extern "C" int32_t function_4a95c0_this(int32_t object, int32_t source) {
    retdec_trace("4a95c0:begin");
    retdec_trace_i32("4a95c0:this", object);
    retdec_trace_i32("4a95c0:source", source);
    // Snapshot before releasing the old destination, including self-assignment.
    auto incoming = ObjectView(source).value();
    upstream::sqplus_retain(current_vm(), incoming);
    retdec_trace("4a95c0:after-addref");
    ObjectView(object).release(current_vm());
    retdec_trace("4a95c0:after-release");
    ObjectView(object).write(incoming);
    retdec_trace("4a95c0:after-copy");
    return object;
}
extern "C" int32_t function_4a9660_this(int32_t object, int32_t index) {
    retdec_trace("4a9660:this-begin");
    retdec_trace_i32("4a9660:this", object);
    retdec_trace_i32("4a9660:this-index", index);
    return object ? ObjectView(object).capture(current_vm(), index) : 0;
}
extern "C" int32_t function_4a9600_this(int32_t object, int32_t source) {
    const auto receiver = ObjectView(object).value();
    if (receiver._type != OT_ARRAY) return 0;
    auto* vm = current_vm();
    upstream::sqplus_append(vm, receiver, ObjectView(source).value());
    return address(vm); // Retained legacy return, not the upstream void result.
}
extern "C" int32_t* function_4a91c0_this(int32_t* object) {
    function_4a94e0_this(address(object));
    sq_newtable(current_vm());
    function_4a9660_this(address(object), -1);
    pop(current_vm());
    return object;
}
extern "C" int32_t* function_4a92e0_this(int32_t* object, int32_t size) {
    function_4a94e0_this(address(object));
    sq_newarray(current_vm(), size);
    function_4a9660_this(address(object), -1);
    pop(current_vm());
    return object;
}
extern "C" int32_t function_4a96c0_this(int32_t object) {
    return ObjectView(object).value()._type == OT_NULL;
}
extern "C" int32_t function_4a96d0(int32_t object) {
    return kinoko_squirrel_object_size(object, address(current_vm()));
}
extern "C" int32_t function_4a99f0(int32_t object) {
    return kinoko_squirrel_object_reverse(object, address(current_vm()));
}
extern "C" int32_t function_4a9730_this(int32_t object, int32_t key, int32_t text) {
    retdec_trace("4a9730:begin");
    retdec_trace_i32("4a9730:this", object);
    retdec_trace_i32("4a9730:a2", key);
    retdec_trace_i32("4a9730:source", text);
    // key is an SQInteger, not a pointer. The old diagnostic dereferenced it
    // even in quiet builds, making valid small integer keys crash.
    if (object) trace_value("4a9730:this-type", "4a9730:this-data", ObjectView(object));
    auto* vm = current_vm();
    const SQInteger top = sq_gettop(vm);
    retdec_trace_i32("4a9730:stack-base", top);
    ObjectView(object).push(vm);
    retdec_trace("4a9730:after-ab90");
    sq_pushinteger(vm, key);
    retdec_trace("4a9730:after-a4f0");
    sq_pushstring(vm, pointer<const char>(text), -1);
    retdec_trace("4a9730:after-a480");
    const int32_t result = SQ_SUCCEEDED(sq_rawset(vm, -3));
    retdec_trace_i32("4a9730:result", result);
    sq_settop(vm, top);
    retdec_trace("4a9730:after-c910");
    return result;
}
extern "C" int32_t function_4a97b0_this(int32_t object, int32_t key, int32_t value) {
    if (!object || !key || !value) return 0;
    return upstream::sqplus_raw_set(current_vm(), ObjectView(object).value(),
                                   ObjectView(key).value(), ObjectView(value).value());
}
extern "C" int32_t function_4a9840_this(int32_t object, const char* key, int32_t value) {
    return upstream::sqplus_raw_set(current_vm(), ObjectView(object).value(),
                                   key, ObjectView(value).value());
}
extern "C" int32_t function_4a9950(int32_t object, int32_t key, int32_t size, int32_t tag) {
    auto* vm = current_vm();
    const auto top = sq_gettop(vm);
    retdec_trace("4a9950:begin");
    retdec_trace_i32("4a9950:this", object);
    retdec_trace_i32("4a9950:name", key);
    retdec_trace_i32("4a9950:size", size);
    retdec_trace_i32("4a9950:aux", tag);
    retdec_trace_i32("4a9950:stack-before", top);
    if (object) trace_value("4a9950:this-type", "4a9950:this-data", ObjectView(object));
    ObjectView(object).push(vm);
    push_key(vm, pointer<const char>(key));
    sq_newuserdata(vm, size);
    if (tag) sq_settypetag(vm, -1, pointer(tag));
    const int32_t result = SQ_SUCCEEDED(sq_rawset(vm, -3));
    sq_settop(vm, top);
    retdec_trace_i32("4a9950:result", result);
    retdec_trace_i32("4a9950:stack-after", sq_gettop(vm));
    return result;
}
extern "C" int32_t function_4a9a30_this(int32_t object) {
    return object ? ObjectView(object).value()._type : 0;
}
extern "C" int32_t function_4a9a40_this(int32_t object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_get_integer(vm, ObjectView(object).value(), key);
}
extern "C" int32_t function_4a9ac0_this(int32_t object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return address(upstream::sqplus_get_string(vm, ObjectView(object).value(), key));
}
extern "C" int32_t function_4aa000_this(int32_t object, int32_t key) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return address(upstream::sqplus_get_userpointer(vm, ObjectView(object).value(), key));
}
extern "C" int32_t function_4a9b40_this(int32_t object, int32_t tag) {
    if (!object) return 0;
    return address(upstream::sqplus_get_instance_up(current_vm(), ObjectView(object).value(), pointer(tag)));
}
extern "C" int32_t function_4a9bb0_this(int32_t object, int32_t native_pointer) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_set_instance_up(vm, ObjectView(object).value(), pointer(native_pointer));
}
extern "C" int32_t function_4a9c10_this(int32_t object) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    return upstream::sqplus_begin_iteration(vm, ObjectView(object).value());
}
extern "C" int32_t function_4a9c60(int32_t* key, int32_t* value) {
    auto* vm = current_vm();
    if (SQ_FAILED(sq_next(vm, -2))) return 0;
    ObjectView(key).capture(vm, -2);
    ObjectView(value).capture(vm, -1);
    pop(vm, 2); // Leave container and iterator for the next call.
    return 1;
}
extern "C" int32_t function_4a9d50(void) { return pop(current_vm(), 2); }
extern "C" int32_t function_4a9d30_this(int32_t object, int32_t* tag) {
    static int trace_count;
    if (!object || !tag) return 0;
    auto value = ObjectView(object).value();
    const bool trace = trace_count < 128 || value._type == OT_CLASS;
    if (trace) {
        retdec_trace_i32("4a9d30:this", object);
        trace_value("4a9d30:type", "4a9d30:data", ObjectView(object));
        retdec_trace_i32("4a9d30:out", address(tag));
    }
    SQUserPointer native_tag = nullptr;
    const int32_t result = SQ_SUCCEEDED(sq_getobjtypetag(&value, &native_tag));
    if (result) {
        const auto bits = address(native_tag);
        std::memcpy(tag, &bits, sizeof(bits));
    }
    if (trace) {
        retdec_trace_i32("4a9d30:result", result);
        retdec_trace_i32("4a9d30:value", result ? address(native_tag) : 0);
        ++trace_count;
    }
    return result;
}
extern "C" int32_t function_4a9d70_this(int32_t object) {
    return kinoko_squirrel_object_destroy(object, address(current_vm()), kinoko_squirrel_object_vtable());
}
extern "C" int32_t function_4a9e30_this(int32_t object, int32_t thread_address) {
    retdec_trace("4a9e30:begin");
    retdec_trace_i32("4a9e30:this", object);
    retdec_trace_i32("4a9e30:source", thread_address);
    retdec_trace_i32("4a9e30:gvm-before", address(current_vm()));
    if (!object) return 0;
    auto* vm = current_vm();
    ObjectView destination(object);
    if (!thread_address || !vm) {
        if (vm) destination.release(vm); else cannot_release(destination);
        destination.reset();
        return object;
    }
    auto* thread = pointer<SQVM>(thread_address);
    retdec_trace_i32("4a9e30:source-ref-before", thread->_uiRef);
    {
        // Replace manual +4 refcount writes/virtual Release dispatch with
        // the exact temporary lifetime in the supplied SQObjectPtr source.
        SQObjectPtr retained(thread);
        {
            SQObjectPtr pushed(retained);
            if (static_cast<SQUnsignedInteger>(vm->_top) >= vm->_stack.size()) sq_reservestack(vm, 1);
            vm->Push(pushed);
        }
        retdec_trace_i32("4a9e30:gvm-after-push", address(current_vm()));
    }
    retdec_trace_i32("4a9e30:source-ref-after", thread->_uiRef);
    HSQOBJECT result;
    sq_resetobject(&result);
    sq_getstackobj(current_vm(), -1, &result);
    retdec_trace_i32("4a9e30:result-type", result._type);
    retdec_trace_i32("4a9e30:result-data", data_bits(result));
    sq_addref(current_vm(), &result);
    destination.release(current_vm());
    retdec_trace_i32("4a9e30:gvm-after-release", address(current_vm()));
    destination.write(result);
    pop(current_vm());
    retdec_trace_i32("4a9e30:gvm-after-pop", address(current_vm()));
    return object;
}
extern "C" int32_t function_4a9f60(int32_t object, int32_t delegate) {
    const auto type = ObjectView(object).value()._type;
    const auto delegate_type = ObjectView(delegate).value()._type;
    retdec_trace_i32("4a9f60:this-type", type);
    retdec_trace_i32("4a9f60:source-type", delegate_type);
    retdec_trace_i32("4a9f60:this-data", data_bits(ObjectView(object).value()));
    retdec_trace_i32("4a9f60:source-data", data_bits(ObjectView(delegate).value()));
    if ((delegate_type != OT_TABLE && delegate_type != OT_NULL) ||
        (type != OT_TABLE && type != OT_USERDATA)) return 0;
    auto* vm = current_vm();
    ObjectView(object).push(vm);
    ObjectView(delegate).push(vm);
    const auto result = sq_setdelegate(vm, -2);
    retdec_trace_i32("4a9f60:setdelegate-result", result);
    pop(vm);
    return SQ_SUCCEEDED(result);
}
extern "C" int32_t function_4aa080(int32_t object, int32_t key, int32_t output, int32_t tag_output) {
    int32_t result = 0;
    bool converted = false;
    retdec_trace("4aa080:begin");
    retdec_trace_i32("4aa080:this", object);
    retdec_trace_squirrel_name("4aa080:name", key);
    retdec_trace_i32("4aa080:out", output);
    retdec_trace_i32("4aa080:aux", tag_output);
    if (object) trace_value("4aa080:this-type", "4aa080:this-data", ObjectView(object));
    auto* vm = current_vm();
    retdec_trace_i32("4aa080:stack-before", sq_gettop(vm));
    ObjectView(object).push(vm);
    push_key(vm, pointer<const char>(key));
    const auto status = sq_get(vm, -2);
    retdec_trace_i32("4aa080:lookup", status);
    if (SQ_SUCCEEDED(status)) {
        const auto conversion = read_userdata(vm, pointer<int32_t>(output), tag_output);
        converted = SQ_SUCCEEDED(conversion);
        retdec_trace_i32("4aa080:read", conversion);
        pop(vm);
        result = 1;
    }
    pop(vm);
    retdec_trace_i32("4aa080:result", result);
    retdec_trace_i32("4aa080:out-value", converted && output ? *pointer<int32_t>(output) : 0);
    retdec_trace_i32("4aa080:stack-after", sq_gettop(vm));
    return result;
}
extern "C" int32_t retdec_function_4aa110_this(int32_t object, const char* key, int32_t* output, int32_t tag_output) {
    auto* vm = current_vm();
    if (!object || !vm) return 0;
    ObjectView(object).push(vm);
    push_key(vm, key);
    int32_t result = 0;
    if (SQ_SUCCEEDED(sq_rawget(vm, -2))) {
        read_userdata(vm, output, tag_output);
        pop(vm);
        result = 1;
    }
    pop(vm);
    return result;
}
extern "C" int32_t function_4aa1a0(int32_t object, const char* key) {
    return upstream::sqplus_exists(current_vm(), ObjectView(object).value(), key);
}
extern "C" int32_t* function_4aa210_this(int32_t object, int32_t output) {
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    // Snapshot AFTER initialization, preserving the legacy output==receiver case.
    destination.write(upstream::sqplus_get_delegate(current_vm(), ObjectView(object).value()));
    return pointer<int32_t>(output);
}
extern "C" int32_t* function_4aa3a0_this(int32_t object, int32_t output, const char* key) {
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    destination.write(upstream::sqplus_get_value(current_vm(), ObjectView(object).value(), key));
    return pointer<int32_t>(output);
}
extern "C" int32_t function_4a90c0_this(int32_t object, int32_t klass) {
    if (!object) return 0;
    function_4a94e0_this(object);
    auto* vm = current_vm();
    if (!klass || !vm) return object;
    trace_value("4a90c0:source-type", "4a90c0:source-data", ObjectView(klass));
    retdec_trace_i32("4a90c0:vm", address(vm));
    const auto top = sq_gettop(vm);
    retdec_trace_i32("4a90c0:stack-before", top);
    ObjectView(klass).push(vm);
    retdec_trace("4a90c0:after-push");
    if (SQ_FAILED(sq_createinstance(vm, -1))) {
        retdec_trace("4a90c0:instance-get-failed");
        sq_settop(vm, top);
        return object;
    }
    retdec_trace("4a90c0:after-instance-get");
    function_4a9660_this(object, -1);
    retdec_trace("4a90c0:after-copy");
    pop(vm, 2);
    return object;
}
