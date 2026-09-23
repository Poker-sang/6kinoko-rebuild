# Batch 106 - abi-alias-separation-r106

Source commit: 5160fd0 (`audit: separate intentional ABI aliases`).

The previous bridge collapse physically removed 80 redundant internal entry layers. This batch separates the remaining public `function_xxx` exports, archival decompiled C wrappers, and the established Squirrel legacy API exports into an explicit `abi_alias` category. These symbols remain for address and calling-convention compatibility and are no longer mixed with removable internal bridge layers.

Audit after the batch:
- `thin_bridge=178` functions / `707` lines remain candidates for further code folding.
- `abi_alias=160` functions / `475` lines are intentional compatibility exports.
- `named_mixed_legacy=0`.

The r105 Win32 Release build and staged DAT artifact remain valid; this batch changes audit classification only. No game, CTest, contract executable, or local automated test was executed.
