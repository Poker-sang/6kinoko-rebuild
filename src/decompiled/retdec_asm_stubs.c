#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "retdec_asm_stubs.h"

#define RETDEC_DEFINE_STUB(name) \
    int64_t name() { return 0; }
RETDEC_ASM_STUBS(RETDEC_DEFINE_STUB)
#undef RETDEC_DEFINE_STUB

static long double g_retdec_fpr[1024];

static unsigned retdec_fpr_index(int32_t reg)
{
    return (unsigned)reg % (unsigned)(sizeof(g_retdec_fpr) / sizeof(g_retdec_fpr[0]));
}

long double __frontend_reg_load_fpr(int32_t reg)
{
    return g_retdec_fpr[retdec_fpr_index(reg)];
}

void __frontend_reg_store_fpr(int32_t reg, long double value)
{
    g_retdec_fpr[retdec_fpr_index(reg)] = value;
}

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

int64_t __asm_rep_movsb_memcpy(void *destination, const void *source, int32_t count)
{
    if (count > 0) {
        memcpy(destination, source, (size_t)count);
    }
    return 0;
}

int64_t __asm_rep_movsd_memcpy(void *destination, const void *source, int32_t count)
{
    if (count > 0) {
        memcpy(destination, source, (size_t)count * 4u);
    }
    return 0;
}

int64_t __asm_rep_stosb_memset(void *destination, int32_t value, int32_t count)
{
    if (count > 0) {
        memset(destination, value & 0xff, (size_t)count);
    }
    return 0;
}

int64_t __asm_rep_stosd_memset(void *destination, int32_t value, int32_t count)
{
    uint32_t *words = (uint32_t *)destination;
    int32_t i;
    for (i = 0; i < count; ++i) {
        words[i] = (uint32_t)value;
    }
    return 0;
}
