# Texture pipeline continuation

> Superseded status: see [2026-09-26 boundary closeout](../boundary-closeout-20260926/README.md) for restored startup/texture failure policy and new savedata file coverage. Historical batch limitations below are not the current checklist.

Base: 938d2a1 (Base Utilities R2 handoff), PR #9 branch.
Scope is the existing packaged CV2 -> bitmap -> D3D texture -> drawing chain.
Three bounded batches proceed from the data/reader interface to upload and binding.
No game or contract execution is authorized; new contracts are compile-only.

## Original evidence

Original ../6kinoko/6kinoko.exe SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP b0253035 was opened by the skill's open.ps1 on a temporary copy after
the previous listed session's worker was unreachable. survey.json identifies
the same x86 executable. Imports/Windows/D3D anchors for this exact hash are
also retained in ../render-camera-r145-r147/imports.json.
Fresh original decompilations, disassembly and xrefs are alongside this file.
The 4141E0 decompilation is shared by interior queries 414220/414440; those
files do not represent separate functions.

Squirrel remains the source-backed 2.2.2 VM. Auxiliary source disassembly in
analysis/remaining-mapping-20260920/source-disassembly-excerpts.json was consulted
for context; this batch changes no VM operation, receiver, trace or refcount.

## Batch 1: CV2 bitmap and reader ownership

414010 now uses kinoko_bitmap_load_cv2(KinokoBitmap*, const char*). Restored
32-byte CBitmapData layout names depth +4, width +8, height +12, row width +16,
encoded payload size +20, borrowed 16-bit palette +24 and owned pixels +28.
Assertions pin the schema; no integer represents the receiver or pixel pointer.
The production texture loader uses the record directly instead of a byte array.

The file header is 17 bytes, read in the original field order. An explicit
encoded size takes priority; otherwise <24-bit storage uses row_width*height*
depth/8 and 24/32-bit storage uses four bytes per stored pixel. This is storage
loading, not decoding the run-length pairs. The inherited exact-read guards,
allocation limits and narrowing behavior are retained and are not claimed as
original malformed-input behavior.

The reader has a scoped owner. The existing malloc/free pixel allocation family
is retained at the reconstruction boundary (original uses new[]/delete[]).
Header failure preserves the previous record. Valid headers publish metadata
before allocation validation; payload failure releases the old pixels and
leaves a null pixel pointer. Palette/methods are borrowed and untouched.
Bitmap's C++ scoped owner is noncopyable and frees only pixels.

bitmap_contract covers 8/16/24/32-bit stored row widths, explicit encoded size,
missing file, truncated header/payload, allocation limit and borrowed palette.
It must not be reported passed until someone executes it.

## Known remaining behavior scope

Original 40E630 chooses CV2 or loose bitmap from its mode, borrows a palette,
uses square-only device caps and calls 4141E0 for raw or run-length upload.
The inherited rebuilt uploader currently has raw 16/24/32 paths and an 8-bit
grayscale fallback. Merely typing its interfaces does not close these original
mode/palette/RLE gaps. They must remain explicit until separately recovered.

## Batch 2: texture image upload

40E630 is now kinoko_texture_load_image with a real IDirect3DTexture9** result,
typed source dimensions and scoped Bitmap/ComOwner lifetimes. The store adopts
that reference directly; no texture pointer crosses an integer ABI. Integer
casts remain only at the existing diagnostic address output boundary.
The bitmap pixels are released on every exit; a failed LockRect or invalid
surface releases the created COM reference, while success transfers it to the
caller and registration then transfers it into the texture store.

Original assembly restores two concrete omissions in the prior C uploader:
SQUAREONLY caps square the allocation after source dimensions are published,
and the existing recursive graphics lock surrounds D3DXCreateTexture. The
managed pool, one mip level, raw 16-bit pair copies and padded source row
strides remain. The existing uploader returns LockRect status and ignores
UnlockRect status; original's differing failure-return policy is not claimed
recovered here. Invalid COM objects with no methods retain the previous
non-releasable boundary rather than calling an invalid Release function.

texture_image_contract uses fake COM/bitmap inputs to cover 16/24/32-bit rows,
odd-width 16-bit untouched tails, source versus square dimensions, lock scope,
create/lock/bits failures and single reference transfer. texture_store_contract
now stubs the typed loader. Both are compile-only. RLE, indexed palette and
loose BMP mode remain outside this batch as described above.

## Batch 3: stage binding and cache lifetime

405E30's EDI texture stage is explicit in kinoko_texture_bind_stage. Original
4059C0's eight split globals become a named array of borrowed handle keys.
All ACT/map/mesh/sprite/quad consumers use the authoritative texture header;
405D60 texture-release bridges and the test-only registration bridge are retired.
Actor cleanup and glyph-atlas cleanup call the actual release interface.
Texture slots and registration now take IDirect3DBaseTexture9*, with real COM
inheritance for production consumers and explicit reinterpretation only in
fake-COM test fixtures.

Original 405E30 behavior: zero always calls SetTexture and returns zero;
unchanged nonzero returns its handle without calling D3D; changed nonzero
returns SetTexture's result and updates the key even on failure. The rebuilt
path formerly always called SetTexture and tracked only stage zero on success.
The named implementation restores the original cache semantics and preserves
the inherited missing-device/invalid-handle diagnostics. Stage bounds now
correspond to the original eight-entry cache (all current production calls use
stage zero). It does not add any new COM AddRef/Release for borrowed cache keys.

Original 405D60 clears matching keys before final texture Release. The store
now invalidates those keys in addition to its existing GetTexture/all-stage
unbind path, so reusing a handle slot cannot suppress its first new binding.
The inherited actual-device query remains a compatibility boundary: unlike the
original, it can also find direct bindings that bypassed the key cache.
Renderer reset clears the same cache after its original eight null binds.

texture_binding_contract covers stages 0/7, duplicate keys, repeated zeros,
failed-bind caching, targeted invalidation, full reset and invalid input.
texture_store_contract additionally covers cache invalidation through real
final release and handle reuse. Existing device/draw/ACT fixtures use the new
interface; they are compiled only. Runtime verification remains with the user.
