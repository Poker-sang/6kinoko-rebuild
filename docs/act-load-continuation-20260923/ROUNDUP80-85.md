# Continuation batches 80-85 (2026-09-23)

Starting point after batch 79: 124 named-mixed-legacy functions and 1,541 lexical body lines. Current audit after batch 85: 114 functions and 1,130 lines. Reduction: 10 functions and 411 lines (26.7% of the starting mixed-line inventory). This is a lexical prioritization measure, not a semantic-completeness score.

- 80: recovered the 46E6F0 input script-instance lifecycle behind typed SqPlus host symbols (`01498f4`, dependency fix `3e0f711`); handoff `5b3c7e0`.
- 81: moved replacement CRT-global construction into `runtime_bootstrap.cpp` with borrowed map/actor/renderer identities (`fc90ee1`, contract-boundary fix `00c487c`); handoff `150706f`.
- 82: moved bounded SQTable and RefTable diagnostics into typed records (`a5671b3`); handoff `3b82ac6`.
- 83: corrected float key hashing to use Squirrel value bit patterns and named the ACT script output-mode/stream ABI (`3ae890f`); handoff `abe0275`.
- 84: moved actor diagnostic VM borrowing to the named actor host and initialized string-layout fields through `StringLayoutRecord` (`87d779d`); handoff `12e2345`.
- 85: named the borrowed SqPlus VM slots and primary VM-open boundary (`c292a46`); handoff `3c37a79`.

Every successful batch used an independent quiet Win32 Release build and staged/verified the three original DATs beside the resulting EXE. Newest EXE: `runtime-builds/sqplus-slots-quiet-r85/kinoko_retdec_rebuild.exe`, recorded in `docs/act-load-continuation-20260923/BATCH85.md`.

The failed first build for batch 80 is retained at `build-runs/input-script-instance-quiet-r80`; batch 81's first contract-boundary build is retained at `build-runs/runtime-boot-quiet-r81`. No game run, CTest, contract executable, or local automated test was executed; successful contract targets were compiled only. User modification `docs/decompiler-cleanup-r126/original-stage-evidence.json` remains unstaged.

Next priorities: migrate global binding registration and camera/map registration, then resource serialization helpers and remaining address-named C adapters. Preserve registration order, alias fields, and borrowed vtable identities.
