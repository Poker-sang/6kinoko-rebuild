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

## Batch 2: physical keyboard/controller state

407500 now has a typed KinokoInputDevice ECX receiver and named 68-byte
assignment / 96-byte state records. Shared schema pins counters +72, releases
+128 and axes +144. Typed ordinary pointers replace the lost-receiver integer
port. The virtual adapter retains the original mixed EAX value (id or pointer)
because that register is not uniformly a returned object; callers consume state.

Original evidence preserves -500/500 inclusive dead zone, negative-direction
precedence, sign-change count reset, overflow bits, one-frame releases, disabled
mapping state retention, six normalized controller axes, keyboard extra-axis
zeroes, absent-controller no-write and disabled-device state clearing.
input_device_contract and the updated legacy stage fixture compile only.

## Batch 3: borrowed device aggregation

4077C0 becomes kinoko_input_cluster_update. KinokoInputCluster names its base
device, separate deque owner (+172) and last winning device byte (+192), with
the 196-byte layout asserted. The deque stores actual borrowed device pointers;
copying owns a new list, but retains the same borrowed targets as the original.
All cluster APIs now take pointers, and at/delete return real pointers. The
update's old mixed count/empty-state EAX remains an explicit ABI result.

Keep registration order, signed absolute comparison (including INT_MIN bits),
strict greater-than tie rules, all twelve buttons, accumulated release flags
only on zero count, independent absolute float-axis selection and retention of
the last device byte on an empty frame. Deletion frees only the deque owner,
restores the base method identity and optionally frees the receiver itself.
input_cluster_contract covers ties, releases, axis NaN, signed limits, copied
borrows and empty frames; it is compiled, never executed by this batch.

## Batch 4: Input owner, copy and real virtual methods

KinokoInputManager restores the 1512-byte layout: 12-byte SqPlus boundary,
keyboard +12, native vector owner +180, cluster +196, keys +392 and published
state +1436. Published counters, release bytes and digits have named fields.
The old vector/deque/key padding stays reserved and is not copied as payload.
Device vectors own normally constructed elements with virtual destruction;
copy construction installs base methods while assignment preserves destination
methods and copies only assignment/state. The new methods tables contain real
typed virtual entries instead of g35/g30 integer arrays.

46ED80/46EBD0 becomes pointer-valued kinoko_input_manager_assign, preserving
SqPlus external-reference assignment first, then keyboard, device owner,
cluster, last-device byte, key owner and published state. A single explicit
copy callback converts at the still-integer shared SqPlus descriptor ABI.
Container APIs take true manager pointers and return borrowed device pointers.
Legacy runtime and historical fixtures temporarily retain conversions at their
integer-layout boundary; later batches remove those runtime conversions.

input_copy_contract covers owner independence, retained buffer/virtual identity,
shallow cluster targets, reserved fields, self-assignment and virtual destruction
on shrink. It and the existing stage copy fixture are compiled only.

## Batch 5: configuration and assignment

Five address-named entries become typed manager/path interfaces. Assignment
records use fields instead of integer-address arithmetic; the config handle
has scoped ownership. Keep the existing complete-record checks and safe failure
returns, the two-record file format and first-controller broadcast.

Fresh assembly at 46BC3C and 46BE6C confirms a correction to the previous
reconstruction: any in-range controller selector accesses the FIRST controller
for SetAssign/GetAssign. WaitAssign broadcasts the first record to every device.
4074C0 copies and sanitizes signed low-byte IDs on all assignment paths,
including WaitAssign. The keyboard setter addresses buttons, while keyboard
wait/get include four direction fields. Retain excluded scans 148/58/112.
The stage contract now checks first-record selection and invalid selectors;
its prior config roundtrip/excluded-key assertions remain. Compiled only.

## Batch 6: frame publication, native setup and script fields

46B9A0 becomes kinoko_input_manager_update, using genuine virtual calls for
controllers, keyboard AND cluster (46BA23), followed by the key tracker. Named
state/publication fields replace fixed offsets; retain six button counters,
four release bytes, scan 11 for digit zero and scans 2..10 for digits 1..9.

Move native device/key/cluster setup into C++ with explicit ownership and
controller-before-keyboard registration. Preserve default assignments, zero
controller behavior and the 22 registered scans. The host retains its existing
one-time guard, full 0x600-byte zeroing and explicit 12-byte SqPlus boundary.

Input class registration uses offsetof-derived fields, preserving aliases and
s1..s9,s0 order. Small explicit adapters isolate the shared SqPlus integer-slot
callback ABI from typed native receiver/path interfaces. Class registration is
now named kinoko_register_input_class; SqPlus reference handling is unchanged.

input_frame_contract checks defaults, empty-controller setup, virtual dispatch
order (including a substituted cluster), same-frame key changes, all published
fields and the null receiver. It is compiled only, never executed here.
