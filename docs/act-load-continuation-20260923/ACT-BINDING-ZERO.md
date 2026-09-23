# ACT binding mixed-body milestone — 2026-09-23

The unchanged `tools/audit_readability.py` lexical inventory reports **0**
`named_mixed_legacy` functions / **0** body lines in
`src/squirrel/act_binding.cpp` at source commit `8cf6a40`. At the beginning
of this continuation that file had 1,609 such lines; the successive source
commits reduced it to 912 (batch 66, `5a251d5`), 669 (batch 67, `1a34dd2`),
394 (batch 68, `f82cb5c`), and zero (batch 69, `8cf6a40`). The same audit
still reports 2,692 mixed body lines across the rest of its scoped C/C++ files.
These are lexical categories, not proof of behavioral identity or complete
C++ recovery.

ACT publication now reads the named native string/vector fields through
byte-backed, offset-checked views. Property setter/getter tables have explicit
class-level ownership and shared failure cleanup. The ACT and resource class
pairs, player class pair, and selected native method callbacks have named
interfaces. The original Squirrel callback entrypoints remain available as
thin ABI adapters. Resource publication still precedes layer registration,
and the publication failure/release order remains in place.

Each source commit preceded its own unique quiet Win32 Release build. Batches
66–69 each staged and verified the three original DAT files beside the EXE.
The latest artifact is
`runtime-builds/act-binding-zero-quiet-r69/kinoko_retdec_rebuild.exe`, SHA256
`62787061C94DDEF831406C0F697C3CFD0719F3D65ECFF1A1610A26AC9F1B049B`.
The build compiled its contract targets; no game, CTest, contract executable,
or local automated test was run by the agent. All prior artifacts remain.
