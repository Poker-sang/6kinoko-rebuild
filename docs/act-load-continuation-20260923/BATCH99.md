# Batch 99 - native-script-callbacks-quiet-r99

Source committed before build: f9106c8 (`Name remaining native script callback entries`).

This batch named the remaining Squirrel script callback entry bodies in
`squirrel_native_calls.cpp`: integer-pair, string-object, integer-result, truthy,
no-argument, string/object, string/bool, integer-truth, float, integer, and
stack callback adapters. The original `function_47xxxx` exports remain as thin
ABI-preserving aliases, so existing registration tables and legacy callers keep
their addresses and calling conventions.

Build: `build-runs/native-script-callbacks-quiet-r99`.
EXE: `runtime-builds/native-script-callbacks-quiet-r99/kinoko_retdec_rebuild.exe`.
EXE SHA256: `A22DEF3451C2A23E9C76758F49D37A179FD857E45099F0436CB292333D572E1D`.

Quiet Win32 Release configure and full build succeeded, including contract
compilation. Three original DATs were staged beside the EXE and size/SHA256
verified.

No game, CTest, contract executable, or local automated test was executed; runtime
verification remains delegated to the user.