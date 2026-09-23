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

## Batch 4: texture-handle vector ownership

The 12-byte manager texture slot now contains an opaque native owner and two
reserved words, not fake begin/end/capacity integers. Construct/append/clear /
destroy take a typed slot; data returns a borrowed const int32_t array. PAT
resource indexing and manager cleanup carry that pointer directly. Handles
remain integers and the manager releases textures before clearing storage.
Clear retains capacity; the two reserved words remain untouched. Existing PAT,
actor record and stage ownership contracts are updated and compiled only.

## Batch 5: integer map owner and original iterator boundary

Native maps now expose a typed opaque owner and borrowed int32_t value slots;
find returns nullptr on missing keys. AnimationIndex and the global sound
lookup store owner pointers, while the separate actor-priority tree retains
its own schema. Original 4706C0 writes an iterator and returns its address:
kinoko_integer_map_lookup_index isolates that legacy integer output/sentinel
contract. SetTake's mixed integer return remains converted at that explicit
boundary. Animation values remain integer slots at the legacy record boundary,
not additional animation ownership. Existing alias, node-stability, missing-key
and cleanup contracts are compiled only.

## Batch 6: map layer/event ownership and borrowed records

All map-container interfaces receive real manager/layout pointers. Render-list
append/index returns stable borrowed RenderLayer pointers; event lookup returns
borrowed ActLayout pointers. Collision queries, layer creation and manager
lifecycle no longer encode these values as integers. The list owns nodes but
not layouts, preserves append order, and reconstructs method identity on copy.
The event vector retains capacity across clear/shorter assignment and preserves
null/duplicate entries. Historical raw C fixtures convert at their buffer edge.

Batch 6 R1 caught a missing public opaque RenderLayer declaration; retained
the failed build and fixed the self-contained header before fresh R2.

## Batch 7: map copy receiver and auto_ptr transfer

4701B0/470100 now converge on pointer-valued kinoko_map_manager_assign.
SqPlus alone retains an explicit integer callback adapter. The source remains
mutable because copying transfers its runtime: clear source first, dispose
the old destination only when distinct, then install the saved runtime. This
preserves self-transfer and destructor callback behavior. Source ACT/holder and
layout values remain borrowed copies; native containers own their own storage.
The map contract adds distinct-source transfer, disposal-before-container-copy,
callback replacement and receiver-return assertions. Compiled only.
