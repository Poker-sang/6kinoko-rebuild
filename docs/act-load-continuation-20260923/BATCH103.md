# Batch 103 - global-registration-bridge-r103

Source commit: 056bc3b (`r103 name global registration bridge slots`).

Converted the global/root registration bodies to named VM and entry aliases. Root table pointers now use typed legacy pointer helpers, callback entry addresses retain their ABI types, and the original registration order remains unchanged.

Audit after the batch: `named_mixed_legacy=26`, `271` function-body lines.

Build: `build-runs/mixed-legacy-r100` (incremental Release rebuild).
EXE: `runtime-builds/mixed-legacy-r103/kinoko_retdec_rebuild.exe`.
EXE SHA256: `6DC896D9BF71BB17EEBE66EA87F5186BB9341AE405B9945523EA0B35DA998940`.

Three DAT files were staged beside the EXE and size/SHA256 verified. No game, CTest, contract executable, or local automated test was executed.
