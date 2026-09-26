#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/upstream_bindings.hpp"

extern "C" int32_t kinoko_native_void_type(void);

// All class lookup, instance creation, ancestry population and release-hook
// installation execute the original SqPlus source. Only host address words
// and the recovered type identity cross this boundary.
extern "C" int32_t kinoko_native_instance_create(SQVM* vm, const char* class_name,
    void* native_pointer, SQRELEASEHOOK release_hook) {
    return kinoko::script::upstream::sqplus_native_instance(vm, class_name, native_pointer,
        release_hook, kinoko::script::pointer(kinoko_native_void_type()));
}
