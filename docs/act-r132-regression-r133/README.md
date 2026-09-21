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
