#pragma once
#include <stdint.h>
struct SQVM;

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered SqPlus class/property binding and typed native-method dispatch.
   Numeric variable offsets may also encode immediate constants; preserve that
   recovered metadata representation at the Variable record boundary. */
/* Stable identities of constructed upstream ClassType objects. */
int32_t* kinoko_sqplus_scalar_type(int32_t category);
int32_t* kinoko_sqplus_game_type(int32_t kind, int32_t (*copy)(int32_t, int32_t));
int32_t* kinoko_native_binding_type(int32_t category);

void * kinoko_sqplus_new_string(void * result, const char * a2);
void * kinoko_sqplus_bind_function(void * a1, void * a2, const char * a3, const char * a4);
void * kinoko_sqplus_bind_object_function(int32_t * a1, void * a2, void * a3, char * a4, char * a5);
int32_t  kinoko_sqplus_create_class(struct SQVM * a1, void * a2, int32_t * a3, const char * a4, const char * a5);
int32_t  kinoko_sqplus_install_variable_handlers(void * a1);
int32_t  kinoko_sqplus_setup_hierarchy(int32_t *root_object);
void * kinoko_sqplus_create_variable(void * a1, const char * a2);
int32_t * kinoko_sqplus_initialize_variable(int32_t *this_ptr, int32_t a1, int32_t a2, int32_t a3, int32_t *a4, int32_t a5, int32_t a6);
int32_t kinoko_sqplus_bind_integer(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
int32_t kinoko_sqplus_bind_float(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
int32_t kinoko_sqplus_bind_boolean(int32_t * a1, int32_t * a2, int32_t a3, char * a4, int32_t a5);
void * kinoko_sqplus_create_actor_class(int32_t * a1, struct SQVM * a2, const char * a3, const char * a4);
void * kinoko_sqplus_construct_class_binding(void * this_ptr, const char *name, const char * parent_ptr);
int32_t *kinoko_sqplus_define_actor_class(int32_t *object_ptr, const char *name,
                                      int32_t parent_ptr);
void  kinoko_sqplus_register_actor_method(struct SQVM * vm, int32_t *object_ptr, const char *name, void * native_function, void * type_wrapper, int32_t slot_flags);
int32_t  kinoko_sqplus_find_variable(int32_t *context, int32_t *out_varinfo);
int32_t  kinoko_sqplus_find_table_variable(int32_t *out_ptr, int32_t *context);
int32_t  kinoko_sqplus_read_variable(int32_t *context, int32_t varinfo, int32_t source_ptr);
int32_t  kinoko_sqplus_write_variable(int32_t *context, int32_t varinfo, int32_t source_ptr);
int32_t  kinoko_sqplus_resolve_instance_variable(int32_t vm, int32_t top, int32_t *out_varinfo, int32_t *out_source);
int32_t  kinoko_sqplus_table_get(struct SQVM * a1);
int32_t  kinoko_sqplus_instance_get(struct SQVM * a1);
int32_t  kinoko_sqplus_table_set(struct SQVM * a1);
int32_t  kinoko_sqplus_instance_set(struct SQVM * a1);
int32_t  kinoko_sqplus_argument_integer(struct SQVM * a1, int32_t a2);
long double  kinoko_sqplus_argument_float(struct SQVM * a1, int32_t a2);
int32_t  kinoko_sqplus_argument_object(int32_t *target, int32_t unused, struct SQVM * vm);
int32_t  kinoko_sqplus_argument_object_at(int32_t *target, int32_t unused, struct SQVM * vm, int32_t index);
int32_t  kinoko_sqplus_call_integer(void * a1, void * a2, int32_t a3, struct SQVM * a4, int32_t a5);
int32_t  kinoko_sqplus_call_rectangle(void * a1, void * a2, int32_t a3, struct SQVM * a4, int32_t a5);
int32_t  kinoko_sqplus_call_move(void * a1, void * a2, int32_t a3, struct SQVM * a4, int32_t a5);
void * kinoko_sqplus_resolve_method(void * result_ptr, struct SQVM * vm);
int32_t  kinoko_sqplus_resolve_method_compat(struct SQVM * vm);
int32_t  kinoko_sqplus_void_method(struct SQVM * a1);
int32_t  kinoko_sqplus_object_method(struct SQVM * a1);
int32_t  kinoko_sqplus_integer_method(struct SQVM * a1);
int32_t  kinoko_sqplus_integer_result_method(struct SQVM * a1);
int32_t  kinoko_sqplus_rectangle_method(struct SQVM * a1);
int32_t  kinoko_sqplus_move_method(struct SQVM * a1);

#ifdef __cplusplus
}
#endif
