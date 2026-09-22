# Mesh resources and 3D layouts: recovery in progress

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

These modules are compiled but the ACT key/resource factories are not changed.
They cannot alter existing 2D/text behavior or admit a half-connected Mesh type.
Remaining work after the decision: resource property/factory and script binding,
model handle/controller ownership, material acquisition and render buffers,
replacement texture mapping, then live Mesh/C3D factory integration and final
all-target quiet build with staged DAT files. Do not count this checkpoint as
completion of that chain or as runtime verification.
