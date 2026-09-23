# Batch 100 - mixed-legacy-accessors-r100

Source committed before build: cca6cf9 (`r100 convert mixed legacy accessors`).

This batch converted the low-risk `named_mixed_legacy` accessors in reconstructed and Squirrel code. Global ABI slots now have named aliases or record accessors in map containers, map layout lifetime, process paths, IME input, stage cleanup, script callbacks, camera initialization, audio runtime, font/glyph state, script-file VM helpers, actor lifecycle, camera binding, Sqrat bridge, and related helpers. The changes preserve the original exported symbols, object layouts, and call order.

Readability audit after the batch: `named_mixed_legacy=57` functions and `569` function-body lines (from `99` functions and `884` lines at r99). Remaining mixed entries are primarily the decompiled C translation unit, ABI/compiler stubs, and larger host bridge bodies that still need semantic field recovery.

Build: `build-runs/mixed-legacy-r100`.
EXE: `runtime-builds/mixed-legacy-r100/kinoko_retdec_rebuild.exe`.
EXE SHA256: `BD1BDDA6322021DB32F064FCEF678E8DC4150C8455B5244E306E8CF09411FF29`.

Quiet Win32 Release configure and full build succeeded. Three original DATs were staged beside the EXE and size/SHA256 verified.

No game, CTest, contract executable, or local automated test was executed; runtime verification remains delegated to the user.
