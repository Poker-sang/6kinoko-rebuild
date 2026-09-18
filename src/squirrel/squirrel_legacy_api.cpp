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
}

// No VM opcode, container, compiler, serializer or ref-table implementation
// belongs in this file. All object ownership is implemented by Squirrel.

extern "C" int32_t function_48a230(int32_t a1, int32_t a2) {
    return kinoko_sq_create_thread(a1, a2);
}

extern "C" int32_t function_48a2d0(int32_t a1) {
    return sq_getvmstate(vm(a1));
}

extern "C" int32_t function_48a300(int32_t a1) {
    sq_seterrorhandler(vm(a1)); return a1;
}

extern "C" int32_t function_48a370(int32_t a1) {
    sq_setdebughook(vm(a1)); return a1;
}

extern "C" int32_t function_48a3e0(int32_t a1, int32_t a2) {
    sq_enabledebuginfo(vm(a1), a2 != 0); return a1;
}

extern "C" int32_t function_48a400(int32_t a1, int32_t a2) {
    if (a2) sq_addref(vm(a1), ptr<HSQOBJECT>(a2)); return a2;
}

extern "C" int32_t function_48a430(int32_t a1, int32_t a2) {
    return a2 ? sq_release(vm(a1), ptr<HSQOBJECT>(a2)) : SQTrue;
}

extern "C" int32_t function_48a460(int32_t a1) {
    sq_pushnull(vm(a1)); return top_slot(a1);
}

extern "C" int32_t function_48a480(int32_t a1, int32_t a2, int32_t a3) {
    sq_pushstring(vm(a1), ptr<const char>(a2), a3); return top_slot(a1);
}

extern "C" int32_t function_48a4f0(int32_t a1, int32_t a2) {
    sq_pushinteger(vm(a1), a2); return top_slot(a1);
}

extern "C" int32_t function_48a530(int32_t a1, int32_t a2) {
    sq_pushbool(vm(a1), a2 != 0); return top_slot(a1);
}

extern "C" int32_t function_48a580(int32_t a1, int32_t a2) {
    float value; std::memcpy(&value, &a2, sizeof value); sq_pushfloat(vm(a1), value); return top_slot(a1);
}

extern "C" int32_t function_48a5c0(int32_t a1, int32_t a2) {
    sq_pushuserpointer(vm(a1), ptr<void>(a2)); return top_slot(a1);
}

extern "C" int32_t function_48a600(int32_t a1) {
    sq_newtable(vm(a1)); return top_slot(a1);
}

extern "C" int32_t function_48a670(int32_t a1) {
    sq_pushroottable(vm(a1)); return top_slot(a1);
}

extern "C" int32_t function_48a690(int32_t a1) {
    sq_pushregistrytable(vm(a1)); return top_slot(a1);
}

extern "C" int32_t function_48a6b0(int32_t a1, int32_t a2) {
    sq_push(vm(a1), a2); return top_slot(a1);
}

extern "C" int32_t function_48a6f0(int32_t a1, int32_t a2) {
    return sq_gettype(vm(a1), a2);
}

extern "C" int32_t function_48a720(int32_t a1, int32_t a2) {
    sq_tostring(vm(a1), a2); return top_slot(a1);
}

extern "C" int32_t function_48a790(int32_t a1, int32_t a2, int32_t * a3) {
    sq_tobool(vm(a1), a2, reinterpret_cast<SQBool*>(a3)); return SQ_OK;
}

extern "C" int32_t function_48a7d0(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getinteger(vm(a1), a2, a3);
}

extern "C" int32_t function_48a830(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getfloat(vm(a1), a2, reinterpret_cast<float*>(a3));
}

extern "C" int32_t function_48a890(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getbool(vm(a1), a2, reinterpret_cast<SQBool*>(a3));
}

extern "C" int32_t function_48a8d0(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getstring(vm(a1), a2, reinterpret_cast<const char**>(a3));
}

extern "C" int32_t function_48a920(int32_t a1, int32_t a2, int32_t * a3, int32_t a4) {
    return sq_getuserdata(vm(a1), a2, reinterpret_cast<void**>(a3), ptr<void*>(a4));
}

extern "C" int32_t function_48a980(int32_t a1, int32_t a2) {
    return sq_getobjtypetag(ptr<HSQOBJECT>(a1), ptr<void*>(a2));
}

extern "C" int32_t function_48a9e0(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getuserpointer(vm(a1), a2, reinterpret_cast<void**>(a3));
}

extern "C" int32_t function_48aa20(int32_t a1) {
    return sq_gettop(vm(a1));
}

extern "C" int32_t function_48aa30(int32_t a1, int32_t a2) {
    return kinoko_sq_pop(a1, a2);
}

extern "C" int32_t function_48aa50(int32_t a1) {
    return kinoko_sq_pop(a1, 1);
}

extern "C" int32_t function_48aa60(int32_t a1, int32_t a2) {
    sq_remove(vm(a1), a2); return a1;
}

extern "C" int32_t function_48aa80(int32_t a1, int32_t a2, int32_t a3) {
    return sq_rawdeleteslot(vm(a1), a2, a3 != 0);
}

extern "C" int32_t function_48ab40(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_getstackobj(vm(a1), a2, reinterpret_cast<HSQOBJECT*>(a3));
}

extern "C" int32_t function_48ab90(int32_t a1, int32_t a2, int32_t a3) {
    HSQOBJECT value; value._type = static_cast<SQObjectType>(a2); std::memcpy(&value._unVal, &a3, sizeof a3); sq_pushobject(vm(a1), value); return top_slot(a1);
}

extern "C" int32_t function_48abe0(int32_t result) {
    sq_resetobject(ptr<HSQOBJECT>(result)); return result;
}

extern "C" int32_t function_48ac00(int32_t a1, char * a2) {
    return sq_throwerror(vm(a1), a2);
}

extern "C" int32_t function_48ac70(int32_t a1) {
    sq_reseterror(vm(a1)); return a1;
}

extern "C" int32_t function_48acc0(int32_t a1) {
    sq_getlasterror(vm(a1)); return top_slot(a1);
}

extern "C" int32_t function_48ace0(int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
    return kinoko_sq_call(a1, a2, a3, a4);
}

extern "C" int32_t function_48ada0(int32_t a1) {
    return sq_suspendvm(vm(a1));
}

extern "C" int32_t function_48adb0(int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
    return sq_wakeupvm(vm(a1), a2 != 0, a3 != 0, a4 != 0);
}

extern "C" int32_t function_48af30(int32_t a1, int32_t a2, int32_t result2) {
    sq_setreleasehook(vm(a1), a2, reinterpret_cast<SQRELEASEHOOK>(ptr<void>(result2))); return result2;
}

extern "C" int32_t function_48afa0(int32_t result, int32_t a2) {
    sq_setcompilererrorhandler(vm(result), reinterpret_cast<SQCOMPILERERROR>(ptr<void>(a2))); return result;
}

extern "C" int32_t function_48afc0(int32_t a1, int32_t a2, int32_t a3) {
    return sq_writeclosure(vm(a1), reinterpret_cast<SQWRITEFUNC>(ptr<void>(a2)), ptr<void>(a3));
}

extern "C" int32_t function_48b050(int32_t a1, int32_t a2, int32_t * a3) {
    return sq_readclosure(vm(a1), reinterpret_cast<SQREADFUNC>(ptr<void>(a2)), a3);
}

extern "C" int32_t function_48b160(int32_t a1, int32_t a2) {
    return addr(sq_getscratchpad(vm(a1), a2));
}

extern "C" int32_t function_48b180(int32_t a1, int32_t a2, int32_t a3) {
    (void)a2; (void)a3; return a1 ? sq_collectgarbage(vm(a1)) : 0;
}

extern "C" int32_t function_48b1a0(int32_t a1, int32_t a2) {
    return sq_setattributes(vm(a1), a2);
}

extern "C" int32_t function_48b320(int32_t a1, int32_t a2) {
    return sq_getattributes(vm(a1), a2);
}

extern "C" int32_t function_48b410(int32_t a1, int32_t a2) {
    return sq_getclass(vm(a1), a2);
}

extern "C" int32_t function_48b490(int32_t a1, int32_t a2) {
    return sq_createinstance(vm(a1), a2);
}

extern "C" int32_t function_48b510(int32_t a1, int32_t a2) {
    sq_weakref(vm(a1), a2); return top_slot(a1);
}

extern "C" int32_t function_48b5a0(int32_t a1, int32_t a2) {
    return sq_getweakrefval(vm(a1), a2);
}

extern "C" int32_t function_48b630(int32_t a1, int32_t a2) {
    return sq_next(vm(a1), a2);
}

extern "C" int32_t function_48b870(int32_t a1, int32_t a2, int32_t a3) {
    sq_move(vm(a1), vm(a2), a3); return top_slot(a1);
}

extern "C" int32_t function_48b8b0(int32_t result, int32_t a2) {
    sq_setprintfunc(vm(result), reinterpret_cast<SQPRINTFUNCTION>(ptr<void>(a2))); return result;
}

extern "C" int32_t function_48b8d0(int32_t a1) {
    return addr(reinterpret_cast<const void*>(sq_getprintfunc(vm(a1))));
}

extern "C" int32_t function_48b8f0(int32_t a1) {
    return addr(sq_malloc(static_cast<SQUnsignedInteger>(a1)));
}

extern "C" int32_t function_48c1c0(int32_t a1) {
    sq_close(vm(a1)); return 0;
}

extern "C" int32_t function_48c1f0(int32_t vm, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror) {
    return sq_compile(::vm(vm), reinterpret_cast<SQLEXREADFUNC>(ptr<void>(reader)), context, ptr<const char>(name), raiseerror != 0);
}

extern "C" int32_t function_48c2f0(int32_t a1, int32_t a2) {
    return addr(sq_newuserdata(vm(a1), a2));
}

extern "C" int32_t function_48c350(int32_t a1, int32_t a2) {
    return sq_newclass(vm(a1), a2 != 0);
}

extern "C" int32_t function_48c400(int32_t a1, int32_t a2) {
    return sq_arrayreverse(vm(a1), a2);
}

extern "C" int32_t function_48c580(int32_t vm, int32_t index, int32_t name) {
    return sq_setnativeclosurename(::vm(vm), index, ptr<const char>(name));
}

extern "C" int32_t function_48c620(int32_t a1) {
    return sq_setroottable(vm(a1));
}

extern "C" int32_t function_48c690(int32_t a1) {
    return sq_setconsttable(vm(a1));
}

extern "C" int32_t function_48c700(int32_t a1, int32_t a2) {
    return sq_getsize(vm(a1), a2);
}

extern "C" int32_t function_48c780(int32_t a1, int32_t a2, int32_t a3) {
    return sq_settypetag(vm(a1), a2, ptr<void>(a3));
}

extern "C" int32_t function_48c7f0(int32_t a1, int32_t a2, int32_t a3) {
    return sq_gettypetag(vm(a1), a2, ptr<void*>(a3));
}

extern "C" int32_t function_48c840(int32_t a1, int32_t a2, int32_t a3) {
    return sq_setinstanceup(vm(a1), a2, ptr<void>(a3));
}

extern "C" int32_t function_48c890(int32_t a1, int32_t a2, int32_t * a3, int32_t a4) {
    return sq_getinstanceup(vm(a1), a2, reinterpret_cast<void**>(a3), ptr<void>(a4));
}

extern "C" int32_t function_48c910(int32_t a1, uint32_t a2) {
    sq_settop(vm(a1), static_cast<SQInteger>(a2)); return a1;
}

extern "C" int32_t function_48c950(int32_t a1, int32_t a2, int32_t a3) {
    return sq_newslot(vm(a1), a2, a3 != 0);
}

extern "C" int32_t function_48ca10(int32_t a1, int32_t a2, int32_t a3) {
    return sq_deleteslot(vm(a1), a2, a3 != 0);
}

extern "C" int32_t function_48cb10(int32_t a1, int32_t a2) {
    return sq_rawset(vm(a1), a2);
}

extern "C" int32_t function_48cc70(int32_t a1, int32_t a2) {
    return sq_setdelegate(vm(a1), a2);
}

extern "C" int32_t function_48cd50(int32_t a1, int32_t a2) {
    return sq_getdelegate(vm(a1), a2);
}

extern "C" int32_t function_48ce00(int32_t a1, int32_t a2) {
    return sq_get(vm(a1), a2);
}

extern "C" int32_t function_48ce70(int32_t a1, int32_t a2) {
    return sq_rawget(vm(a1), a2);
}

extern "C" int32_t function_48cfa0(int32_t vm, uint32_t level, int32_t index) {
    return addr(sq_getlocal(::vm(vm), level, index));
}

extern "C" int32_t function_48d0b0(int32_t vm, int32_t text, int32_t length, int32_t *name, int32_t raiseerror) {
    return sq_compilebuffer(::vm(vm), ptr<const char>(text), length, reinterpret_cast<const char*>(name), raiseerror != 0);
}

extern "C" int32_t function_48d770(int32_t a1, int32_t a2) {
    sq_newarray(vm(a1), a2); return top_slot(a1);
}

extern "C" int32_t function_48d7e0(int32_t a1, int32_t a2) {
    return sq_arrayappend(vm(a1), a2);
}

extern "C" int32_t function_48d850(int32_t a1, int32_t a2, int32_t a3) {
    sq_newclosure(vm(a1), reinterpret_cast<SQFUNCTION>(ptr<void>(a2)), a3); return top_slot(a1);
}

extern "C" int32_t function_48dd10(int32_t vm, int32_t index, int32_t push_value) {
    return sq_arraypop(::vm(vm), index, push_value != 0);
}

extern "C" int32_t function_48dda0(int32_t a1, int32_t a2, int32_t a3) {
    return sq_setparamscheck(vm(a1), a2, ptr<const char>(a3));
}

extern "C" int32_t function_48bb30(int32_t shared_state, int32_t proto) {
    return addr(SQClosure::Create(ptr<SQSharedState>(shared_state), ptr<SQFunctionProto>(proto)));
}

extern "C" int32_t function_48bec0(int32_t a1, int32_t a2) {
    return addr(SQUserData::Create(ptr<SQSharedState>(a1), a2));
}

extern "C" int32_t function_4c7c90(int32_t a1) {
    return sqstd_register_iolib(vm(a1));
}

extern "C" int32_t function_4c73a0(int32_t a1) {
    return sqstd_register_bloblib(vm(a1));
}

extern "C" int32_t function_4c6c20(int32_t a1) {
    return sqstd_register_mathlib(vm(a1));
}

extern "C" int32_t function_4c6670(int32_t a1) {
    return sqstd_register_stringlib(vm(a1));
}

extern "C" int32_t function_4c5c80(int32_t vm) {
    sqstd_seterrorhandlers(::vm(vm)); return SQ_OK;
}

extern "C" int32_t function_49c350_this(int32_t this_ptr) {
    ptr<SQSharedState>(this_ptr)->~SQSharedState(); return this_ptr;
}

extern "C" int32_t function_48e520_this(int32_t this_ptr, int32_t delegate_ptr) {
    return ptr<SQDelegable>(this_ptr)->SetDelegate(ptr<SQTable>(delegate_ptr));
}

extern "C" int32_t function_491820_this(int32_t this_ptr, int32_t value_ptr) {
    auto* machine = vm(this_ptr);
    // Preserve the recovered caller's stack reservation at this ABI boundary.
    // Retain a value before growth: value_ptr may alias the current stack.
    SQObjectPtr value = *ptr<SQObjectPtr>(value_ptr);
    if (machine->_top >= machine->_stack.size()) sq_reservestack(machine, 1);
    machine->Push(value); return top_slot(this_ptr);
}

extern "C" int32_t function_491880_this(int32_t this_ptr, int32_t index) {
    return kinoko_sq_get_up(this_ptr, index);
}

extern "C" int32_t function_4918a0_this(int32_t this_ptr, int32_t index) {
    return kinoko_sq_get_at(this_ptr, index);
}

extern "C" int32_t function_497a00_this(int32_t table_ptr, int32_t *key_ptr,
                                    int32_t out_ptr) {
    if (!table_ptr || !key_ptr || !out_ptr) return 0;
    return ptr<SQTable>(table_ptr)->Get(*reinterpret_cast<const SQObjectPtr*>(key_ptr), *ptr<SQObjectPtr>(out_ptr));
}

extern "C" int32_t function_499970(int32_t vm, uint32_t level, int32_t *info) {
    return sq_stackinfos(::vm(vm), static_cast<SQInteger>(level), reinterpret_cast<SQStackInfos*>(info));
}

extern "C" int32_t function_491760_this(int32_t this_ptr) {
    if (!this_ptr || vm(this_ptr)->_top <= 0) return 0;
    const auto result = top_slot(this_ptr); vm(this_ptr)->Pop(); return result;
}

extern "C" int32_t function_489f30_this(int32_t this_ptr) {
    if (this_ptr) ptr<SQObjectPtr>(this_ptr)->~SQObjectPtr(); return this_ptr;
}

extern "C" int32_t function_489f50_this(int32_t this_ptr, int32_t source_ptr) {
    *ptr<SQObjectPtr>(this_ptr) = *ptr<SQObjectPtr>(source_ptr); return this_ptr;
}

extern "C" int32_t function_48e0e0_this(int32_t this_ptr, int32_t value) {
    if (this_ptr) *ptr<SQObjectPtr>(this_ptr) = value; return this_ptr;
}

extern "C" int32_t function_48e120_this(int32_t this_ptr, float value) {
    if (this_ptr) *ptr<SQObjectPtr>(this_ptr) = value; return this_ptr;
}

extern "C" int32_t function_48e4d0_this(int32_t this_ptr) {
    if (!this_ptr) return 0;
    auto* object = ptr<SQRefCounted>(this_ptr); auto weak = addr(object->_weakref);
    object->SQRefCounted::~SQRefCounted(); return weak;
}

extern "C" int32_t function_499b00(int32_t this_ptr, int32_t value_ptr) {
    if (!this_ptr || !value_ptr) return this_ptr;
    auto& value = *ptr<SQObjectPtr>(value_ptr);
    const auto result = ISREFCOUNTED(type(value)) ? addr(_refcounted(value)) : this_ptr;
    vm(this_ptr)->Raise_Error(value); return result;
}

extern "C" int32_t function_499a20(int32_t this_ptr, const char *format, ...) {
    if (!this_ptr || !format) return SQ_ERROR;
    va_list args; va_start(args, format);
    va_list measure; va_copy(measure, args);
    const int size = std::vsnprintf(nullptr, 0, format, measure); va_end(measure);
    if (size < 0) { va_end(args); return SQ_ERROR; }
    std::vector<char> text;
    try {
        text.resize(static_cast<size_t>(size) + 1);
    } catch (const std::bad_alloc&) {
        va_end(args);
        return sq_throwerror(vm(this_ptr), "out of memory formatting error");
    }
    std::vsnprintf(text.data(), text.size(), format, args);
    va_end(args);
    sq_throwerror(vm(this_ptr), text.data());
    return SQ_OK;
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

extern "C" void __fastcall function_48be70(int32_t object, void* unused) {
    (void)unused; ptr<SQUserData>(object)->Finalize();
}

extern "C" int32_t __fastcall function_48bf50(int32_t object, void* unused, int32_t flags) {
    (void)unused;
    auto* data = ptr<SQUserData>(object); const auto size = sizeof(SQUserData) + data->_size - 1;
    data->~SQUserData(); if (flags & 1) sq_vm_free(data, size); return object;
}
