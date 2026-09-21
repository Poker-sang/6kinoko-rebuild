# R140: complete gameplay collision module migration

The user expanded this batch from lifecycle cleanup to the entire collision
module before handoff. R139 is the accepted baseline. Completion here means
collision implementation and field/ownership organization, not a claim of
executed runtime validation or complete migration of the whole game.

## Coverage inventory

| Original responsibility | Current implementation |
|---|---|
| 435220 + 4361C0/436290/4362F0 map rectangle scans | map_collision.cpp (R138/R139) |
| 4355F0 / 46EF40 point/event selection | collision_queries.cpp |
| 4681E0 GetChipFlag, 4682A0 IsExistChip | collision_queries.cpp |
| 4693A0 map registration and proxy pairing | collision_manager.cpp (R139) |
| 468620 proxy synchronization/candidate refresh | collision_manager.cpp |
| 468950 / 468790 map reset/weak-reference release | collision_manager.cpp |
| 4689D0 candidate collection/support association | collision_manager.cpp (R139) |
| 466E20 vertical interval constraint | actor_collision.cpp |
| 4674B0 sides, 4677C0 ceiling/corners, 467CD0 floor/slopes | actor_collision.cpp |
| 4689D0 precise move correction | actor_collision.cpp |
| 45DBD0 bounded Move steps, 45EC60 Update and bounds | actor_motion.cpp |
| 462CE0 reciprocal callbacks | squirrel/collision_callbacks.cpp |
| 462E80 pair enumeration, 462F30 immediate actor dispatch | squirrel/collision_callbacks.cpp |
| Collision callback assignment/destruction | script_callbacks.cpp existing named APIs, now using actor schema offsets |

Original ABI/script registration functions remain as thin adapters. Shared
ActorManager allocation/tree iteration, Squirrel external-object helpers,
map resource/container loading and diagnostics are dependencies, not duplicated
inside collision. The remaining C collision code consists of conversion shims,
host allocation/script bridges and existing diagnostics. No gameplay collision
loop, query, callback dispatch or movement solver remains in generated C.

## Layout and ownership

Actor positions/previous positions, velocity/inherited velocity, pitch, hit slots,
crush/free-space state, collision flags and callback VM/environment/function now
have explicit fields and layout assertions. Chip shape +34 joins the map schema.
The narrow solver uses these fields instead of numeric-offset read/field helpers.
The original numeric flag masks and shape codes are retained where inventing a
semantic label would exceed evidence. Hit arrays and the crush byte are direct
borrowed field storage; no pointer-wrapper ownership is introduced.

Manager iteration and callback candidate buffers use actual Actor pointers with
an explicit native_buffer owner. Their valid counts remain distinct from buffer
ends. Collision refresh retains high-water storage and publishes only selected
candidates. Reset clears registered map/parent lengths and actor count, releases
each owned weak reference exactly once, and preserves scratch ends and storage.
No destructor of borrowed maps or Actors is added. Null manager reset is a no-op.

LockedParent is shared by refresh, support selection and actor motion. Refresh
checks a successful lock before touching the borrowed layout; expired layouts may
already be unmapped. Actor Update retains its parent through motion, diagnostics
and detach, then releases the temporary strong reference at the original boundary.

## Behavioral boundaries preserved

- Motion queries expand by 24 pixels; GetChipFlag and IsExistChip do not.
- Visible maps are queried in registration order. HasChip's actor phase includes
  touching edges and excludes only the receiver; it adds no collision-mask gate.
- Point/event selection retains half-open local integer chip rectangles and
  first-hit selection, with scratch buffer destruction on every return path.
- Actor Move retains 8-pixel component steps; Update retains inherited platform
  motion, slope compensation, hit clearing and post-move bound reconstruction.
- Narrow phase preserves directional overlap rules, one-way/shape masks, corner
  correction and the existing double intermediate at 469151..469162. Expression
  ordering and scalar stores remain unchanged during the field conversion.
- Reciprocal callbacks re-read masks and closure after the first callback;
  failed callbacks restore stack depth and clear only the failing callback.
  Pair enumeration snapshots candidates; immediate dispatch re-reads count.
- Existing trace calls remain active even in quiet builds. Diagnostics are not
  compiled out to hide lifecycle or VM stack issues.

This batch preserves the R139 helper contracts, including defined null/failure
handling and legacy incidental return tokens. Known R138 boundaries (internal
empty-map success, missing-chip skip, widened broad-phase subtraction and ID
lookup instead of the optional cached chip lookup fast path) remain documented
in ../map-collision-r138/README.md. They are not newly claimed to match every
malformed input or undocumented original return register value.

## Evidence and validation

Fresh IDA MCP captures in this directory cover the listed original routines;
session a3eeab00, original EXE SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
4693A0/4689D0 full captures are in ../collision-manager-r139/; map iterator
captures are in ../map-collision-r138/. Squirrel 2.2.2 sqapi.cpp gettop/settop and
external-reference semantics are supported by source-sqstate-disassembly.txt
RefTable::AddRef/Release in analysis/remaining-mapping-20260920. No VM opcodes
or Squirrel ownership model were changed.

New collision_lifecycle_contract.h covers movement of map proxies, stable filtered
candidate order, capacity retention, expired parent/layout safety, invalid reset,
repeat reset, manager switching and weak/strong counts. Existing stage contracts
cover walls/floors/ceilings/slopes, jump/drop/platform inheritance, pair order,
mask mutation, immediate callbacks, GetChipFlag/HasChip and point/event bounds.
An added failed-callback case checks reciprocal dispatch, stack depth and closure
clearing. All these are compiled only. The agent runs no tests or game.

Final source commit, build result, staged DAT hashes and executable paths are
recorded in artifacts.json. Only the completed R140 batch is handed off.
