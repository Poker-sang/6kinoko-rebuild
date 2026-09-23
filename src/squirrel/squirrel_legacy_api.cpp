#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_vm_lifecycle.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqtable.h"
#include "squserdata.h"
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <new>

namespace {
static_assert(sizeof(void*) == 4 && sizeof(SQObjectPtr) == 8);
template<class T> T* ptr(int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
int32_t addr(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
SQVM* vm(int32_t value) noexcept { return ptr<SQVM>(value); }
int32_t top_slot(int32_t value) { return addr(&vm(value)->GetUp(-1)); }
int32_t top_slot(SQVM* machine) { return addr(&machine->GetUp(-1)); }
}

// No VM opcode, container, compiler, serializer or ref-table implementation
// belongs in this file. All object ownership is implemented by Squirrel.

extern "C" int32_t function_48a230(int32_t a1, int32_t a2) {
    return kinoko_sq_create_thread(a1, a2);
}

static int32_t kinoko_sq_add_object_reference(SQVM* machine, int32_t a2) {
    if (a2) sq_addref(machine, ptr<HSQOBJECT>(a2)); return a2;
}
extern "C" int32_t function_48a400(int32_t a1, int32_t a2) {
    return kinoko_sq_add_object_reference(vm(a1), a2);
}

static int32_t kinoko_sq_release_object_reference(SQVM* machine, int32_t a2) {
    return a2 ? sq_release(machine, ptr<HSQOBJECT>(a2)) : SQTrue;
}
extern "C" int32_t function_48a430(int32_t a1, int32_t a2) {
    return kinoko_sq_release_object_reference(vm(a1), a2);
}

static int32_t kinoko_sq_push_string(SQVM* machine, int32_t a2, int32_t a3) {
    sq_pushstring(machine, ptr<const char>(a2), a3); return top_slot(machine);
}
extern "C" int32_t function_48a480(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_push_string(vm(a1), a2, a3);
}

static int32_t kinoko_sq_push_integer(SQVM* machine, int32_t a2) {
    sq_pushinteger(machine, a2); return top_slot(machine);
}
extern "C" int32_t function_48a4f0(int32_t a1, int32_t a2) {
    return kinoko_sq_push_integer(vm(a1), a2);
}

static int32_t kinoko_sq_push_user_pointer(SQVM* machine, int32_t a2) {
    sq_pushuserpointer(machine, ptr<void>(a2)); return top_slot(machine);
}
extern "C" int32_t function_48a5c0(int32_t a1, int32_t a2) {
    return kinoko_sq_push_user_pointer(vm(a1), a2);
}

static int32_t kinoko_sq_push_new_table(SQVM* machine) {
    sq_newtable(machine); return top_slot(machine);
}
extern "C" int32_t function_48a600(int32_t a1) {
    return kinoko_sq_push_new_table(vm(a1));
}

static int32_t kinoko_sq_push_root_table(SQVM* machine) {
    sq_pushroottable(machine); return top_slot(machine);
}
extern "C" int32_t function_48a670(int32_t a1) {
    return kinoko_sq_push_root_table(vm(a1));
}

static int32_t kinoko_sq_get_type(SQVM* machine, int32_t a2) {
    return sq_gettype(machine, a2);
}
extern "C" int32_t function_48a6f0(int32_t a1, int32_t a2) {
    return kinoko_sq_get_type(vm(a1), a2);
}

static int32_t kinoko_sq_get_integer(SQVM* machine, int32_t a2, int32_t * a3) {
    return sq_getinteger(machine, a2, a3);
}
extern "C" int32_t function_48a7d0(int32_t a1, int32_t a2, int32_t * a3) {
    return kinoko_sq_get_integer(vm(a1), a2, a3);
}

static int32_t kinoko_sq_get_user_pointer(SQVM* machine, int32_t a2, int32_t * a3) {
    return sq_getuserpointer(machine, a2, reinterpret_cast<void**>(a3));
}
extern "C" int32_t function_48a9e0(int32_t a1, int32_t a2, int32_t * a3) {
    return kinoko_sq_get_user_pointer(vm(a1), a2, a3);
}

static int32_t kinoko_sq_get_stack_top(SQVM* machine) {
    return sq_gettop(machine);
}
extern "C" int32_t function_48aa20(int32_t a1) {
    return kinoko_sq_get_stack_top(vm(a1));
}

extern "C" int32_t function_48aa30(int32_t a1, int32_t a2) {
    return kinoko_sq_pop(a1, a2);
}

extern "C" int32_t function_48aa50(int32_t a1) {
    return kinoko_sq_pop(a1, 1);
}

static int32_t kinoko_sq_get_stack_object(SQVM* machine, int32_t a2, int32_t * a3) {
    return sq_getstackobj(machine, a2, reinterpret_cast<HSQOBJECT*>(a3));
}
extern "C" int32_t function_48ab40(int32_t a1, int32_t a2, int32_t * a3) {
    return kinoko_sq_get_stack_object(vm(a1), a2, a3);
}

static int32_t kinoko_sq_push_raw_object(SQVM* machine, int32_t a2, int32_t a3) {
    HSQOBJECT value; value._type = static_cast<SQObjectType>(a2); std::memcpy(&value._unVal, &a3, sizeof a3); sq_pushobject(machine, value); return top_slot(machine);
}
extern "C" int32_t function_48ab90(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_push_raw_object(vm(a1), a2, a3);
}

static int32_t kinoko_sq_reset_object(HSQOBJECT* object) {
    sq_resetobject(object); return addr(object);
}
extern "C" int32_t function_48abe0(int32_t result) {
    return kinoko_sq_reset_object(ptr<HSQOBJECT>(result));
}

static int32_t kinoko_sq_throw_error(SQVM* machine, char * a2) {
    return sq_throwerror(machine, a2);
}
extern "C" int32_t function_48ac00(int32_t a1, char * a2) {
    return kinoko_sq_throw_error(vm(a1), a2);
}

static int32_t kinoko_sq_reset_error(SQVM* machine) {
    sq_reseterror(machine); return addr(machine);
}
extern "C" int32_t function_48ac70(int32_t a1) {
    return kinoko_sq_reset_error(vm(a1));
}

static int32_t kinoko_sq_get_last_error(SQVM* machine) {
    sq_getlasterror(machine); return top_slot(machine);
}
extern "C" int32_t function_48acc0(int32_t a1) {
    return kinoko_sq_get_last_error(vm(a1));
}

extern "C" int32_t function_48ace0(int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
    return kinoko_sq_call(a1, a2, a3, a4);
}

static int32_t kinoko_sq_set_compiler_error_handler(SQVM* machine, int32_t a2) {
    sq_setcompilererrorhandler(machine, reinterpret_cast<SQCOMPILERERROR>(ptr<void>(a2))); return addr(machine);
}
extern "C" int32_t function_48afa0(int32_t result, int32_t a2) {
    return kinoko_sq_set_compiler_error_handler(vm(result), a2);
}

static int32_t kinoko_sq_write_closure(SQVM* machine, int32_t a2, int32_t a3) {
    return sq_writeclosure(machine, reinterpret_cast<SQWRITEFUNC>(ptr<void>(a2)), ptr<void>(a3));
}
extern "C" int32_t function_48afc0(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_write_closure(vm(a1), a2, a3);
}

static int32_t kinoko_sq_read_closure(SQVM* machine, int32_t a2, int32_t * a3) {
    return sq_readclosure(machine, reinterpret_cast<SQREADFUNC>(ptr<void>(a2)), a3);
}
extern "C" int32_t function_48b050(int32_t a1, int32_t a2, int32_t * a3) {
    return kinoko_sq_read_closure(vm(a1), a2, a3);
}

static int32_t kinoko_sq_collect_garbage(SQVM* machine, int32_t a2, int32_t a3) {
    (void)a2; (void)a3; return machine ? sq_collectgarbage(machine) : 0;
}
extern "C" int32_t function_48b180(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_collect_garbage(vm(a1), a2, a3);
}

static int32_t kinoko_sq_create_instance(SQVM* machine, int32_t a2) {
    return sq_createinstance(machine, a2);
}
extern "C" int32_t function_48b490(int32_t a1, int32_t a2) {
    return kinoko_sq_create_instance(vm(a1), a2);
}

static int32_t kinoko_sq_move_object(SQVM* machine, int32_t a2, int32_t a3) {
    sq_move(machine, vm(a2), a3); return top_slot(machine);
}
extern "C" int32_t function_48b870(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_move_object(vm(a1), a2, a3);
}

static int32_t kinoko_sq_set_print_function(SQVM* machine, int32_t a2) {
    sq_setprintfunc(machine, reinterpret_cast<SQPRINTFUNCTION>(ptr<void>(a2))); return addr(machine);
}
extern "C" int32_t function_48b8b0(int32_t result, int32_t a2) {
    return kinoko_sq_set_print_function(vm(result), a2);
}

static int32_t kinoko_sq_get_print_function(SQVM* machine) {
    return addr(reinterpret_cast<const void*>(sq_getprintfunc(machine)));
}
extern "C" int32_t function_48b8d0(int32_t a1) {
    return kinoko_sq_get_print_function(vm(a1));
}

static int32_t kinoko_sq_compile_lexed(SQVM* machine, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror) {
    return sq_compile(machine, reinterpret_cast<SQLEXREADFUNC>(ptr<void>(reader)), context, ptr<const char>(name), raiseerror != 0);
}
extern "C" int32_t function_48c1f0(int32_t vm, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror) {
    return kinoko_sq_compile_lexed(::vm(vm), reader, context, name, raiseerror);
}

static int32_t kinoko_sq_new_class(SQVM* machine, int32_t a2) {
    return sq_newclass(machine, a2 != 0);
}
extern "C" int32_t function_48c350(int32_t a1, int32_t a2) {
    return kinoko_sq_new_class(vm(a1), a2);
}

static int32_t kinoko_sq_set_closure_name(SQVM* machine, int32_t index, int32_t name) {
    return sq_setnativeclosurename(machine, index, ptr<const char>(name));
}
extern "C" int32_t function_48c580(int32_t vm, int32_t index, int32_t name) {
    return kinoko_sq_set_closure_name(::vm(vm), index, name);
}

static int32_t kinoko_sq_set_type_tag(SQVM* machine, int32_t a2, int32_t a3) {
    return sq_settypetag(machine, a2, ptr<void>(a3));
}
extern "C" int32_t function_48c780(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_set_type_tag(vm(a1), a2, a3);
}

static int32_t kinoko_sq_set_instance_pointer(SQVM* machine, int32_t a2, int32_t a3) {
    return sq_setinstanceup(machine, a2, ptr<void>(a3));
}
extern "C" int32_t function_48c840(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_set_instance_pointer(vm(a1), a2, a3);
}

static int32_t kinoko_sq_get_instance_pointer(SQVM* machine, int32_t a2, int32_t * a3, int32_t a4) {
    return sq_getinstanceup(machine, a2, reinterpret_cast<void**>(a3), ptr<void>(a4));
}
extern "C" int32_t function_48c890(int32_t a1, int32_t a2, int32_t * a3, int32_t a4) {
    return kinoko_sq_get_instance_pointer(vm(a1), a2, a3, a4);
}

static int32_t kinoko_sq_set_stack_top(SQVM* machine, uint32_t a2) {
    sq_settop(machine, static_cast<SQInteger>(a2)); return addr(machine);
}
extern "C" int32_t function_48c910(int32_t a1, uint32_t a2) {
    return kinoko_sq_set_stack_top(vm(a1), a2);
}

static int32_t kinoko_sq_get_slot(SQVM* machine, int32_t a2) {
    return sq_get(machine, a2);
}
extern "C" int32_t function_48ce00(int32_t a1, int32_t a2) {
    return kinoko_sq_get_slot(vm(a1), a2);
}

static int32_t kinoko_sq_raw_get_slot(SQVM* machine, int32_t a2) {
    return sq_rawget(machine, a2);
}
extern "C" int32_t function_48ce70(int32_t a1, int32_t a2) {
    return kinoko_sq_raw_get_slot(vm(a1), a2);
}

static int32_t kinoko_sq_compile_buffer(SQVM* machine, int32_t text, int32_t length, int32_t *name, int32_t raiseerror) {
    return sq_compilebuffer(machine, ptr<const char>(text), length, reinterpret_cast<const char*>(name), raiseerror != 0);
}
extern "C" int32_t function_48d0b0(int32_t vm, int32_t text, int32_t length, int32_t *name, int32_t raiseerror) {
    return kinoko_sq_compile_buffer(::vm(vm), text, length, name, raiseerror);
}

static int32_t kinoko_sq_new_closure(SQVM* machine, int32_t a2, int32_t a3) {
    sq_newclosure(machine, reinterpret_cast<SQFUNCTION>(ptr<void>(a2)), a3); return top_slot(machine);
}
extern "C" int32_t function_48d850(int32_t a1, int32_t a2, int32_t a3) {
    return kinoko_sq_new_closure(vm(a1), a2, a3);
}

static int32_t kinoko_sq_array_pop(SQVM* machine, int32_t index, int32_t push_value) {
    return sq_arraypop(machine, index, push_value != 0);
}
extern "C" int32_t function_48dd10(int32_t vm, int32_t index, int32_t push_value) {
    return kinoko_sq_array_pop(::vm(vm), index, push_value);
}

static SQUserData* kinoko_sq_create_user_data(SQSharedState* shared, int32_t bytes) {
    return SQUserData::Create(shared, bytes);
}
extern "C" int32_t function_48bec0(int32_t shared_state, int32_t bytes) {
    return addr(kinoko_sq_create_user_data(ptr<SQSharedState>(shared_state), bytes));
}

static int32_t kinoko_sq_register_io_library(SQVM* machine) {
    return sqstd_register_iolib(machine);
}
extern "C" int32_t function_4c7c90(int32_t a1) {
    return kinoko_sq_register_io_library(vm(a1));
}

static int32_t kinoko_sq_register_blob_library(SQVM* machine) {
    return sqstd_register_bloblib(machine);
}
extern "C" int32_t function_4c73a0(int32_t a1) {
    return kinoko_sq_register_blob_library(vm(a1));
}

static int32_t kinoko_sq_register_math_library(SQVM* machine) {
    return sqstd_register_mathlib(machine);
}
extern "C" int32_t function_4c6c20(int32_t a1) {
    return kinoko_sq_register_math_library(vm(a1));
}

static int32_t kinoko_sq_register_string_library(SQVM* machine) {
    return sqstd_register_stringlib(machine);
}
extern "C" int32_t function_4c6670(int32_t a1) {
    return kinoko_sq_register_string_library(vm(a1));
}

static int32_t kinoko_sq_install_error_handlers(SQVM* machine) {
    sqstd_seterrorhandlers(machine); return SQ_OK;
}
extern "C" int32_t function_4c5c80(int32_t vm) {
    return kinoko_sq_install_error_handlers(::vm(vm));
}

static int32_t kinoko_sq_set_object_delegate(SQDelegable* receiver, SQTable* delegate) {
    return receiver->SetDelegate(delegate);
}
extern "C" int32_t function_48e520_this(int32_t receiver, int32_t delegate) {
    return kinoko_sq_set_object_delegate(ptr<SQDelegable>(receiver), ptr<SQTable>(delegate));
}

extern "C" int32_t function_491880_this(int32_t this_ptr, int32_t index) {
    return kinoko_sq_get_up(this_ptr, index);
}

static int32_t kinoko_sq_destroy_object(SQObjectPtr* value) {
    if (value) value->~SQObjectPtr();
    return addr(value);
}
extern "C" int32_t function_489f30_this(int32_t value) {
    return kinoko_sq_destroy_object(ptr<SQObjectPtr>(value));
}

static int32_t kinoko_sq_assign_object(SQObjectPtr* destination, const SQObjectPtr* source) {
    *destination = *source;
    return addr(destination);
}
extern "C" int32_t function_489f50_this(int32_t destination, int32_t source) {
    return kinoko_sq_assign_object(ptr<SQObjectPtr>(destination), ptr<SQObjectPtr>(source));
}

static int32_t kinoko_sq_assign_integer(SQObjectPtr* destination, int32_t value) {
    if (destination) *destination = value;
    return addr(destination);
}
extern "C" int32_t function_48e0e0_this(int32_t destination, int32_t value) {
    return kinoko_sq_assign_integer(ptr<SQObjectPtr>(destination), value);
}

static int32_t kinoko_sq_assign_float(SQObjectPtr* destination, float value) {
    if (destination) *destination = value;
    return addr(destination);
}
extern "C" int32_t function_48e120_this(int32_t destination, float value) {
    return kinoko_sq_assign_float(ptr<SQObjectPtr>(destination), value);
}

static int32_t kinoko_sq_raise_object_error(SQVM* machine, SQObjectPtr* error_value) {
    if (!machine || !error_value) return addr(machine);
    auto& value = *error_value;
    const auto result = ISREFCOUNTED(type(value)) ? addr(_refcounted(value)) : addr(machine);
    machine->Raise_Error(value);
    return result;
}
extern "C" int32_t function_499b00(int32_t machine, int32_t value) {
    return kinoko_sq_raise_object_error(vm(machine), ptr<SQObjectPtr>(value));
}

namespace {
int32_t raise_formatted_error(HSQUIRRELVM machine, const char* format, va_list args) {
    if (!machine || !format) return SQ_ERROR;
    va_list measure; va_copy(measure, args);
    const int size = std::vsnprintf(nullptr, 0, format, measure); va_end(measure);
    if (size < 0) return SQ_ERROR;
    std::vector<char> text;
    try {
        text.resize(static_cast<size_t>(size) + 1);
    } catch (const std::bad_alloc&) {
        return sq_throwerror(machine, "out of memory formatting error");
    }
    std::vsnprintf(text.data(), text.size(), format, args);
    sq_throwerror(machine, text.data());
    return SQ_OK;
}
} // namespace
extern "C" int32_t kinoko_sq_raise_formatted_error(int32_t receiver, const char *format, ...) {
    if (!receiver || !format) return SQ_ERROR;
    va_list args; va_start(args, format);
    const int32_t result = raise_formatted_error(vm(receiver), format, args);
    va_end(args);
    return result;
}

extern "C" int32_t function_49a520_this(int32_t shared_state, int32_t vm) {
    return shared_state && vm ? kinoko_sq_collect(shared_state, vm) : 0;
}

extern "C" void retdec_squirrel_addref(int32_t type, int32_t data) {
    if (ISREFCOUNTED(type) && data) ++ptr<SQRefCounted>(data)->_uiRef;
}

extern "C" void retdec_squirrel_release(int32_t type, int32_t data) {
    if (ISREFCOUNTED(type) && data) {
        auto* object = ptr<SQRefCounted>(data);
        if (--object->_uiRef == 0) object->Release();
    }
}

extern "C" void retdec_squirrel_assign(int32_t *dst, const int32_t *src) {
    if (dst && src) *reinterpret_cast<SQObjectPtr*>(dst) = *reinterpret_cast<const SQObjectPtr*>(src);
}

extern "C" void retdec_release_squirrel_value(int32_t *value_ptr) {
    if (value_ptr) reinterpret_cast<SQObjectPtr*>(value_ptr)->Null();
}

extern "C" int32_t retdec_gc_object_type(int32_t object_ptr) {
    return kinoko_sq_source_object_type(object_ptr);
}

extern "C" void retdec_gc_mark_value(const int32_t *value, int32_t *chain_head) {
    kinoko_sq_mark_value(value, chain_head);
}

extern "C" void retdec_gc_finalize_collectable(int32_t object_ptr,
                                            int32_t object_type) {
    kinoko_sq_finalize_object(object_ptr, object_type);
}

static void kinoko_sq_finalize_userdata(SQUserData* data) {
    data->Finalize();
}
static int32_t kinoko_sq_destroy_userdata(SQUserData* data, int32_t flags) {
    const auto size = sizeof(SQUserData) + data->_size - 1;
    data->~SQUserData();
    if (flags & 1) sq_vm_free(data, size);
    return addr(data);
}
extern "C" void __fastcall function_48be70(int32_t object, void* unused) {
    kinoko_sq_finalize_userdata(ptr<SQUserData>(object));
}
extern "C" int32_t __fastcall function_48bf50(int32_t object, void* unused, int32_t flags) {
    return kinoko_sq_destroy_userdata(ptr<SQUserData>(object), flags);
}
