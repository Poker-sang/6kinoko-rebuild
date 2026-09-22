# Texture pipeline continuation

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
