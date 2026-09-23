# Batch 102 - audit-classification-r102

Source commit: 7f83056 (`audit: classify decompiled bodies by legacy markers`).

The readability inventory no longer marks every function in `src/decompiled` as mixed solely because of its file path. A function is now `named_mixed_legacy` only when its body contains the actual legacy markers being tracked (`function_xxx`, `v###`, `g###`, `goto`, or the old integer-address cast form). This keeps the audit aligned with the recovery goal while retaining the decompiled sources in scope.

Current audit: `named_mixed_legacy=28`, `328` function-body lines; `thin_bridge=384`, `1116` lines; `named_structured_candidate=1607`, `18081` lines.

This batch changes audit classification only; the r101 executable remains the latest rebuilt artifact at `runtime-builds/mixed-legacy-r101/kinoko_retdec_rebuild.exe`, with DATs staged and verified. No game, CTest, contract executable, or local automated test was executed.
