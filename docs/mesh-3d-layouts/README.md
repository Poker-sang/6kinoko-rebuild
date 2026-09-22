# Mesh resources and 3D layouts

> **ORIGINAL BUG FIX / 原版缺陷修正：** 用户已授权修复控制器编号被误作模型
> 指针的问题。见 [醒目标记与证据](ORIGINAL-BUG-FIX.md)。以下 R1/R2 是历史
> 检查点；R3 接通 ACT 的 Mesh/C3D 运行链，仍不声称游戏实际使用了它。

Batch directories use English names, starting with `mesh-3d-layouts-r1-quiet`.
This is an implementation checkpoint, not a completed Mesh runtime handoff.

## Original evidence

IDA MCP database `6bfd4d9c`, opened from the supplied original
`C:/WorkSpace/6kinoko/6kinoko.exe` (temporary copy `10d5135a-6kinoko.exe`).
The same original's SHA256 is
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
JSON files preserve fresh decompilation and selected assembly; addresses in file
names can be inside a function, so the actual function start in the JSON is
authoritative. No game or test executable was launched during this investigation.
The supplied C reconstruction retains address markers for these disconnected
methods; original IDA evidence supplies the missing bodies. No Squirrel VM
implementation changes are involved; the existing 2.2.2 source VM is retained.

## Blocking original discrepancy

- `44C990` embeds a CMeshController at resource +92. `457040` constructs its
  root controller node and finally passes that node to `457280`.
- `457310` stores the borrowed model pointer at controller node +24.
- `457280` writes the integer registry index at node +20. Descendants are
  registered before their root; an empty root gets index zero, and a nonempty
  root gets the later index. This is not a pointer field.
- `44CA41` nevertheless executes `mov edx,[ecx+14h]` on that root and passes
  the result to `44CA60`. That function immediately treats it as a model pointer
  and calls its virtual type method. The argument is an index, not the model.
- Cross-references of the controller factory global `51BA78` show its original
  default factory and consumers; no alternate game-specific factory assignment
  was found to explain the field mismatch.
- The apparent correction is to use the model at +24. That changes the supplied
  original program, so it has NOT been applied or hidden as an ABI recovery.
  User clarification was requested before enabling this loading chain.
- `44C990` also returns false at `44CA50` after the loading branch, independently
  of whether a model was loaded. This is a separate original behavior, not a
  reason to synthesize a success result.

## Implemented independent work

- `mesh_model.hpp/.cpp`: named, owning C++ model tree and original MSH decoder.
  Root/ref/mesh/node types, original >=10 version rule, count-selected index
  width, vertex/normal arrays, material names, attribute groups, normal/UV/color
  layers, skin influences and bone matrices, and shape arrays are represented.
  Version 11 colors, version 12 reference-group field, and version 14 flags are
  read at the original boundaries. All allocations have explicit C++ ownership.
  The archive reader remains borrowed. No X-file loader or inferred mesh format.
- `act_layout_3d.hpp/.cpp`: typed original binding, cloning, update and draw.
  Update uses D3DX yaw/pitch/roll -> translation -> scaling, without 2D aliases
  or layer-position substitution. Draw queries the actual Mesh type, traverses
  the original render-node list, and preserves the original failure-before-state-
  restoration behavior. Per-node failure is not turned into a new draw failure.
- `mesh_layout_contract.cpp`: compile-only regression source for versions
  10/11/12/14, nested nodes and all mesh payload categories, truncation, matrix
  order, hidden updates, clone borrowing, and successful/failed draw state flow.

These modules have build targets, but the ACT key/resource factories are not changed.
They cannot alter existing 2D/text behavior or admit a half-connected Mesh type.
Remaining work after the decision: resource property/factory and script binding,
model handle/controller ownership, material acquisition and render buffers,
replacement texture mapping, then live Mesh/C3D factory integration and final
all-target quiet build with staged DAT files. Do not count this checkpoint as
completion of that chain or as runtime verification.

R1 (`19dd23a`) exposed an import-library symbol decoration mismatch in the new
contract target. Source compilation succeeded, but its link failed; this is not
a passing build. R2 preserves the DLL's stdcall ABI and explicitly aliases the
decorated import references to the undecorated symbols in the existing library.
The R1 build log and products are retained.

## R2 compiled checkpoint (2026-09-22)

- Source commit: `d52b5cf1827a8fed07a038456e142251cc5d3c3e`.
- Win32 Release all-target quiet build: exit 0, including the new contract.
- Build tree: `build-runs/mesh-3d-layouts-r2-quiet`.
- EXE: `runtime-builds/mesh-3d-layouts-r2-quiet/kinoko_retdec_rebuild.exe`.
- EXE SHA256: `5C6448363366CAAA642FD684A252112F3F664A44AC9F30C9CD9E43C3C9AF9FA5`.
- Three original DAT files staged beside the EXE by `tools/stage_dat.ps1`;
  sizes and SHA256 verified, exit 0.
- No game or contract test execution. The contract is compiled, not passed.
- The new decoder and 3D core are not reachable through the current factories.
  This artifact is a retained checkpoint, not a completed Mesh-enabled release.
  The clarification about correcting the original pointer mismatch remains open.

## R3 implementation

User authorization on 2026-09-22 resolves the historical pending question.
Fresh IDA session `84a4091c` supplies remaining constructor, material, renderer,
binding and shape-reader evidence. R3 includes:

- ACT factory recognition by original decorated-name hashes; separate Mesh
  property schema, C3D property writer, exact QueryType names and virtual slots.
- Mesh resource construction, prefix reuse, cloning by reloading, reference-
  counted model/material cache, controller reference expansion and postorder
  registry, material acquisition, render-list ownership and cleanup.
- Typed model and controller pointers; the authorized correction uses the root's
  model, not its integer registry index. Raw original addresses are not called.
- D3D managed index/position/normal/UV buffers and declaration, original first-
  layer uploads, original 16-bit renderer even for 32-bit serialized indices,
  indexed attribute drawing, borrowed multimap replacement textures, and original
  final diffuse triangle. ACT does not bind a controller to its mesh renderer,
  so this path has no added controller transforms or invented animation clock.
- Original nonconstructible Mesh and C3D script classes, LoadMesh and replacement
  texture methods, property access, object/table binding, and layout publication.
- Shape vertex/normal index arrays after the position/normal payload, missed by
  the R2 decoder, now consumed. Material versions <=15 and old/new color payloads
  are represented; automatic texture acquisition is not invented in the material
  reader (the original has a separate method for that).
- Truncated/overflowing input, cyclic external references and oversized buffer
  uploads fail rather than writing outside owned storage. These failure guards
  do not replace successful asset behavior with guessed content.

Scope is the ACT-reachable Mesh loading/drawing chain. General standalone mesh
controller animation/editor APIs are not implicitly exposed by ACT. No game or
local automated tests will be executed, per the user's latest request.
