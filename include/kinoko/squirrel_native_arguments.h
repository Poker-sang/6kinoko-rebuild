#pragma once
#include <stdint.h>
struct SQVM;

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered native closure argument boundaries. Numeric conversions are
   deliberately strict: accepting a float where an integer was required would
   change the original host API, even though sq_getinteger permits coercion. */
void* kinoko_native_target_from_userdata(struct SQVM * vm);
void* kinoko_native_callback_from_stack(struct SQVM * vm);
int32_t kinoko_native_string_arg(struct SQVM * vm, int32_t index, int32_t* value);
int32_t kinoko_native_integer_arg(struct SQVM * vm, int32_t index, int32_t* value);
int32_t kinoko_native_float_arg(struct SQVM * vm, int32_t index, float* value);
/* Borrowed pair: no reference-count change. Positive argument indices only. */
int32_t kinoko_native_value_pair(struct SQVM * vm, int32_t index, int32_t* value);
/* Constructs a 12-byte owning wrapper from a positive argument index. */
int32_t kinoko_squirrel_pair_from_stack(struct SQVM * vm, int32_t index, int32_t* target);

#ifdef __cplusplus
}
#endif
