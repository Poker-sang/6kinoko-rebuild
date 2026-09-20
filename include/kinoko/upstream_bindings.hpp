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
std::array<char, 258> sqplus_variable_key(const SQChar* name) noexcept;
HSQOBJECT sqplus_new_table(HSQUIRRELVM vm);
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
void sqrat_retain(HSQUIRRELVM vm, HSQOBJECT value);
void sqrat_release(HSQUIRRELVM vm, HSQOBJECT value);
} // namespace kinoko::script::upstream
