#pragma once
#include <squirrel.h>

// Values cross this boundary; neither the game's unaligned legacy records nor
// internal SQObjectPtr references are overlaid with an upstream C++ object.
// Each *_retain/new/assign result owns exactly one external VM reference.
namespace kinoko::script::upstream {
HSQOBJECT sqplus_assign(HSQUIRRELVM vm, HSQOBJECT previous, HSQOBJECT incoming);
void sqplus_retain(HSQUIRRELVM vm, HSQOBJECT value);
void sqplus_release(HSQUIRRELVM vm, HSQOBJECT value);
bool sqplus_create_class(HSQUIRRELVM vm, HSQOBJECT& output, SQUserPointer tag,
                         const SQChar* name, const SQChar* parent);
HSQOBJECT sqrat_root(HSQUIRRELVM vm);
HSQOBJECT sqrat_table(HSQUIRRELVM vm);
void sqrat_retain(HSQUIRRELVM vm, HSQOBJECT value);
void sqrat_release(HSQUIRRELVM vm, HSQOBJECT value);
} // namespace kinoko::script::upstream
