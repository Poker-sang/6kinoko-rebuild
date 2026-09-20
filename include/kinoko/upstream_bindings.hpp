#pragma once
#include <squirrel.h>
#include <array>
#include "kinoko/squirrel_variable_record.hpp"

// Values cross this boundary; neither the game's unaligned legacy records nor
// internal SQObjectPtr references are overlaid with an upstream C++ object.
// Each *_retain/new/assign result owns exactly one external VM reference.
namespace kinoko::script::upstream {
// Scalar values use upstream getVar/setVar, with aligned temporary storage.
// immediate_value is used only for the host's Constant representation.
SQInteger sqplus_read_scalar(HSQUIRRELVM vm, const binding::Variable& metadata,
                            const void* storage, int32_t immediate_value);
SQInteger sqplus_write_scalar(HSQUIRRELVM vm, const binding::Variable& metadata,
                             void* storage);
void sqplus_variable_metadata(HSQUIRRELVM vm, HSQOBJECT root,
    const binding::Variable& fields, void* output);
std::array<char, 258> sqplus_variable_key(const SQChar* name) noexcept;
bool sqplus_bind_function(HSQUIRRELVM vm, SQFUNCTION function, const SQChar* name,
                           const SQChar* mask, void (*capture)(void*, HSQOBJECT), void* context);
void sqplus_setup_hierarchy(HSQUIRRELVM vm, HSQOBJECT owned_class);
bool sqplus_native_instance(HSQUIRRELVM vm, const SQChar* name, SQUserPointer native,
                            SQRELEASEHOOK hook, SQUserPointer native_type);
bool sqplus_new_instance(HSQUIRRELVM vm, HSQOBJECT klass, HSQOBJECT& output);
HSQOBJECT sqplus_new_table(HSQUIRRELVM vm);
HSQOBJECT sqplus_new_array(HSQUIRRELVM vm, int size);
bool sqplus_set_string(HSQUIRRELVM vm, HSQOBJECT receiver, int key, const SQChar* value);
bool sqplus_new_userdata(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key,
                          int size, SQUserPointer tag);
bool sqplus_get_userdata(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key,
                          SQUserPointer* data, SQUserPointer* tag, bool raw);
bool sqplus_get_typetag(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer* tag);
bool sqplus_set_delegate(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT delegate);
HSQOBJECT sqplus_new_string(HSQUIRRELVM vm, const SQChar* text);
HSQOBJECT sqplus_new_closure(HSQUIRRELVM vm, SQFUNCTION native);
HSQOBJECT sqplus_assign(HSQUIRRELVM vm, HSQOBJECT previous, HSQOBJECT incoming);
// The caller supplies a valid stack index, as in Squirrel 2.2.2's API.
HSQOBJECT sqplus_capture(HSQUIRRELVM vm, HSQOBJECT previous, int index);
void sqplus_retain(HSQUIRRELVM vm, HSQOBJECT value);
void sqplus_release(HSQUIRRELVM vm, HSQOBJECT value);
bool sqplus_create_class(HSQUIRRELVM vm, HSQOBJECT& output, SQUserPointer tag,
                         const SQChar* name, const SQChar* parent);
// Borrowed receiver/arguments. Only GetValue/GetDelegate/GetSlot results own
// a new external reference; returned native pointers/strings remain borrowed.
// Select the declaring base via the snapshot's typetag/__ot branch. This
// excludes field offsets, static/constants and legacy metadata representation.
bool sqplus_instance_base(HSQUIRRELVM vm, HSQOBJECT receiver,
                          SQUserPointer declaring_type, SQUserPointer& result);
int sqplus_length(HSQUIRRELVM vm, HSQOBJECT receiver);
bool sqplus_reverse(HSQUIRRELVM vm, HSQOBJECT receiver);
void sqplus_append(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT value);
bool sqplus_raw_set(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT key, HSQOBJECT value);
bool sqplus_raw_set(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key, HSQOBJECT value);
int sqplus_get_integer(HSQUIRRELVM vm, HSQOBJECT receiver, int key);
const SQChar* sqplus_get_string(HSQUIRRELVM vm, HSQOBJECT receiver, int key);
SQUserPointer sqplus_get_userpointer(HSQUIRRELVM vm, HSQOBJECT receiver, int key);
SQUserPointer sqplus_get_instance_up(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer tag);
bool sqplus_set_instance_up(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer value);
bool sqplus_begin_iteration(HSQUIRRELVM vm, HSQOBJECT receiver);
bool sqplus_exists(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key);
HSQOBJECT sqplus_get_delegate(HSQUIRRELVM vm, HSQOBJECT receiver);
HSQOBJECT sqplus_get_value(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key);
// found is independent of an OT_NULL result; _get is invoked exactly once.
bool sqrat_get(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key, HSQOBJECT& result);
HSQOBJECT sqrat_root(HSQUIRRELVM vm);
HSQOBJECT sqrat_table(HSQUIRRELVM vm);
void sqrat_execute(HSQUIRRELVM vm, HSQOBJECT environment, HSQOBJECT closure,
                   SQBool raiseerror,
                   SQRESULT (*invoke)(HSQUIRRELVM, SQInteger, SQBool, SQBool));
void sqrat_retain(HSQUIRRELVM vm, HSQOBJECT value);
void sqrat_release(HSQUIRRELVM vm, HSQOBJECT value);
HSQOBJECT sqrat_object_value(HSQUIRRELVM vm, HSQOBJECT value);
void sqrat_destroy_object(HSQUIRRELVM vm, HSQOBJECT value, bool owns);
void sqrat_bind_function(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* name,
                          const void* payload, std::size_t size,
                          SQFUNCTION function, bool static_slot);
} // namespace kinoko::script::upstream
