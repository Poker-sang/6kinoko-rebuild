#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Address-named exports are compatibility boundaries only. Implementation
   uses ObjectView/HSQOBJECT and the vendored Squirrel 2.2.2 API. */
int32_t kinoko_squirrel_object_vtable(void);
int32_t function_4a94e0_this(int32_t object);
int32_t retdec_msvc_0_Init_locks_std__QAE_XZ5_this(int32_t object);
int32_t* function_4a9500_this(int32_t* object, int32_t source);
int32_t function_4a9540_this(int32_t object, int32_t type, int32_t data);
int32_t function_4a9570_this(int32_t object);
int32_t function_4a95c0_this(int32_t object, int32_t source);
int32_t function_4a9600_this(int32_t object, int32_t source);
int32_t function_4a9660_this(int32_t object, int32_t index);
int32_t function_4a96c0_this(int32_t object);
int32_t function_4a96d0(int32_t object);
int32_t function_4a99f0(int32_t object);
int32_t function_4a9730_this(int32_t object, int32_t key, int32_t text);
int32_t function_4a97b0_this(int32_t object, int32_t key, int32_t value);
int32_t function_4a9840_this(int32_t object, const char* key, int32_t value);
int32_t function_4a9950(int32_t object, int32_t key, int32_t size, int32_t tag);
int32_t function_4a9a30_this(int32_t object);
int32_t function_4a9a40_this(int32_t object, int32_t key);
int32_t function_4a9ac0_this(int32_t object, int32_t key);
int32_t function_4aa000_this(int32_t object, int32_t key);
int32_t function_4a9b40_this(int32_t object, int32_t tag);
int32_t function_4a9bb0_this(int32_t object, int32_t native_pointer);
int32_t function_4a9c10_this(int32_t object);
int32_t function_4a9c60(int32_t* key, int32_t* value);
int32_t function_4a9d30_this(int32_t object, int32_t* tag);
int32_t function_4a9d50(void);
int32_t function_4a9d70_this(int32_t object);
int32_t function_4a9e30_this(int32_t object, int32_t thread);
int32_t function_4a9f60(int32_t object, int32_t delegate);
int32_t function_4aa080(int32_t object, int32_t key, int32_t output, int32_t tag_output);
int32_t retdec_function_4aa110_this(int32_t object, const char* key, int32_t* output, int32_t tag_output);
int32_t function_4aa1a0(int32_t object, const char* key);
int32_t* function_4aa210_this(int32_t object, int32_t output);
int32_t* function_4aa3a0_this(int32_t object, int32_t output, const char* key);
int32_t function_4a90c0_this(int32_t object, int32_t klass);
int32_t* function_4a91c0_this(int32_t* object);
int32_t* function_4a92e0_this(int32_t* object, int32_t size);

#ifdef __cplusplus
}
#endif
