# R132: original ACT constructor and virtual clone chain

**Withdrawn after user regression report:** R131 was confirmed normal; R132
lost backgrounds and played music incorrectly, making the game unplayable.
R133 restores R131 runtime code. See ../act-r132-regression-r133/README.md.
The build-only results below do not establish runtime correctness.

Scope: replace native Clone's bulk record copying with the original 427950
constructor and resource/layer clone virtuals. R130 is user-confirmed working;
R131 has not been explicitly confirmed. No tests or game are run by the agent.

## Evidence

Input/import evidence is retained at ../act-association-r130/input-imports.json:
original C:/WorkSpace/6kinoko/6kinoko.exe, SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 8fe11ad8 was queried directly for the captures in this folder.
The supplied RetDec body is src/decompiled/6kinoko.exe.c:56034; the maintained
6kinoko_rebuilt.c has the migrated entry and real vtable. Lost receivers in
RetDec are resolved from IDA assembly, not inferred from stack temporaries.
Squirrel 2.2.2 source/object disassembly under analysis/remaining-mapping-20260920
remains auxiliary evidence; no VM implementation is modified.

- 427991/4279AC allocate and construct CAct. 427A78..427AC0 copy only timing,
  dimensions, name, offsets, margins and visibility. Resource path and suspended
  flag remain constructor defaults; opaque bytes are not copied from the source.
- 427AE4/427B01 GetText then SetText on the new document script. 415EA0 returns
  the compiled marker with E_FAIL for compiled input, and the caller ignores
  that status. 415F60 clears the filename, replaces the buffer and marks dirty,
  without changing the compiled flag. The new document keeps flag zero.
- Resource clone dispatch is +0x24 (427B2F); layer clone is +0x14 (427C07).
  Source null entries are skipped, not copied as holes. Returned objects are
  appended and indexed before advancing to the next source entry.
- 41EA50 uses 41ECA0 assignment, including script payload assignment 416700,
  then GetText/SetText. Its script compiled flag is therefore inherited, unlike
  the document. Script payload assignment leaves callback records alone.
- Layer key clone uses +0x14 at 41EC22, then the copied layout gets SetLayer
  through +0x18 at 41EC6D, before resource reassociation at document 427DAD.
  There is no final document-wide layout binding pass.

## Changes

The document clone uses the existing constructor and typed DocumentView fields,
typed resource/layer virtual returns, owning document and pending-object scopes,
and R131's ID-based associations. Its public C ABI now uses document pointers;
integer conversion occurs only at the existing runtime/diagnostic boundary.

Removed the private raw-copy clone hierarchy and retdec_act_bind_cloned_layouts.
Map, string and 2D layouts now use their existing concrete virtual clone methods,
including the map clone's full cache copy rather than the removed cache-reset
special case. Texture resources use their original borrowed flag plus the native
store's retained reference bookkeeping. No new final layout rebind is added.

ScriptTextRecord names the text fields and keeps actual buffers as pointers.
Shared helpers implement 416700 and GetText/SetText with the distinct document
and layer compiled-flag behavior. Callback and Sqrat ownership remains in the
existing constructors and layer assignment implementation.

Existing native exception-to-null handling and failure cleanup are retained,
including rejecting null virtual clone results instead of reproducing the
original invalid dereference. Native STL representations and allocation-failure
behavior are not claimed identical to historical MSVC. Layer assignment still
omits the original temporary shallow list assignment immediately cleared before
deep key cloning; this change restores its observable clone/bind dispatch chain.

## Regression sources

act_virtual_clone_contract.h links the real document/resource/layer/key/layout
path. Probes require resource -> layer -> key -> layout -> SetLayer -> SetResource
order, exactly one early layout bind, source-resource visibility at bind time,
null-slot compaction, constructor defaults, deep document name, unchanged source,
and both document/layer script compiled-flag behaviors. Existing association,
map/string layout, timeline, texture and chip ownership contracts remain.
All contract targets compiled successfully; none were executed in this batch.

## Delivery

Source commit: 335d71d946ae9e4b8e04cda28560affa814333f3.
The independent Release Win32 quiet build completed successfully for all targets.
EXE: runtime-builds/act-virtual-clone-r132-quiet/kinoko_retdec_rebuild.exe.
tools/stage_dat.ps1 copied all three DAT files beside the executable and verified
their sizes and SHA256 against the original. Exact artifact hashes and log paths
are recorded in artifacts.json. No game or automated tests were executed, and
R132 has not yet received user runtime verification. Previous artifacts remain.
