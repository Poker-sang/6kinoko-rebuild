#include "kinoko/squirrel_native_types.h"
#ifndef KINOKO_SQUIRREL_SOURCE_RUNTIME_H
#define KINOKO_SQUIRREL_SOURCE_RUNTIME_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Exchange only the legacy TLS receiver, returning its previous value.
 * This is an ABI adapter, not another VM or an alternate execution path. */
typedef struct SQVM* (*kinoko_sq_context_exchange)(struct SQVM* vm);
void kinoko_sq_set_context_exchange(kinoko_sq_context_exchange exchange);
struct SQVM;
typedef struct KinokoVmStackSnapshot {
    const void *storage;
    int32_t top, base;
} KinokoVmStackSnapshot;
/* Diagnostic view of the source VM; borrows storage and does not mutate it. */
KinokoVmStackSnapshot kinoko_sq_stack_snapshot(const struct SQVM *vm);

SQVM* kinoko_sq_open(int32_t stack_size);
int32_t kinoko_sq_compile_act_source(SQVM* vm, const char *text, int32_t length, const int32_t environment[2]);
SQSharedState* kinoko_sq_shared_state(SQVM* vm);
void kinoko_sq_delete_shared_state(SQSharedState* state);
/* Existing embedding constructor: intentionally returns no values. */
int32_t kinoko_sq_noop_constructor(SQVM* vm);
/* Original SQRefCounted deleting-destructor entry (ECX receiver). */
SQRefCounted* __fastcall kinoko_sq_delete_refcounted(SQRefCounted* object, void *unused_edx, int32_t flags);
SQVM* kinoko_sq_construct_vm(void* storage, SQSharedState* shared_state);
int32_t kinoko_sq_call(SQVM* vm, int32_t nargs, int32_t retval, int32_t raiseerror);
int32_t kinoko_sq_call_object(SQVM* vm, SQObjectPtr* closure, int32_t nargs,
    int32_t stackbase, SQObjectPtr* result, int32_t raiseerror);
int32_t kinoko_sq_execute(SQVM* vm, SQObjectPtr* closure, int32_t target,
    int32_t nargs, int32_t stackbase, SQObjectPtr* result, int32_t raiseerror,
    int32_t resume_vm);
int32_t kinoko_sq_call_native(SQVM* vm, SQNativeClosure* closure, int32_t nargs,
    int32_t stackbase, SQObjectPtr* result, unsigned char* suspended);
SQVM* kinoko_sq_pop(SQVM* vm, int32_t count);
SQObjectPtr* kinoko_sq_get_up(SQVM* vm, int32_t index);
SQObjectPtr* kinoko_sq_get_at(SQVM* vm, int32_t index);

void kinoko_sq_mark_value(const HSQOBJECT *value, SQCollectable **live_head);
void kinoko_sq_finalize_object(SQCollectable* object, int32_t type);
int32_t kinoko_sq_collect(SQSharedState* shared_state, SQVM* vm);
/* Called only after the recovered vtable identities have been ruled out. */
int32_t kinoko_sq_source_object_type(SQCollectable* object);

#define KINOKO_SQ_GC_ENTRIES(name, Class) \
    void __fastcall kinoko_sq_##name##_mark(Class* object, void *unused, SQCollectable **chain); \
    void __fastcall kinoko_sq_##name##_finalize(Class* object, void *unused)
KINOKO_SQ_GC_ENTRIES(closure, SQClosure);
KINOKO_SQ_GC_ENTRIES(nativeclosure, SQNativeClosure);
KINOKO_SQ_GC_ENTRIES(userdata, SQUserData);
KINOKO_SQ_GC_ENTRIES(array, SQArray);
KINOKO_SQ_GC_ENTRIES(generator, SQGenerator);
KINOKO_SQ_GC_ENTRIES(vm, SQVM);
KINOKO_SQ_GC_ENTRIES(table, SQTable);
KINOKO_SQ_GC_ENTRIES(instance, SQInstance);
KINOKO_SQ_GC_ENTRIES(class, SQClass);
#undef KINOKO_SQ_GC_ENTRIES

#ifdef __cplusplus
}
#endif
#endif
