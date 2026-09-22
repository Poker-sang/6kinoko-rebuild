# Native continuation (eight batches)

Original SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 62a98cd6. Existing import inventory for the same hash:
../sprite-math-20260922/imports.json. Squirrel source disassembly consulted at
analysis/remaining-mapping-20260920/source-disassembly-excerpts.json; the selected
native string/container operations are not VM semantics. No VM changes planned.
All builds are quiet Win32 Release; no game or automated test is executed.

## Batch 1: append receivers and borrowed substring sources

4038C0/403BF0 now take actual, possibly unaligned string-record pointers and
return the receiver pointer. StringView retains snapshot-before-mutation for
self-aliasing, substring clamping and original overflow guards. Source record
is const and borrowed; neither the view nor return value owns storage. Existing
unaligned/alias/overflow contract cases now call the pointer interfaces directly.

## Batch 2: string capacity and borrowed growth buffer

4039E0/403CE0 take actual storage pointers; growth returns char* all the way
through StringView, with no integer round trip. The pointer is borrowed from
the native string, not an allocation for the caller to delete. Reserve keeps
zero-request clearing, small shrink truncation and the existing invalid-size /
allocation-failure guards. Modern STL capacity policy remains intentional;
this does not claim VC8 allocation sizes or exception behavior are identical.
Existing capacity/shrink/null/overflow contracts are compiled only.

## Batch 3: string ownership and remaining public interfaces

Assign/data/destroy now use actual record pointers; assignment returns the
receiver and data accepts const borrowed storage. The two active owner fields
are named and accessed through unaligned-safe RecordView, replacing +4 pointer
loads. ACT, text, script properties and fixtures use the authoritative header.
Legacy integer-layout callers convert only at their record boundary; native
storage callers pass pointers directly. Native std::string owns characters;
borrowed data remains invalidated by mutation. No allocation policy changes.
