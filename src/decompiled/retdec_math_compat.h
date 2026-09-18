#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* These return in x87 ST(0) on Win32. An implicit int declaration leaks
   the result and eventually overflows the floating-point register stack. */
double _frexp(double value, int *exponent);
double _fabs(double value);
double _acos(double value);
double _floor(double value);
double _ceil(double value);
long double llvm_log2_f80(long double value);
long double llvm_round_f80(long double value);
long double llvm_exp2_f80(long double value);

#ifdef __cplusplus
}
#endif
