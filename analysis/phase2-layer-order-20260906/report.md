# Original render-layer ordering

## Cause and restoration

Original `46F6D0` enumerates ACT layouts, appends their names to a Squirrel
array, then calls `4A99F0` at `46F923` before publishing `map.layer_name`.
That call is SquirrelObject::ArrayReverse -> `48C400` / sq_arrayreverse.
The rebuilt loader omitted the reversal. A diagnostic branch then moved all
map render layers to the list head, leaving back-priority flying objects in
front of the chimney.

Restored the missing call with its explicit ECX receiver. Recovered `48C400`
from original IDA disassembly and Squirrel 2.2.2 `sqapi.cpp` (also checked
against `build-runs/phase2-cppvm/kinoko_squirrel_cpp_vm.dir/Release/sqapi.obj`
disassembly). It uses the original three SQObjectPtr assignments per swap,
with correct ownership of each old/new value.

Removed the diagnostic map-head insertion. `46A210` now appends every layer
as the original does. Coordinates, random velocities, rotation, timing and
the original scripts/DAT files were not changed; no replacement ordering
table or chimney-specific masking was introduced.

## Evidence and validation

Binary identity/imports and general workflow evidence are carried forward
from `../phase2-20260906/report.md`. IDA MCP sessions used here: `9fa86bd4`
and `ab9c8d5d`; the first expired before annotations could be saved.

Diagnostic trace records the script-generated order:
`bg2 -> bg1 -> bg0 -> actor_back -> terrain -> front -> actor_middle -> actor_front -> actor_water`.
It records all 26 flying-object sound events. Recorded frames show the chimney
occluding the lower parts of the objects, matching the original recording.
The user also confirmed the visual repair.

Both `p2-layer-order-diag` and `p2-layer-order-notrace` Release builds succeeded,
and each passed archive_smoke (1/1). The three DATs were copied and hash-checked
beside each EXE, and both were launched through run_staged.ps1 without a
working-directory override. The no-log recording ended with a GDI capture
error when its window disappeared; no final full no-log replay is claimed.
The user requested immediate commit instead of further testing.

Recordings/crops are in `analysis/evidence/raw/phase2-layer-order-20260906/`.
The previously deferred exit faults and title-screen work remain outside scope.
