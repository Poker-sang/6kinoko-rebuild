# R137: map layout schema, resource roles and C++ activation

R136 was confirmed restored by the user in this conversation. It is the behavior
baseline for this batch; this is not an agent-executed smoke test.

## Completed scope

1. `include/kinoko/map_layout_records.hpp` describes the verified map placement,
   chip definition, layout, owning-layer and chip-resource fields. Actual pointers
   use pointer types; opaque bytes remain opaque. Static assertions constrain
   Win32 offsets and sizes. These are memcpy-based RecordView schemas, never
   C++ objects constructed over legacy storage. native_buffer still owns buffer
   allocations; the third buffer word is its opaque owner, not VC8 capacity.
2. Resource roles are explicit. `kinoko_map_layer_chip_data` queries the layer's
   document-owned resource through virtual QueryType. `kinoko_map_cached_chip_data`
   only observes the layout's borrowed cache. `kinoko_map_query_chip_data` alone
   applies the query consumer's lazy SetLayer contract. None retains or releases
   resources. Clone suppression, SetLayer, map registration and drawing resource
   checks now share the schema. Drawing still rejects stale non-null bindings;
   query still keeps a non-null binding. Generic cached readers do not load.
3. `src/reconstructed/map_activation.cpp` owns CreateActorFromMap's record loop,
   event registration/callback loop, signed chip rectangle and spawn calculations,
   and VM stack restoration. Typed entry points accept borrowed layout/manager/
   script-object pointers. The remaining C functions are ABI conversion shims;
   ActorManager's allocator/initializer remains behind one narrow typed bridge.

The original first-key lookup rule and R134-R136 fixes are preserved. No final
forced clone rebind, scene-specific visibility, player spawn, or BGM override.

## Original evidence and ownership

IDA MCP session `d1293fe6`, original EXE SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
`field-evidence.json` records fresh narrow assembly reads of actor/event resource
queries, rectangle-query binding and layout property registration. Full original
463E60/46FD70 captures remain in `../map-activation-r136/`; Clone/SetLayer/query
captures remain in `../r132-root-cause/`.

The actor loop retains its initial record count and re-reads the buffer for each
iteration. No placement pointer survives into the next iteration across script
execution. Lookup creates one externally retained SqPlus temporary; its scoped
C++ destructor invokes the same 4A9D70 release as the old C path. Callback and
environment inputs are borrowed, and their existing C callers still release
their own references. Event callbacks keep receiver + five integers and use
kinoko_sq_call with the same flags before restoring the VM stack.

Auxiliary Squirrel evidence: `../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp`
sq_addref/sq_release and the existing `analysis/remaining-mapping-20260920/
source-sqstate-disassembly.txt` RefTable::AddRef/Release; original 4A9D70 capture
in `analysis/eight-entry-repair-20260914/original-0x4a9d70.json`. No SQObjectPtr
is substituted for an externally retained wrapper. No VM opcode changes.

Malformed event records keep R136's defined error return instead of using
uninitialized bounds from the original decompilation. Return counts, callback
order, traces and signed spawn calculations are retained.

## Verification boundary

The existing stage contract covers actor creation/data/coordinates and event
bounds with an empty render cache. The map lazy-binding source adds a deliberately
different non-null cache: creation must still see the layer resource, cached
inspection/query must see the cache without rebinding. Existing clone, timeline,
missing-resource and invisible collision-query cases remain.

Compile only: no game, ctest, regression executable, or local automated test
execution. Build Release Win32, trace disabled, in a fresh
`build-runs/map-lifecycle-r137-quiet` tree and publish to
`runtime-builds/map-lifecycle-r137-quiet`. DAT staging verifies all three original
files beside the EXE. `artifacts.json` records commit and hashes after the build.
