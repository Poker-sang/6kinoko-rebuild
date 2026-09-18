#pragma once

#include <stdint.h>
#include "kinoko/legacy_abi.h"
#include "kinoko/retdec_memory.h"

int *__errno(void);

int32_t __purecall(void);
int32_t _3f__3f__G__non_rtti_object_40_std_40__40_UAEPAXI_40_Z(int32_t this_ptr, uint32_t flags);


/* RetDec leaves these machine-level helpers as external calls. The generic
   declarations keep the generated C compilable; typed REP/FPR operations live separately in retdec_memory.cpp. Do not
   convert these unknown-argument C declarations into zero-argument C++ APIs. */
#define RETDEC_ASM_STUBS(X) \
    X(__asm_addpd) \
    X(__asm_addsd) \
    X(__asm_addsd_450) \
    X(__asm_andnpd) \
    X(__asm_andpd) \
    X(__asm_cmpeqsd) \
    X(__asm_cmpltsd) \
    X(__asm_comisd) \
    X(__asm_cvtdq2pd) \
    X(__asm_cvtsd2si) \
    X(__asm_cvtsi2sd) \
    X(__asm_cvttsd2si) \
    X(__asm_divsd) \
    X(__asm_fnsave) \
    X(__asm_fpatan) \
    X(__asm_fptan) \
    X(__asm_frstor) \
    X(__asm_hlt) \
    X(__asm_insb) \
    X(__asm_int3) \
    X(__asm_movapd) \
    X(__asm_movd) \
    X(__asm_movd_452) \
    X(__asm_movdqa_446) \
    X(__asm_movlpd) \
    X(__asm_movlpd_451) \
    X(__asm_movq) \
    X(__asm_movq_459) \
    X(__asm_movsd) \
    X(__asm_mulpd) \
    X(__asm_mulsd) \
    X(__asm_mulsd_449) \
    X(__asm_orpd) \
    X(__asm_orps) \
    X(__asm_paddq) \
    X(__asm_pand) \
    X(__asm_pandn) \
    X(__asm_pcmpeqd) \
    X(__asm_pextrw) \
    X(__asm_pinsrw) \
    X(__asm_pmaxsw) \
    X(__asm_pmovmskb) \
    X(__asm_pshufd) \
    X(__asm_psllq) \
    X(__asm_psllq_454) \
    X(__asm_psrlq) \
    X(__asm_psubd) \
    X(__asm_psubq) \
    X(__asm_pxor) \
    X(__asm_sqrtsd) \
    X(__asm_subpd) \
    X(__asm_subsd) \
    X(__asm_subsd_453) \
    X(__asm_ucomisd) \
    X(__asm_unpckhpd) \
    X(__asm_unpcklpd) \
    X(__asm_wait) \
    X(__asm_xorpd)

#define RETDEC_DECLARE_STUB(name) int64_t name();
RETDEC_ASM_STUBS(RETDEC_DECLARE_STUB)
#undef RETDEC_DECLARE_STUB
