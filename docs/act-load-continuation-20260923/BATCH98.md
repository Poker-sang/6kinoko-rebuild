# Batch 98 - legacy-squirrel-api-quiet-r98

Source committed before build: 6d699d5 (`Name legacy Squirrel API entry bodies`).

This batch covered the remaining address-based entry layer in
`src/squirrel/squirrel_legacy_api.cpp` (57 Squirrel API, library, object, and
userdata adapters). Each readable `_entry` body now carries the semantic call;
the original `function_48*`, `function_49*`, and `function_4c*` symbols remain as
thin ABI-preserving forwarders. The underlying Squirrel call order, pointer
conversion boundaries, fastcall exports, and return values are unchanged.

Build: `build-runs/legacy-squirrel-api-quiet-r98`.
EXE: `runtime-builds/legacy-squirrel-api-quiet-r98/kinoko_retdec_rebuild.exe`.
EXE SHA256: `D1802A53037B592BA0B9FFBCD72C69217D0EA894BD33F42C084C0AF23B12E719`.

Quiet Win32 Release configure and full build succeeded, including contract
compilation. Three original DATs were staged beside the EXE and size/SHA256
verified.

No game, CTest, contract executable, or local automated test was executed; runtime
verification remains delegated to the user.