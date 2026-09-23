# Three substantive continuation batches (2026-09-23)

Original evidence was checked with IDA MCP session `459e6cfa` against the
original `6kinoko.exe`. The 2.2.2 Squirrel `SQTable::Next` and `SQVM` source
remain auxiliary evidence for host reference and iteration semantics; no VM
opcode or upstream runtime was changed.

## Batch 53: native callback argument dispatch

IDA 470DF0 distinguishes null, boolean, integer, float and other types;
471160 transfers a by-value SqPlus object to its native callback; 471880 and
471960 read a string, two or three integers and a truth value in a fixed order.
The corresponding native callback group now has named typed C++ functions for
truth conversion, string/object callbacks, multi-argument callbacks and the
float/integer dispatchers. Pointer-valued string arguments and the three-word
owning object are passed with their actual types. Registered address entries
remain short C ABI adapters. The callback's external reference is released on
conversion/no-callback failure and handed to the callee on success.
Source commit `cfdd1af`; fresh quiet build and DAT check:
`build-runs/squirrel-native-dispatch-quiet` and
`runtime-builds/squirrel-native-dispatch-quiet/kinoko_retdec_rebuild.exe`.
See `../act-load-continuation-20260923/BATCH53.md`.

## Batch 54: Camera, Map and root class registration

IDA 466890 resolves an instance and callback slot, then passes a three-word
by-value SqPlus argument through `__thiscall`. IDA 4669D0, 46FAC0 and 473010
show class registration order and the root table's five-word layout.
Camera/Map registration now uses a 48-byte asserted class state with three
SqPlus objects, typed receiver and callback-slot pointers, and named cleanup.
Root registration uses an asserted 20-byte Sqrat wrapper and a single named
mask-binding routine while preserving update-before-render ordering, constants,
class publication and script load. Integer conversion remains only at old C
ABI functions. First build failed on a legacy `int32_t*` hierarchy parameter;
the conversion was confined to that boundary, committed, and rebuilt in a
new directory. Failed artifacts remain.
Source commits `23bdfb3`, `35edca1`; successful quiet build and DAT check:
`build-runs/squirrel-root-camera-map-fix-quiet` and
`runtime-builds/squirrel-root-camera-map-fix-quiet/kinoko_retdec_rebuild.exe`.
See `../act-load-continuation-20260923/BATCH54.md`.

## Batch 55: MCD and ACT resources

IDA 42FAE0 replaces chip data only after the temporary load succeeds; 446BD0
loads texture suffixes after the resource's unload entry; 428150 publishes
resources before the later load pass. The MCD parser now uses an owned archive
reader and owned chip/texture data until publication, retaining the original
header guards, serialized chip shape, texture-acquire order and failure labels.
The chip-resource method retains temporary replacement and shared-data release.
The texture load/unload methods and resource factory use asserted 100-byte
chip/texture schemas for names, handles, crop and ownership fields. The mesh
resource still uses its separate reader. No eager render-target creation or
new loading pass was added.
Source commit `9874ed5`; quiet build and DAT check:
`build-runs/act-mcd-resource-chain-quiet` and
`runtime-builds/act-mcd-resource-chain-quiet/kinoko_retdec_rebuild.exe`.
See `../act-load-continuation-20260923/BATCH55.md`.

Each source commit preceded its own fresh Win32 Release build; each successful
build staged and verified the three original DAT files beside the EXE. The
last EXE SHA256 is
`6440A6270A44FF28701C6D107555C5E108384683CBC03E5AF5B98DFE43878A7C`.
No game, CTest or contract executable was run by the agent. The post-batch
lexical inventory of 154 first-party source files contains 1,848 functions:
1,336 named structured candidates, 178 named mixed legacy, 178 thin bridges,
and 156 address-named functions. Address-named function body lines fell from
1,112 before these three batches to 883; the count retains the short original
ABI addresses and is not a runtime correctness measure. The original unrelated
modification to `docs/decompiler-cleanup-r126/original-stage-evidence.json`
remains untouched.
