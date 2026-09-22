# ACT special layouts: supported draw paths and type audit

Batch title: ACT special layouts. Revision R1. Directory names are English:
act-special-layouts-r1-quiet. R numbers count revisions within this batch;
no cross-batch sequence or letter suffixes. Earlier products retain their names.
Base: MCD source e943b1a, handoff dbc6a01.

## Evidence and scope

Original EXE SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 48cb210b, temporary f0d73e91-6kinoko.exe. Same-binary metadata
and imports are in ../render-camera-r145-r147. This batch uses fresh original
method evidence alongside the provided C reference. Existing Squirrel 2.2.2
source-backed VM is unchanged; no VM trace call sites were removed.

The former blanket grid/3D/rich-text backlog conflated several states. Concrete
layout RTTI found here is C2DLayout, C2DMapLayout, C3DLayout and CStringLayout.
The grid wording does not establish a separate missing layout type. Map/MCD is
already handled by the preceding batches. String loading and font atlas support
already exist. C3DLayout has a property reader but is still rejected by the
key factory, and CActResourceMesh has no supported resource-loading chain.

R1 closes the existing 2D/text update/draw chain and audits the 3D dependency.
It does NOT claim that Mesh/3D loading is implemented or silently admit unknown
serialized types. Completing that separate dependency requires the mesh resource
factory, reader/loader, render-node ownership, then the 3D factory and binding.
43C920/43CA40 prove original rotation -> translation -> scaling and the mesh-list
draw path; the latter leaves changed D3D state on failed resource QueryType.
These details are recorded for that recovery, not replaced with guessed behavior.

## Changes grounded in original behavior

- Explicit 316-byte 2D and 260-byte string schemas, actual borrowed layer pointer,
  named transforms/colors/pivots and 256-byte glyph schema. Also record the
  verified 108-byte 3D layout for the next dependency, without enabling it.
- 42BCC0/42C470 retain property aliases and one-time pivot initialization, real
  QueryType and binding only loaded textures. No file search or fallback load.
- 42C100 uses exact resource type conversion, cropped rectangle and cached handle.
  Removed invented zero-scale-to-one and nonpositive-crop-to-image-size rewrites.
  RGB uses each integer's low byte; alpha clamps after the original double
  multiplication. UV end coordinates add separately rounded start and extent.
  Scale, Z/Y/X rotation and the actual virtual world-position call remain ordered.
- 42C300 draws already prepared geometry. Removed the duplicate Update, unproved
  ancestor visibility traversal and per-quad error propagation. Preserve four
  D3D state reads, alpha enable, original blend selection, bind/submit/unbind and
  four restores. Submission failure does not change the original success return.
- Share the actual 42AC20 blend routine with text. It intentionally differs from
  402770 (transition key 32 does not write BLENDOP); modes 5/default remain.
- String update retains pending multibyte consumption before visibility testing,
  original alignment/half-height adjustment, low-byte colors and double arithmetic.
  String draw only submits prepared glyphs, restores the four original states,
  and leaves filtering set to linear. Atlas allocation/ownership is unchanged.
- Removed the unused alternative C2D update implementation after checking active
  source/header/test references. Remaining legacy entry names are ABI bridges.
- Initialize serialized layout_type before a failed read can log it. Unknown
  type handling still fails explicitly; no unsupported type is accepted as 2D.

## Verification boundary

New act_layout_render_contract compiles real 2D/text update/draw code against
fake device/resource/position callbacks and actual native strings. Covers
zero scale, nonpositive crop, low-byte colors, one-time pivots, property aliases,
no update during draw, failed submission with state restoration, own visibility,
failed resource query, special blend transition, hidden pending-text processing
and glyph alignment. It is compiled only, not executed by the agent.
