# R139: map registration, collision collection and support association

R138 was accepted by the user. This batch completes the next bounded collision
flow: register a map and its proxy, query actor bounds against maps, collect actor
candidates, resolve with the existing narrow phase, and associate map support.
It does not claim that the remaining collision update/reset or narrow-phase
offsets have all been migrated.

## Implementation

- `collision_records.hpp` describes the manager, layout/hit/cumulative-end buffers,
  borrowed candidate actors and paired weak references. Pointer fields are typed.
  The third buffer word remains the reconstructed native_buffer owner. RecordView
  schemas do not construct new objects over old allocations. R138's short event
  scratch buffer still accesses only the hit field, never the full manager record.
- `collision_manager.cpp` owns registration, query collection and support lookup.
  Map and weak-parent entries move to the front together, with one weak retain for
  the new entry. Existing entries move without changing their reference counts.
  New hit-end slots are zeroed; visible map queries update cumulative counts.
- Actor schema names update/collision masks, bounds, hit index and scan caches.
  Flag +20 remains `registration_flag20`: its write is proven, but no speculative
  semantic name is assigned. The +480 scan cache remains distinct from +512's
  GetChipID cache. No new layer-count restriction is introduced.
- Support locking uses a scoped temporary strong reference. Setting a script
  parent consumes the same copied external SquirrelObject as before. Thin C
  bridges remain for the old ActorManager allocator and script wrapper consumer;
  the already migrated narrow phase is unchanged.

## Evidence / compatibility boundary

Fresh IDA MCP captures: original 4693A0 and 4689D0, session `a3eeab00`.
Original EXE SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
4693A0 establishes placement coordinates, proxy bounds/flags and paired rotations.
4689D0 establishes visibility gating, 24-pixel query expansion, +480 scan cursors,
cumulative layer hit ends, actor group/mask and horizontal candidate filtering,
and the strong lock around final support association. Squirrel ownership follows
the existing 4606D0 consumer and R137 source/disassembly evidence; no VM change.

This is a structural migration of the R138 caller contract. It preserves prior
allocation-failure handling, query failure returns, group order, disabled-layer
handling, and support selection. It does not reinterpret old unresolved edge
cases as new original behavior. Registration pre-reserves storage and publishes
entries after successful proxy creation, exactly as the R138 helper did; original
STL allocation timing is not newly reproduced. Reset/update remain the existing
C implementation and use the same state layout and ownership.

## Compile-only verification

Existing stage contracts cover movement, landing/jumping, moving supports, expired
parents and query caches. Added source assertions cover paired front insertion,
buffer lengths, proxy flags/bounds/priority, coordinate initialization, support
identity and absence of leaked temporary strong references, and two-map cumulative
hit grouping. Borrowed stack layouts are retired before leaving test scope.

The user requested one complete batch before testing. No intermediate executable
was handed off. All tests/game execution remain user-owned; build success is not
test success. Final source commit and hashes are recorded in artifacts.json.
