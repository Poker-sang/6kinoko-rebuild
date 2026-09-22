# R151: MCD mutable chip definitions and sprite caches

Base source R150 be9e440; handoff 2dc66c7. User requested the complete MCD
cache/update/lifetime batch. Original EXE SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session e7abb451 (temporary 3a7f7a84-6kinoko.exe). Metadata/import survey
for this same binary is retained under ../render-camera-r145-r147/.
No executable or automated test was run. Squirrel uses the existing 2.2.2
source-backed VM; no bytecode, VM trace call, or VM ownership change here.

## Correction of the earlier scope description

435D00/435E80 implement a sprite cache, not an autonomous animation timer.
435FD0 (SetChipRect) changes the shared MCD definition. Script calls can animate
that rectangle; the renderer flushes the changed-definition queue before visible
query, then compares all 48 bytes before reusing each cached sprite. No invented
native clock, PAT frame loop, or texture reloading policy has been added.

## Original evidence and recovered chain

- 433780: vectors +384 (288-byte cache), +404 (48-byte definitions), +420
  (borrowed changed-definition pointers), +436 (ID-to-index); count +400,
  maximum ID +452. Layout assertions cover every field and the 464-byte owner.
- 435B20: build in original ordered-map ID order, retain maximum ID unless it
  is negative, retain existing sprite entries/changed queue. Shared MCD storage
  remains separate from the copied per-layout definition array.
- 435D00: reject index zero/outside chip count; grow sprite vector to ID-table
  size; compare the entire definition; copy it before looking up texture;
  initialize sprite then set valid. Missing texture keeps invalid and retries.
- 435E80: return a cached sprite only after successful refresh and valid bounds.
- 435FD0 assembly: mutate shared rectangle BEFORE ID-vs-definition-count and
  index-zero checks. On success copy the definition and append its borrowed
  shared-MCD pointer, including duplicates. Preserve the surprising false return
  after mutation for sparse IDs/index zero. New helper uses typed ID and shorts.
- 434B60: hidden layer/missing or mismatched resource returns before flushing.
  Otherwise flush all changed chips, clear queue, then query visibility. This
  also updates offscreen changes when the visible placement result is empty.
  Cache hits use 42B3F0 payload copying; fallback uses placement texture refs
  when present, then direct lookup otherwise. Transforms/alpha affect only the
  copied output quad. Definition changes shared by another layout are detected
  on that layout's next visible lookup even without a queued notification.
- 404EE0: store texture size, source UV extents, normalized UV, white vertex
  colors and signed rectangle geometry; zero/negative dimensions are retained.
  UV endpoints add separately rounded origin and extent floats. Existing guards
  for malformed texture handles/dimensions remain reconstruction boundaries.
- 435860: one named preparation path now serves serialization and script
  PreArrangement; clear placement reference ranges, sort positions, rebuild
  index, recompute extents. Preserve the last-X initial bottom value at 4359FF.
- Existing original-backed clone/destructor now name the cache schemas. Clone
  owns independent flat buffers, borrows the same MCD/texture/queued definitions,
  copies initialized bytes only, and retains one-shot binding suppression.
  Reverse-order destruction frees buffers without releasing borrowed textures.

The earlier flat MCD loader and texture ownership are unchanged. Source MCD
textures are already acquired once; this batch adds no second texture acquire.
SetLayer retains its original cache rebuilding behavior; no speculative cache
reset is inserted at a scene boundary. Existing malformed-layout allocation
limits are retained. Original evidence, including counterintuitive bounds, is
stored alongside this document rather than inferred from reported symptoms.

## Verification source (compile only)

map_chip_cache_contract covers sorted/sparse IDs, index-zero fallback, exact
snapshot invalidation, cache hit, duplicate deferred changes, invisible-owner
queue retention, offscreen flush, mutation-before-failure, missing-texture retry,
UV/geometry/alpha isolation during real map-update calls, independent cloned
buffers with shared borrowed targets, preparation, signed/zero dimensions and
cleanup without releasing resource ownership. No pass claim before execution.

## Build attempts

- c4b70dd: fresh build-runs/mcd-r151-quiet, runtime-builds/mcd-r151-quiet.
  Compilation failed: missing Windows HRESULT definitions in the new cache
  translation unit and generic Buffer name colliding with collision records.
  Both are integration-only corrections; keep all failed-attempt artifacts.
- Next attempt uses mcd-r151b-quiet after committing those corrections.

## Successful handoff (2026-09-22)

Source commit: e943b1a163df75a6f33ffb6cf47b5ff1bc3bd44c.
Fresh Win32 Release quiet build: build-runs/mcd-r151b-quiet.
Configure and full ALL build exited 0, including map_chip_cache_contract.
Executable: runtime-builds/mcd-r151b-quiet/kinoko_retdec_rebuild.exe.
SHA256: CC5E758F41858396991D48F0A5C59320ACAB8556FA67A01EF063BA3006F2B93A.
stage_dat.ps1 exited 0: all three original DAT files copied beside EXE and
verified by size/SHA256. Logs retained in the build tree. The failed first
attempt and all older build/runtime products remain intact.
No game, CTest, or contract executable was run. This is compile/staging evidence,
not a claim that gameplay or compiled contracts passed runtime validation.
