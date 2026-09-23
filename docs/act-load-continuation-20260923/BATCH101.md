# Batch 101 - small-mixed-legacy-bridges-r101

Source commit: d45b161 (`r101 finish small mixed legacy bridges`).

Completed the remaining small non-decompiled mixed entries in string layout type identity, actor creation callback, input class construction, and Squirrel property dispatch. Replaced legacy global slots and explicit `intptr_t` bridge casts with named aliases, typed pointer helpers, and address conversion at the ABI boundary while preserving the original call order and exported wrappers.

Readability audit after the batch: `named_mixed_legacy=53` and `513` function-body lines. The residual set is concentrated in the decompiled C translation unit and larger registration/host bridge bodies.

Build: `build-runs/mixed-legacy-r100` (incremental Release rebuild after r101).
EXE: `runtime-builds/mixed-legacy-r101/kinoko_retdec_rebuild.exe`.
EXE SHA256: `B33B9FB8BCF7B0ECD4A4185519352C2D20E891035769952F8585ACFEDB27B17D`.

Three original DATs were staged beside the EXE and size/SHA256 verified. No game, CTest, contract executable, or local automated test was executed; runtime verification remains delegated to the user.
