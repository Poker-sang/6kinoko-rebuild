#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered SqPlus class, property and native-method boundaries. The legacy
   numeric exports remain only for callers in the generated game code. */
/* Stable identities of constructed upstream ClassType objects. */
int32_t* kinoko_sqplus_scalar_type(int32_t category);
int32_t* kinoko_sqplus_game_type(int32_t kind, int32_t (*copy)(int32_t, int32_t));
int32_t* kinoko_native_binding_type(int32_t category);

int32_t function_4a9250(int32_t result, int32_t a2);
int32_t function_4a9370(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t function_4a9490(int32_t * a1, int32_t a2, int32_t a3, char * a4, char * a5);
int32_t function_4aa540(int32_t a1, int32_t a2, int32_t * a3, int32_t a4, int32_t a5);
int32_t function_45f4f0(int32_t a1);
int32_t function_45f640(int32_t *root_object);
int32_t function_45fab0(int32_t a1, int32_t a2);
int32_t *function_45f3e0_this(int32_t *this_ptr, int32_t a1,
                                     int32_t a2, int32_t a3, int32_t *a4,
                                     int32_t a5, int32_t a6);
int32_t function_460920(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
int32_t function_4609c0(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
int32_t function_460a60(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
int32_t function_4607e0(int32_t * a1, int32_t a2, int32_t a3, int32_t a4);
int32_t function_460d10_this(int32_t this_ptr, const char *name,
                                    int32_t parent_ptr);
int32_t *function_460d10_actor(int32_t *object_ptr, const char *name,
                                      int32_t parent_ptr);
void function_460e00_register_actor_method(int32_t vm,
                                                  int32_t *object_ptr,
                                                  const char *name,
                                                  int32_t native_function,
                                                  int32_t type_wrapper,
                                                  int32_t slot_flags);
int32_t function_4aa5e0(int32_t *context, int32_t *out_varinfo);
int32_t retdec_get_var_info(int32_t *out_ptr, int32_t *context);
int32_t retdec_get_var_value(int32_t *context, int32_t varinfo,
                                    int32_t source_ptr);
int32_t retdec_set_var_value(int32_t *context, int32_t varinfo,
                                    int32_t source_ptr);
int32_t retdec_resolve_instance_var(int32_t vm, int32_t top,
                                           int32_t *out_varinfo,
                                           int32_t *out_source);
int32_t function_4aab60(int32_t a1);
int32_t function_4aabd0(int32_t a1);
int32_t function_4aaf30(int32_t a1);
int32_t function_4aafa0(int32_t a1);
int32_t function_45f560(int32_t a1, int32_t a2);
long double function_45f5a0(int32_t a1, int32_t a2);
int32_t function_45f5e0(int32_t *target, int32_t unused, int32_t vm);
int32_t function_45f5e0_at(int32_t *target, int32_t unused,
                                   int32_t vm, int32_t index);
int32_t function_45f850(int32_t a1, int32_t a2, int32_t a3,
                        int32_t a4, int32_t a5);
int32_t function_45f8c0(int32_t a1, int32_t a2, int32_t a3,
                        int32_t a4, int32_t a5);
int32_t function_45f9d0(int32_t a1, int32_t a2, int32_t a3,
                        int32_t a4, int32_t a5);
int32_t function_460540_this(int32_t result_ptr, int32_t vm);
int32_t function_460540(int32_t vm);
int32_t function_460b00(int32_t a1);
int32_t function_460b50(int32_t a1);
int32_t function_460bc0(int32_t a1);
int32_t function_460c10(int32_t a1);
int32_t function_460c70(int32_t a1);
int32_t function_460cc0(int32_t a1);

#ifdef __cplusplus
}
#endif
