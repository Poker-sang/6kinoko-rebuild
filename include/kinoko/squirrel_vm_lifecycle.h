#include "kinoko/squirrel_native_types.h"
#ifndef KINOKO_SQUIRREL_VM_LIFECYCLE_H
#define KINOKO_SQUIRREL_VM_LIFECYCLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* sq_newthread semantics: share the parent's state and push the child onto
 * its stack. The returned address is borrowed; the stack owns the reference.
 * No game resources are loaded and no fallback VM is created here. */
SQVM* kinoko_sq_create_thread(SQVM* parent, int32_t stack_size);

/* Mixed-runtime GC still identifies recovered objects by their vtables.
 * Return the source SQVM vtable once a source child has been constructed,
 * or zero before then. This does not create a probe/dummy VM. */
const void* kinoko_sq_source_vm_vtable(void);

#ifdef __cplusplus
}
#endif

#endif
