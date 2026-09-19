#pragma once
#include <squirrel.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Real SQFUNCTION signatures for the recovered Sqrat field callbacks. The
   captured userdata is a signed Win32 field offset, not a code address. */
SQInteger kinoko_sqrat_get_int(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_int(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_float(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_float(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_bool(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_bool(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_short(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_short(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_pointer_int(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_pointer_int(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_pointer_float(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_pointer_float(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_get_string(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_set_string(HSQUIRRELVM vm);
SQInteger kinoko_sqrat_noop(HSQUIRRELVM vm);
#ifdef __cplusplus
}
#endif
