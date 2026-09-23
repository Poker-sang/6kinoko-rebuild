# Continuation batches 74-79 (2026-09-23)

The lexical readability inventory at this continuation's start counted 137 named-mixed-legacy functions and 2,172 lines. After batches 74-79 it counts 124 and 1,541 lines: 13 fewer functions and 631 fewer lines. This is a lexical migration measure, not proof of semantic completeness. The final inventory also counts 1,511 named structured candidates (16,934 lines) and 298 thin bridges (858 lines).

- 74: separated Sqrat bytecode VM from SqPlus source-script VM (`22851f9`); build/DAT handoff `c9a8b6f`.
- 75: named table-serialization VM boundary and owned SE-wave input (`cf55be0`); handoff `4bb5a39`.
- 76: named current BGM handle and IME control slots (`b524608`); handoff `efda36a`.
- 77: archive-mode and IME-context ownership boundary (`78a0214`, contract wiring `44030c6`); handoff `20013d1`.
- 78: moved foreground keyboard fallback to input service (`3a9d0fe`, `ea8fbb0`); handoff `ec12c8d`.
- 79: moved ReadCSV script adapter out of address-named C (`478259a`, contract wiring `a83a0f5`); handoff `82c4e50`.

Each successful batch had its source committed before an independent quiet Win32 Release build. Each successful EXE has three original DATs beside it, verified by size and SHA256. The newest EXE is `runtime-builds/script-csv-entry-quiet-r79b/kinoko_retdec_rebuild.exe`, SHA256 `86120E8DAB7359BFE1D36EB20181E9302270152E770B05BD8737DC6070865D2E`.

Failed full-build artifacts remain in `build-runs/app-ime-archive-quiet-r77` (contract stubs) and `build-runs/script-csv-entry-quiet-r79` (contract still referenced old address name). Their replacements used independent `r77b` and `r79b` directories. No game run, CTest, contract executable, or local automated test was executed; contract targets were compiled only. Existing user modification to `docs/decompiler-cleanup-r126/original-stage-evidence.json` was not staged.

Next: recover the still mixed 50-line runtime-object initialization and input-script initialization with original binary evidence, then examine the 61-line Squirrel-table diagnostic path separately. Preserve init order and borrowed object identities; audit line counts remain only a prioritization tool.
