# R133: withdraw the R132 runtime migration

User report: R131 works; from R132 the background disappears, music playback
is incorrect and the game is unplayable. This is user runtime evidence. The
agent has not run the game or automated tests.

R132 changed constructor initialization, resource/layer/key/layout virtual
dispatch, script payload/text copying and layout binding timing together. Its
successful build did not validate integration with the existing runtime.
No single root cause has been established, and this recovery is not claimed
to prove original-behavior parity or fix one specific audio/render function.

## Recovery

Restore every R132-modified production/build/caller file to R131 source commit
5efa9a4ace97b158ad68128b8ce60681ee1c3fc4, including its existing ID-based clone
associations. No extra background, music or script fallback is introduced.
The two R132-only implementation files are retained under
analysis/act-virtual-clone-r132 and removed from the production build.
tests/act_virtual_clone_contract.h is retained as unexecuted experimental
regression source; it is no longer included in stage_contract.c because it
asserts the withdrawn R132 path. All historical EXEs and evidence are retained.

Fresh IDA MCP session 7a424fea (standard open script used a temporary copy of
the same original EXE) captured 450950, 41F580, 415FD0 and GetText/SetText
assembly. These confirm original virtual clone invocation and the text-copy
semantics but do not establish which R132 integration change caused the user's
symptoms. Input/import anchor remains ../act-association-r130/input-imports.json.
Do not claim that the original clone is wrong; the reconstructed replacement
must be checked together with its callers, script loader and resource lifetime.

## Follow-up boundary

Use R131 as the last user-verified runtime baseline. Reintroduce constructor,
script, and virtual dispatch changes separately after tracing the affected
runtime interactions. The R132 compiled-only contract cannot serve as runtime
verification. No local tests or game runs are authorized for this batch.

Static follow-up lead: original 45099D invokes source document slot +0x20
(428BD0) before cloning. That method restores suspended resources using the
source path when +204 is set. The current begin-stage reconstruction omits
this call. This is not established as the cause of all R132 symptoms and is
not changed in the recovery build.

## Delivery

Source commit 24535158624e1ff683870e4c2db112fce6985006. All Release Win32 quiet
targets built successfully. A name-only comparison of CMakeLists.txt, src and
include against R131 source 5efa9a4 returned no differences.
EXE: runtime-builds/act-recovery-r133-quiet/kinoko_retdec_rebuild.exe.
The three DAT files were staged beside it and size/SHA256 verified. Exact
hashes and logs are in artifacts.json. Tests/game were not run; the rebuilt
R133 executable is not yet user-verified. This is a rollback delivery, not a
confirmed root-cause repair of the withdrawn migration.
