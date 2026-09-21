# R138: named map collision query and result storage

R137 was accepted by the user; this is not an agent-executed runtime validation.
`map_collision.cpp` now owns the cached-index forward/backward scan and hit
assembly. `map_layout_records.hpp` names the verified layer visibility and X/Y
fields; map placements, dimensions and resources use the R137 schemas. Result
buffers name begin/end/native_buffer ownership, with borrowed chip and placement
pointers. The old C functions are thin integer-slot ABI adapters. The actor
collision narrow phase and supporting-actor synthetic hit use the same append API.

Evidence: IDA MCP session `0f94247c`, original EXE SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
Fresh 4361C0/436290/4362F0 captures are alongside this file; main 435220 capture
is `../r132-root-cause/0x435220.json`. Original setup truncates layer offsets for
integer broad-phase bounds; hit coordinates retain fractional offsets. Forward
scan precedes backward scan, whose starting index subtracts the forward flag.
The first forward hit and every backward hit update the cached index. Bounds are
inclusive. Output records can overwrite existing slots without shrinking the
published end. No visibility gate or eager binding was added.

Scope is structural migration of the existing R137 internal helper, not a claim
that every original 435220 edge case has been reimplemented. Existing empty-map
success, missing-chip skip, widened subtraction, and ID lookup instead of the
optional cached chip-index fast path remain. Original optional cursor/HRESULT
semantics and invalid-index handling are outside this helper's caller contract.
No Squirrel opcode/runtime changes; its upstream source/disassembly evidence from
R137 remains applicable without new VM analysis.

Regression source in map_lazy_binding_contract.h now covers forward/backward
ordering, backward-only scanning, inclusive X/Y limits, fractional layer offsets,
borrowed result identity, output overwrite/append, high-water end and empty-map
cursors. Existing invisible-query lazy binding and missing-resource cases remain.
All test execution and gameplay validation remain assigned to the user. This
batch compiles the regression target only; it does not run tests or the game.

Build result: all Release Win32 targets compiled successfully from `7b78e0a6ca3a0cf54516ebfa7088b21ad8a0278a`.
Quiet executable: `runtime-builds/map-collision-r138-quiet/kinoko_retdec_rebuild.exe`.
Three DAT files were copied beside it and verified by size and SHA256.
`artifacts.json` records binaries, hashes, build logs and the compile-only boundary.
