#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered native closure argument boundaries. Numeric conversions are
   deliberately strict: accepting a float where an integer was required would
   change the original host API, even though sq_getinteger permits coercion. */
int32_t retdec_native_target_from_userdata(int32_t vm);
int32_t retdec_native_callback_from_stack(int32_t vm);
int32_t retdec_native_string_arg(int32_t vm, int32_t index, int32_t* value);
int32_t retdec_native_integer_arg(int32_t vm, int32_t index, int32_t* value);
int32_t retdec_native_float_arg(int32_t vm, int32_t index, float* value);
/* Borrowed pair: no reference-count change. Positive argument indices only. */
int32_t retdec_native_value_pair(int32_t vm, int32_t index, int32_t* value);
/* Constructs a 12-byte owning wrapper from a positive argument index. */
int32_t retdec_squirrel_pair_from_stack(int32_t vm, int32_t index, int32_t* target);

#ifdef __cplusplus
}
#endif
