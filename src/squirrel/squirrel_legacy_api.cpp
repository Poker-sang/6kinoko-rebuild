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
SQObjectPtr* top_slot(SQVM* machine) { return &machine->GetUp(-1); }
}

// No VM opcode, container, compiler, serializer or ref-table implementation
// belongs in this file. All object ownership is implemented by Squirrel.



extern "C" HSQOBJECT* kinoko_sq_add_object_reference(SQVM* machine, HSQOBJECT* object) {
    if (object) sq_addref(machine, object); return object;
}


extern "C" int32_t kinoko_sq_release_object_reference(SQVM* machine, HSQOBJECT* object) {
    return object ? sq_release(machine, object) : SQTrue;
}


extern "C" SQObjectPtr* kinoko_sq_push_string(SQVM* machine, const char* text, int32_t length) {
    sq_pushstring(machine, text, length); return top_slot(machine);
}


extern "C" SQObjectPtr* kinoko_sq_push_integer(SQVM* machine, int32_t a2) {
    sq_pushinteger(machine, a2); return top_slot(machine);
}


extern "C" SQObjectPtr* kinoko_sq_push_user_pointer(SQVM* machine, void* value) {
    sq_pushuserpointer(machine, value); return top_slot(machine);
}


extern "C" SQObjectPtr* kinoko_sq_push_new_table(SQVM* machine) {
    sq_newtable(machine); return top_slot(machine);
}


extern "C" SQObjectPtr* kinoko_sq_push_root_table(SQVM* machine) {
    sq_pushroottable(machine); return top_slot(machine);
}


extern "C" int32_t kinoko_sq_get_type(SQVM* machine, int32_t a2) {
    return sq_gettype(machine, a2);
}


extern "C" int32_t kinoko_sq_get_integer(SQVM* machine, int32_t a2, int32_t * a3) {
    return sq_getinteger(machine, a2, a3);
}


extern "C" int32_t kinoko_sq_get_user_pointer(SQVM* machine, int32_t index, void** output) {
    return sq_getuserpointer(machine, index, output);
}


extern "C" int32_t kinoko_sq_get_stack_top(SQVM* machine) {
    return sq_gettop(machine);
}






extern "C" int32_t kinoko_sq_get_stack_object(SQVM* machine, int32_t index, HSQOBJECT* output) {
    return sq_getstackobj(machine, index, output);
}


extern "C" SQObjectPtr* kinoko_sq_push_raw_object(SQVM* machine, int32_t a2, int32_t a3) {
    HSQOBJECT value; value._type = static_cast<SQObjectType>(a2); std::memcpy(&value._unVal, &a3, sizeof a3); sq_pushobject(machine, value); return top_slot(machine);
}


extern "C" HSQOBJECT* kinoko_sq_reset_object(HSQOBJECT* object) {
    sq_resetobject(object); return object;
}


extern "C" int32_t kinoko_sq_throw_error(SQVM* machine, char * a2) {
    return sq_throwerror(machine, a2);
}


extern "C" SQVM* kinoko_sq_reset_error(SQVM* machine) {
    sq_reseterror(machine); return machine;
}


extern "C" SQObjectPtr* kinoko_sq_get_last_error(SQVM* machine) {
    sq_getlasterror(machine); return top_slot(machine);
}




extern "C" SQVM* kinoko_sq_set_compiler_error_handler(SQVM* machine, SQCOMPILERERROR callback) {
    sq_setcompilererrorhandler(machine, callback); return machine;
}


extern "C" int32_t kinoko_sq_write_closure(SQVM* machine, SQWRITEFUNC writer, SQUserPointer context) {
    return sq_writeclosure(machine, writer, context);
}


extern "C" int32_t kinoko_sq_read_closure(SQVM* machine, SQREADFUNC reader, SQUserPointer context) {
    return sq_readclosure(machine, reader, context);
}


extern "C" int32_t kinoko_sq_collect_garbage(SQVM* machine, int32_t a2, int32_t a3) {
    (void)a2; (void)a3; return machine ? sq_collectgarbage(machine) : 0;
}


extern "C" int32_t kinoko_sq_create_instance(SQVM* machine, int32_t a2) {
    return sq_createinstance(machine, a2);
}


extern "C" SQObjectPtr* kinoko_sq_move_object(SQVM* machine, SQVM* source, int32_t count) {
    sq_move(machine, source, count); return top_slot(machine);
}


extern "C" SQVM* kinoko_sq_set_print_function(SQVM* machine, SQPRINTFUNCTION callback) {
    sq_setprintfunc(machine, callback); return machine;
}


extern "C" int32_t kinoko_sq_get_print_function(SQVM* machine) {
    return addr(reinterpret_cast<const void*>(sq_getprintfunc(machine)));
}


extern "C" int32_t kinoko_sq_compile_lexed(SQVM* machine, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror) {
    return sq_compile(machine, reinterpret_cast<SQLEXREADFUNC>(ptr<void>(reader)), context, ptr<const char>(name), raiseerror != 0);
}


extern "C" int32_t kinoko_sq_new_class(SQVM* machine, int32_t a2) {
    return sq_newclass(machine, a2 != 0);
}


extern "C" int32_t kinoko_sq_set_closure_name(SQVM* machine, int32_t index, const char* name) {
    return sq_setnativeclosurename(machine, index, name);
}


extern "C" int32_t kinoko_sq_set_type_tag(SQVM* machine, int32_t index, void* tag) {
    return sq_settypetag(machine, index, tag);
}


extern "C" int32_t kinoko_sq_set_instance_pointer(SQVM* machine, int32_t index, void* instance) {
    return sq_setinstanceup(machine, index, instance);
}


extern "C" int32_t kinoko_sq_get_instance_pointer(SQVM* machine, int32_t index, void** output, void* tag) {
    return sq_getinstanceup(machine, index, output, tag);
}


extern "C" SQVM* kinoko_sq_set_stack_top(SQVM* machine, uint32_t a2) {
    sq_settop(machine, static_cast<SQInteger>(a2)); return machine;
}


extern "C" int32_t kinoko_sq_get_slot(SQVM* machine, int32_t a2) {
    return sq_get(machine, a2);
}


extern "C" int32_t kinoko_sq_raw_get_slot(SQVM* machine, int32_t a2) {
    return sq_rawget(machine, a2);
}


extern "C" int32_t kinoko_sq_compile_buffer(SQVM* machine, int32_t text, int32_t length, int32_t *name, int32_t raiseerror) {
    return sq_compilebuffer(machine, ptr<const char>(text), length, reinterpret_cast<const char*>(name), raiseerror != 0);
}


extern "C" SQObjectPtr* kinoko_sq_new_closure(SQVM* machine, SQFUNCTION callback, int32_t free_variables) {
    sq_newclosure(machine, callback, free_variables); return top_slot(machine);
}


extern "C" int32_t kinoko_sq_array_pop(SQVM* machine, int32_t index, int32_t push_value) {
    return sq_arraypop(machine, index, push_value != 0);
}


extern "C" SQUserData* kinoko_sq_create_user_data(SQSharedState* shared, int32_t bytes) {
    return SQUserData::Create(shared, bytes);
}


extern "C" int32_t kinoko_sq_register_io_library(SQVM* machine) {
    return sqstd_register_iolib(machine);
}


extern "C" int32_t kinoko_sq_register_blob_library(SQVM* machine) {
    return sqstd_register_bloblib(machine);
}


extern "C" int32_t kinoko_sq_register_math_library(SQVM* machine) {
    return sqstd_register_mathlib(machine);
}


extern "C" int32_t kinoko_sq_register_string_library(SQVM* machine) {
    return sqstd_register_stringlib(machine);
}


extern "C" int32_t kinoko_sq_install_error_handlers(SQVM* machine) {
    sqstd_seterrorhandlers(machine); return SQ_OK;
}


extern "C" int32_t kinoko_sq_set_object_delegate(SQDelegable* receiver, SQTable* delegate) {
    return receiver->SetDelegate(delegate);
}




extern "C" SQObjectPtr* kinoko_sq_destroy_object(SQObjectPtr* value) {
    if (value) value->~SQObjectPtr();
    return value;
}


extern "C" SQObjectPtr* kinoko_sq_assign_object(SQObjectPtr* destination, const SQObjectPtr* source) {
    *destination = *source;
    return destination;
}


extern "C" SQObjectPtr* kinoko_sq_assign_integer(SQObjectPtr* destination, int32_t value) {
    if (destination) *destination = value;
    return destination;
}


extern "C" SQObjectPtr* kinoko_sq_assign_float(SQObjectPtr* destination, float value) {
    if (destination) *destination = value;
    return destination;
}


extern "C" SQVM* kinoko_sq_raise_object_error(SQVM* machine, SQObjectPtr* error_value) {
    if (!machine || !error_value) return machine;
    auto& value = *error_value;
    const auto result = ISREFCOUNTED(type(value)) ? addr(_refcounted(value)) : addr(machine);
    machine->Raise_Error(value);
    return result;
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



extern "C" void kinoko_squirrel_addref(int32_t type, int32_t data) {
    if (ISREFCOUNTED(type) && data) ++ptr<SQRefCounted>(data)->_uiRef;
}

extern "C" void kinoko_squirrel_release(int32_t type, int32_t data) {
    if (ISREFCOUNTED(type) && data) {
        auto* object = ptr<SQRefCounted>(data);
        if (--object->_uiRef == 0) object->Release();
    }
}

extern "C" void kinoko_squirrel_assign(int32_t *dst, const int32_t *src) {
    if (dst && src) *reinterpret_cast<SQObjectPtr*>(dst) = *reinterpret_cast<const SQObjectPtr*>(src);
}

extern "C" void kinoko_release_squirrel_value(int32_t *value_ptr) {
    if (value_ptr) reinterpret_cast<SQObjectPtr*>(value_ptr)->Null();
}

extern "C" int32_t kinoko_gc_object_type(int32_t object_ptr) {
    return kinoko_sq_source_object_type(object_ptr);
}

extern "C" void kinoko_gc_mark_value(const int32_t *value, int32_t *chain_head) {
    kinoko_sq_mark_value(value, chain_head);
}

extern "C" void kinoko_gc_finalize_collectable(int32_t object_ptr,
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
extern "C" void kinoko_sq_finalize_userdata_entry(int32_t object, void* unused) {
    kinoko_sq_finalize_userdata(ptr<SQUserData>(object));
}

extern "C" int32_t kinoko_sq_destroy_userdata_entry(int32_t object, void* unused, int32_t flags) {
    return kinoko_sq_destroy_userdata(ptr<SQUserData>(object), flags);
}

