# R136: restore map activation resource lookup

## User report

R135 world-map music is normal. On entering a level, the background appears,
but the player, solid blocks, floor and enemies do not. This is user feedback;
no game or test was executed by the agent.

## Confirmed original behavior

Reference EXE SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
IDA MCP session `6ffda705`, standard script opened a temporary original copy.

- Actor creation `463E60:463F4F..463F98` reads layout+312 (owning layer), then
  layer+100 (resource), calls its virtual QueryType at +8 for CActResourceChip,
  then reads the queried resource's MCD pointer at +64. The MCD supplies actor
  dimensions, spawn offsets and the 48-byte initialization record.
- Event creation `46FD70:46FE29..46FE67` follows the same owning-layer resource
  chain, independently of the drawing cache at layout+316. Event callbacks use
  the chip rectangle to receive the correct right/bottom coordinates.
- Our ports used `retdec_map_chip_data`, which instead reads layout+316.
  R132's original clone sequence intentionally leaves this cache empty; R131's
  compatibility rebind masked these two independent consumer errors. In that
  state event creation returned before its callback and actors lost their MCD
  initialization records/position adjustments.
- Rectangle query `435220:435243..43526B` lazily calls SetLayer through +24
  on an empty cache and then reads it again. R134 fixed the position-query
  wrapper but missed the rectangle-query port used by collision and events.
  Full prior evidence: `../r132-root-cause/0x435220.json`.

`resource-access-asm.json` contains narrow assembly evidence for all three.
`0x46f6d0.json` confirms LoadMap's immediate BeginStage before script-side
layer registration. Existing static `analysis/green-platform-stage5-20260913/
loadstage-bytecode.txt` shows LoadStage registering collisions and events before
CreateActorFromMap. Background render-layer creation occurs earlier.

## Implementation

- Added a borrowed, typed layer-chip lookup that follows the actual virtual
  QueryType call. Actor/event creation uses it without populating render caches.
- Added a separate query-chip lookup preserving original lazy binding. Both
  position queries and rectangle/collision queries use this common prologue.
- Kept generic cached-chip lookup and PreArrangement semantics unchanged.
  No visibility override, forced player spawn, BGM override, or final clone
  rebind is introduced. R135 layout Register and R134 drawing lazy binding stay.

## Regression sources and delivery

The stage contract now creates actors and invokes event callbacks with the
render cache explicitly empty but the layer resource valid; it checks existing
spawn coordinate/MCD-data and event-bound assertions and verifies that neither
creation path primes the cache. The virtual-clone contract additionally queries
collision rectangles on an invisible freshly cloned map and covers missing
resources. These sources are compiled only, not run.

R136 is built as Release Win32 with trace disabled in independent directories
`build-runs/map-activation-r136-quiet` and
`runtime-builds/map-activation-r136-quiet`. Three original DAT files are staged
beside the EXE and verified by size/SHA256. See artifacts.json for the exact
source commit and outputs. Compilation is not a runtime-success claim.
