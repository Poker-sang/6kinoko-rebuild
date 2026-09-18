#include <stdint.h>
#include <errno.h>

#include "retdec_asm_stubs.h"

#define RETDEC_DEFINE_STUB(name) \
    int64_t name() { return 0; }
RETDEC_ASM_STUBS(RETDEC_DEFINE_STUB)
#undef RETDEC_DEFINE_STUB

int32_t __purecall(void)
{
    return 0;
}

int *__errno(void)
{
    return _errno();
}

int32_t _3f__3f__G__non_rtti_object_40_std_40__40_UAEPAXI_40_Z(int32_t this_ptr, uint32_t flags)
{
    (void)this_ptr;
    (void)flags;
    return 0;
}
