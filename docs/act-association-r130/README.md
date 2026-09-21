# R130: original ACT layer/resource association phases

Baseline: R129 d61ea32 (build record fa25c50). Requested scope: 428150 layer,
parent and resource association. No local game or tests will be executed.

## Evidence

IDA MCP session 8fe11ad8, opened by the standard skill scripts. Input/import
capture pins original SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports: Windows/USER32/GDI32, WinMM, Direct3D/D3DX, IME and COM; no conclusion
about dynamic imports. Full 428150 capture is in ../original-act-flow-20260921.
New captures record 4296A0 (map operator[]), 42A310 (unique insert), 42A800
(node payload), 455FD0 (parent attachment) and relevant call-site assembly.
The 427C30 capture belongs to 427950 clone, and 41F810 belongs to 41F800 layer
reader. 41F710 is an exploratory capture of 41F580 registration, not parse evidence.
Squirrel 2.2.2 source/object disassembly under analysis/remaining-mapping-20260920
remains the auxiliary VM evidence. This batch does not change any VM code.

## Findings and changes

1. Original 428150 indexes each completed layer using its ID. Duplicate IDs
   retain the first insertion (42A310 equality frees the new index node).
2. 428310..428346 links parents after all layers, BEFORE reading resource count.
   A nonnegative parent ID uses map operator[] (4296A0); missing ID yields null.
   455FD0 clears parent/parentID, appends the child to a found parent's children
   in document order, then writes parent ID and pointer. Negative IDs skip this
   call. No cycle rejection is introduced.
3. The parser previously omitted this parent phase. DocumentLoadAssociations now
   owns borrowed ID indices; the parent method is intentionally limited to fresh
   parser-created layers, whose old parent is null. It is not a generic reparent
   API and must not be used on existing layers/clones. Their detach path is outside
   this scope. Current native child vector ownership is retained.
4. Original resource index is constructed after parent resolution/count reading.
   It keeps the first duplicate resource ID. Once all resources parse, each layer
   searches by signed resource ID; no match means no write and no virtual call.
   On match 428681 calls layer slot 0x18. A new typed 41EF20 implementation restores
   its lost ECX receiver, stores the borrowed resource and its ID, and rejects null.
5. The former parser final helper directly cleared missing resource pointers and
   rebound EVERY key layout. Those writes/calls are removed from deserialization.
   The per-key virtual bind at 41F8B9 remains exactly where it was, before the next
   key. The later R128 resource-load phase is unchanged.
6. The shared old helper is renamed retdec_act_bind_cloned_layouts and restricted
   to Clone::act. That implementation copies layouts without the per-key reader,
   so its rebind is still necessary. Clone behavior is not silently changed here;
   its ID-vs-pointer remapping differences remain a separate audit item.

## Regression sources

New isolated contract links actual association and native array implementations:
forward parents, child order, first duplicate IDs, missing/negative parent IDs,
self-parent preservation, signed resource IDs, missing-resource no-op, virtual
callback ordering/mutation and null-resource rejection. Layer tails stay guarded
against unwanted layout writes. Existing real-file fixture now serializes a child
before its parent and one layout; it checks actual parser parent association and
exactly one layout bind. These tests are compiled only, not executed.

Existing parser allocation/size guards, class factory coverage, exception behavior
and clone-specific mapping are not claimed fully original by this batch. In
particular native container representation differs from historical MSVC storage.
## Build delivery

Source commit: c43373bcc53507021470449316b40712f7db49fc. The independent
Release Win32 quiet build in build-runs/act-association-r130-quiet completed
successfully (all targets, including regression contracts). No tests or game
were executed; runtime behavior remains for user verification.

Executable: runtime-builds/act-association-r130-quiet/kinoko_retdec_rebuild.exe.
The three original DAT files were copied beside it by tools/stage_dat.ps1,
with sizes and SHA256 verified. Exact artifacts and log paths are recorded in
artifacts.json. Previous build and runtime artifacts are preserved.
