# Batch 105 - bridge-entry-collapse-r105

Source commit: 4f4316c (`r105 remove redundant bridge entry layers`).

Removed 80 redundant internal bridge entry functions from `squirrel_legacy_api.cpp` and `squirrel_native_calls.cpp`, then collapsed the remaining callback alias pairs without changing the exported `function_xxx` ABI names. The old public symbols remain available; duplicate `_entry` layers are gone.

Audit after the batch: `thin_bridge=338` functions / `1182` lines, down from 419 / 1423. `named_mixed_legacy` remains zero; one address-named ABI symbol remains separately classified.

Build: `build-runs/mixed-legacy-r100` (Win32 Release succeeded).
EXE: `runtime-builds/bridge-collapse-r105/kinoko_retdec_rebuild.exe`.
EXE SHA256: `B86CB0E87480E9D8C83FD9F5740B3A0624CBB973ACBF0D987672C253C0507192`.

Three DAT files were staged beside the EXE and size/SHA256 verified. No game, CTest, contract executable, or local automated test was executed.
