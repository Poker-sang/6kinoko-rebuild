# Continuation batches 86-90 (2026-09-23)

Starting point after batch 85: 114 named-mixed-legacy functions and 1,130 lexical body lines. Current audit after batch 90: 108 functions and 1,007 lines. Reduction: 6 functions and 123 lines (10.9% of the starting mixed-line inventory). The audit is lexical and only a prioritization measure.

- 86: named root binding value/integer adapters (`9d22282`); handoff `cde2d2c`.
- 87: named camera/map class publication slots and camera update closure (`a888b22`, declaration fix `6b599ab`); handoff `fa58ebc`.
- 88: named ACT resource stream and output mode boundaries (`8e335ab`); handoff `eb9c632`.
- 89: used named ACT host identities for map layout lifetime (`f017768`), retained chip sprite ABI declaration, and supplied host identities to the isolated map cache contract (`c297ca9`, `4471012`); first three build directories were retained, successful handoff `c4d5d4a`.
- 90: used current VM and named camera/map registration interfaces (`fdc1fba`), resolved internal/external registration name ambiguity (`c2a23ab`); handoff `b2f2201`.

Every successful batch used an independent quiet Win32 Release build and staged/verified the three original DATs beside the resulting EXE. Newest EXE: `runtime-builds/global-registration-names-quiet-r90b/kinoko_retdec_rebuild.exe`, recorded in `docs/act-load-continuation-20260923/BATCH90.md`.

Failed build artifacts remain in `build-runs/camera-map-slots-quiet-r87`, `build-runs/map-layout-host-quiet-r89`, `build-runs/map-layout-host-quiet-r89b`, `build-runs/map-layout-host-quiet-r89c`, and `build-runs/global-registration-names-quiet-r90`; replacement builds used independent directories. No game run, CTest, contract executable, or local automated test was executed; successful contract targets were compiled only. User modification `docs/decompiler-cleanup-r126/original-stage-evidence.json` remains unstaged.

Next priorities: migrate the remaining global registration body with explicit named VM/root lifecycle helpers, then actor/input class registration and map chip cache refresh. Keep the current C ABI aliases where isolated contracts still depend on them.
