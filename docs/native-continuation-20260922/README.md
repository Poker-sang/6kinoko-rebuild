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
