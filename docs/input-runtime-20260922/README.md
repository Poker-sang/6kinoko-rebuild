# Input runtime continuation

Base 377679e on codex/sprite-math-cleanup already includes the previous three
rendering batches, not yet pushed at task start. Continue from that code on
codex/input-runtime-continuation, preserving the unrelated R126 evidence edit.
User accepted the preceding result and requested more consecutive batches,
prioritizing simple or important work. This round performs compile/link and
DAT staging only; no local contracts, CTest, Python tests or game are executed.
Historical execution claims in older handoffs are not new validation evidence.

Original ../6kinoko/6kinoko.exe SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP ef94d882 was opened through skill open.ps1 on a temporary copy.
survey.json confirms the original x86 executable. Original imports are retained
for this same hash in ../sprite-math-20260922/imports.json: Win32 window/input,
file/thread, D3D/D3DX and COM anchors. Fresh input disassembly/decompilation and
xrefs are recorded here. Squirrel remains source-backed 2.2.2; auxiliary
analysis/remaining-mapping-20260920/source-disassembly-excerpts.json supports
the existing receiver/refcount boundaries, which this round must preserve.

## Batch 1: registered keys and modifiers

408320 and 4083E0 become kinoko_input_keys_update and kinoko_input_key_pressed.
KinokoKeyTracker names the 256 counters, native key-list owner at +1024, and
Shift/Alt/Control at +1040/+1041/+1042. Layout assertions preserve 1044 bytes.
The key-list storage is a real separate vector owner, not a vector overlaid on
old fields. All new APIs accept actual pointers; only not-yet-migrated manager
and legacy test-storage callers convert at their explicit boundary.

Keep registered-key-only updates, signed INC bit wrap, high-bit key detection,
left/right modifier pairs, low-byte scan/modifier arguments, one-frame presses,
deduplicated insertion and independent assigned key-list owners. Reserved
record bytes remain untouched. input_keys_contract covers these cases and
compiles only. Runtime/manager/copy consumers use the new APIs.

Planned next ownership chains: physical device state; borrowed cluster devices;
manager-owned device vector/copy; assignment files and capture; frame publication
and script field descriptors. Each gets a separate committed build and DAT set.
