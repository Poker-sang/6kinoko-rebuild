# R126: named stage runtime and record boundaries

Baseline: master 6635f26, including the R125 source build at 8ad4aaf6.
The user confirmed R125 on 2026-09-21. This batch prioritizes decompiler
artifacts over further third-party replacement.

## Address mapping

| Original | Current implementation |
| --- | --- |
| 466050 | stage_runtime.cpp: kinoko_stages_update |
| 466090 | stage_runtime.cpp: kinoko_stages_prepare_draw |
| 4660C0 | stage_runtime.cpp: kinoko_stages_draw |
| 466100 | stage_runtime.cpp: kinoko_stage_load |
| 451640 | act_frame_update.cpp: kinoko_act_update_frame |
| 4522F0 | act_frame_render.cpp: kinoko_act_prepare_draw |
| 4525D0 | act_frame_render.cpp: kinoko_act_draw |

The four stage implementations leave the generated C translation unit. All
compiled callers and contract sources use the semantic names; there are no
new aliases or old-name wrappers for these seven entries. The existing 452BA0
replacement ledger follows the renamed frame-update implementation.

## Original evidence and implementation

original-stage-evidence.json contains IDA MCP disassembly and callers from
the original EXE; its SHA256 was matched against ../6kinoko/6kinoko.exe.
The database was auto-analyzed before collecting nonempty function bodies.
Existing original import/ownership evidence in analysis/function-inventory-
20260918-postmerge remains the sample triage baseline.

466050 advances time before frame update. 466090 prepares every stage before
4660C0 draws them at (0,0). Traversal advances after callbacks. Existing null
guards and the distinct empty/null-entry return policies are preserved.

466100 allocates a 12-byte owner containing document, holder and runtime at
0/4/8. StageOwner's former void-pointer overlay and integer indexing now use
a shared OwnerRecord schema and alignment-independent RecordView. The loader
uses an allocation owner until publication, and shares the same schema with
cleanup. ACT runtime diagnostics use existing named members instead of
+8/+12/+100/+104; their historical four-byte flag snapshots and trace calls
remain, including in quiet builds.

The fixed 466208 call to 450E30 now invokes the already recovered explicit
receiver implementation directly. No function-to-void-pointer cast or generic
thiscall trampoline is needed for that known target. The genuine virtual
deleting destructor at 465FCC remains polymorphic: its slot is named and
statically verified, not replaced with a guessed concrete destructor. The
runtime pointer is reread after that destructor callback as before.

Squirrel execution and object ownership are not reimplemented here. The
existing root-table bridge still uses source-backed Sqrat/Squirrel. Prior
source/reference and source-object disassembly evidence remains under
analysis/remaining-mapping-20260920; no VM instruction or trace call was removed.

## Limits and next work

This is a bounded refactor of the R125 behavior, not proof of full original
equivalence. The original post-load virtual call at 46618A is absent in the
baseline and remains a separate recovery question. The baseline partial-ACT
load failure path releases only the owner record; its document cleanup policy
also needs a separate evidence-backed behavior change. The full 240-byte ACT
document and the 427530/428000/455880/455E40 legacy host ports are not migrated
by naming the stage orchestration. Those ports are useful next candidates.

Existing frame contracts now call semantic entries. The stage contract adds
empty-list/null-owner cases to protect traversal return policies. Following
the user's handoff, contracts are compiled only; no test/game is executed.
Build commit, quiet EXE, and DAT hashes are recorded separately after building.
