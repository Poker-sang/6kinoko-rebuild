#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void kinoko_diagnostics_initialize(void);
void kinoko_diagnostics_shutdown(void);
int kinoko_report_exception(EXCEPTION_POINTERS *exception);
void retdec_trace(const char *message);
void retdec_trace_hresult(const char *label, long value);

#ifdef __cplusplus
}
#endif
