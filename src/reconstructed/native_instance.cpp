#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/upstream_bindings.hpp"

extern "C" int32_t kinoko_native_void_type(void);

// All class lookup, instance creation, ancestry population and release-hook
// installation execute the original SqPlus source. Only host address words
// and the recovered type identity cross this boundary.
extern "C" int32_t kinoko_native_instance_create(int32_t vm_address, int32_t class_name,
    int32_t native_pointer, int32_t release_hook, int32_t object_vtable) {
    using namespace kinoko::script;
    (void)object_vtable; // no simulated SquirrelObject is constructed anymore
    return upstream::sqplus_native_instance(pointer<SQVM>(vm_address),
        pointer<const char>(class_name), pointer(native_pointer),
        reinterpret_cast<SQRELEASEHOOK>(pointer(release_hook)),
        pointer(kinoko_native_void_type()));
}
