# Batch 97 - callback-adapters-quiet-r97

Source committed before build: f0e32bd (`Name Squirrel callback adapter entries`).

This batch names the three adjacent Squirrel callback adapters for string-only,
two-integer, and string-plus-object-pair calls. The original
`function_4716b0`, `function_471a60`, and `function_471720` symbols remain as
ABI-preserving aliases. The `function_471f70` path now delegates through the
named string-pair entry as well; argument validation, callback order, ownership,
and return behavior are unchanged.

Build: `build-runs/callback-adapters-quiet-r97`.
EXE: `runtime-builds/callback-adapters-quiet-r97/kinoko_retdec_rebuild.exe`.
EXE SHA256: `CBC2594D2CCCEBAC1A83676A5C1E855189061ED309EAF88B02D25948E33B390C`.

Quiet Win32 Release configure and full build succeeded, including contract
compilation. Three original DATs were staged beside the EXE and size/SHA256
verified.

No game, CTest, contract executable, or local automated test was executed; runtime
verification remains delegated to the user.