# Continuation batches 91-94 (2026-09-23)

Starting point after batch 90: 108 named-mixed-legacy functions and 1,007 lexical body lines. Current audit after batch 94: 99 functions and 883 lines. Reduction: 9 functions and 124 lines (12.3% of the starting mixed-line inventory).

- 91: moved actor class, step/user keys, and input class publication through named host identities (`2130c52`, writable-slot fix `2bf03c2`); handoff `e03a4dd`.
- 92: named the map chip quad vtable in ACT host symbols (`0385ed7`, include/conversion fix); handoff `34661e7`.
- 93: named four input native-call wrapper entries and updated their isolated contract (`4e6d320`, contract fix); handoff `c3cb268`.
- 94: exposed the five SqPlus VM lifecycle slots through a named host-state structure (`ac11ef7`, C-host include fix `5a4809c`); handoff `59ce73b`.

Every successful batch used an independent quiet Win32 Release build and staged/verified the three original DATs beside the resulting EXE. Newest EXE: `runtime-builds/sqplus-host-slots-quiet-r94b/kinoko_retdec_rebuild.exe`, recorded in `docs/act-load-continuation-20260923/BATCH94.md`.

Failed build artifacts remain in `build-runs/actor-input-slots-quiet-r91`, `build-runs/map-chip-host-quiet-r92`, `build-runs/input-entry-names-quiet-r93`, and `build-runs/sqplus-host-slots-quiet-r94`; replacement builds used independent directories. No game run, CTest, contract executable, or local automated test was executed; successful contract targets were compiled only. User modification `docs/decompiler-cleanup-r126/original-stage-evidence.json` remains unstaged.

Next priorities: finish the global registration body and the remaining camera/input class builder helpers, then migrate the 34-line map clone and 32-line resource/cache helpers where their vtable identities are already available through named host symbols.
