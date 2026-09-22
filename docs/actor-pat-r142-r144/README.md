# Actor drawing, PAT animation and frame ownership: R142–R144

Base: R141b, source `72da95e`, handoff `bb4542a`. User requested 2–3 complete
batches before stopping. Only the final combined version is built for handoff;
each logical source batch is committed first. No game or test executable is run.

## R142: Actor drawing

Actor::Render (45F0F0), Actor::Draw (45F350) and camera projection (466320)
are expressed in actor_render.cpp. PAT frame vertices, base/working positions,
pivots, texture extents and owned appearance share one layout with compile-time
offset checks. Frame color no longer has a separate local schema.

Preserve active/visible checks, scale*256 culling extent, facing/pivot order,
degree-based rotation, ARGB multiplication by 255, blend selection and restore.
The original has two position stores: Actor translation, then camera translation.
Restore that order instead of combining offsets before addition. X uses floor,
Y uses ceil as confirmed at 46637E/466397. Null-camera and missing-texture guards
are inherited reconstruction boundaries, not claimed original guards.

Device submission is still the existing quad renderer host; this batch does not
replace the D3D device or broader map/UI renderer. Trace calls remain at the host
boundary. New render contract source covers negative coordinates, facing,
shared-frame color reset, culling and restoring blend state after failed submit.

## Evidence

Adjacent JSON files are IDA MCP decompile/disassembly output for original
`../6kinoko/6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
The x87 disassembly is used when decompiler temporaries alias the sine/cosine
results. Existing Squirrel 2.2.2-backed object and callback adapters are reused;
this work does not introduce a second VM or new script reference ownership.

## R143: PAT reading and frame construction

pat_animation.cpp owns the scoped reader and pending animation, reads the same
ordered fields (including skipped editor records), keeps the first bounds block,
links -2 continuation nodes and resolves -1 aliases in reverse order. Texture
names remain relative to the supplied resource directory; a missing texture
still occupies its resource slot. Existing 4096-count guards and partial-load
publication behavior are preserved as reconstruction compatibility boundaries.

pat_frame.cpp constructs UVs and frame appearance through the shared schema.
The original quad rotation uses Z then Y then X. Disassembly confirms Y/X use
`axis*cos + z*sin` and `z*cos - axis*sin`; the previous helper used reversed
signs for both axes. Resource lookup also rejects base+index==count, fixing the
previous one-past-end guard. This only affects malformed resource references.

New PAT contract source covers exact reader consumption, nonzero resource base,
forward alias, continuation links, signed duration, first bounds, two-axis
rotation, resource limits and cleanup of a truncated pending animation. Existing
stage fixtures retain their real-reader/VM integration entry points.

## R144: animation and frame ownership closure

Animation and frame references in ActorRecord and AnimationRecord now use real
pointer types. Continuation next/previous links, frame begin/end/capacity and
duration_total are named. Generic legacy integer map conversion is isolated in
animation_storage.cpp; callers use named bind/find/adopt APIs. Actor SetTake
still resets take/frame time before lookup, preserves the previous selection
on a missing take, and keeps the original bounds and upper-only sync clamp.

The animation list exclusively owns each animation allocation and its fixed
frame vector; each frame exclusively owns its optional appearance. Take indices,
aliases, continuation links and Actor fields borrow them. List-node allocation
precedes ownership transfer so a failed insertion cannot double-release the
PAT pending owner. Normal manager cleanup order remains textures, live actors,
nonowning lookup, owning animations, priority/iteration state.

The original +44 sum of signed frame durations is named duration_total and the
Actor copy at +220 is take_duration. Storage clear is repeatable and retains
the list host. Existing ABI wrappers and test fixtures remain; no frame or
animation ownership is transferred to the script VM.

## Completion and verification boundary

All accepted work in these three batches is implemented: Actor drawing,
PAT parsing/alias/link publication, frame texture/appearance transforms and
animation/frame ownership. The generated C retains narrow adapters, diagnostics,
and test-facing scalar reader helpers. The general D3D quad submission/device,
map/UI rendering, and the shared archive reader remain separate dependencies.

The final quiet Win32 build includes the new Actor render and PAT contracts and
the existing stage/animation/ownership fixtures. Compilation and DAT verification
are recorded separately from execution: **no game or automated test was run**.
