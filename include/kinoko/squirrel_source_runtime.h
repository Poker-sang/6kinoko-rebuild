#ifndef KINOKO_SQUIRREL_SOURCE_RUNTIME_H
#define KINOKO_SQUIRREL_SOURCE_RUNTIME_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Exchange only the legacy TLS receiver, returning its previous value.
 * This is an ABI adapter, not another VM or an alternate execution path. */
typedef int32_t (*kinoko_sq_context_exchange)(int32_t vm);
void kinoko_sq_set_context_exchange(kinoko_sq_context_exchange exchange);
int32_t kinoko_sq_open(int32_t stack_size);
int32_t kinoko_sq_shared_state(int32_t vm);
/* Existing embedding constructor: intentionally returns no values. */
int32_t kinoko_sq_noop_constructor(int32_t vm);
/* Original SQRefCounted deleting-destructor entry (ECX receiver). */
int32_t __fastcall kinoko_sq_delete_refcounted(int32_t object, void *unused_edx, int32_t flags);
int32_t kinoko_sq_construct_vm(int32_t storage, int32_t shared_state);
int32_t kinoko_sq_call(int32_t vm, int32_t nargs, int32_t retval, int32_t raiseerror);
int32_t kinoko_sq_call_object(int32_t vm, int32_t closure, int32_t nargs,
    int32_t stackbase, int32_t result, int32_t raiseerror);
int32_t kinoko_sq_execute(int32_t vm, int32_t closure, int32_t target,
    int32_t nargs, int32_t stackbase, int32_t result, int32_t raiseerror,
    int32_t resume_vm);
int32_t kinoko_sq_call_native(int32_t vm, int32_t closure, int32_t nargs,
    int32_t stackbase, int32_t result, int32_t suspended);
int32_t kinoko_sq_pop(int32_t vm, int32_t count);
int32_t kinoko_sq_get_up(int32_t vm, int32_t index);
int32_t kinoko_sq_get_at(int32_t vm, int32_t index);

void kinoko_sq_mark_value(const int32_t *value, int32_t *live_head);
void kinoko_sq_finalize_object(int32_t object, int32_t type);
int32_t kinoko_sq_collect(int32_t shared_state, int32_t vm);
/* Called only after the recovered vtable identities have been ruled out. */
int32_t kinoko_sq_source_object_type(int32_t object);

#define KINOKO_SQ_GC_ENTRIES(name) \
    void __fastcall kinoko_sq_##name##_mark(int32_t object, void *unused, int32_t *chain); \
    void __fastcall kinoko_sq_##name##_finalize(int32_t object, void *unused)
KINOKO_SQ_GC_ENTRIES(closure);
KINOKO_SQ_GC_ENTRIES(nativeclosure);
KINOKO_SQ_GC_ENTRIES(userdata);
KINOKO_SQ_GC_ENTRIES(array);
KINOKO_SQ_GC_ENTRIES(generator);
KINOKO_SQ_GC_ENTRIES(vm);
KINOKO_SQ_GC_ENTRIES(table);
KINOKO_SQ_GC_ENTRIES(instance);
KINOKO_SQ_GC_ENTRIES(class);
#undef KINOKO_SQ_GC_ENTRIES

#ifdef __cplusplus
}
#endif
#endif
