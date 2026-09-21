# R127: real pointer types for stage ownership and ACT source interfaces

Baseline: R126 (source 3f312a6, artifact record 71408f1). User direction:
hardcoded numeric pointer addresses should be removed; ordinary T* types are
appropriate, and true pointers should not remain int32_t merely for C ABI.
R125 is user-confirmed. No user verification of R126 is implied here.

## Typed boundaries

Stage owner fields now hold KinokoActDocument*, KinokoActSourceHolder* and
KinokoActRuntime*. The holder stores a borrowed document pointer. Stage load
returns KinokoStageOwner*, and first/next/value/append use opaque node/owner
pointers. Layout assertions still require the original Win32 12-byte owner,
4-byte holder and field positions. Views read/write legacy byte records with
memcpy; they do not overlay a modern C++ object with constructors on old data.

The generated host's g603 slot is still an integer and preserves sentinel
identity. Its conversion is isolated at the list boundary. The opaque end
pointer is only compared, never dereferenced. The historical integer return
policy of update/draw/clear is unchanged, including the empty-list sentinel.
Remaining frame/constructor ports and diagnostic output explicitly convert
pointers at their boundaries. This is not a claim that every ACT field or
project interface is already pointer-typed.

Stage cleanup uses a typed vtable pointer and an explicit thiscall deleting
destructor pointer. It still dispatches virtually, passes the original receiver
and delete flag, rereads the runtime after that callback, and preserves the
holder/document/runtime/owner release order.

## Recovered entry mapping

| Original | Typed C++ implementation |
| --- | --- |
| 455880 | kinoko_act_source_initialize(holder*, document*) -> holder* |
| 455890 | kinoko_act_source_layer_count(const holder*) -> int32_t |
| 455E40 | kinoko_act_source_create_runtime(holder*) -> runtime* |

Both runtime-creation callers consumed the temporary output pointer and passed
zero for the unused flags argument. The new API returns that pointer directly.
The allocation remains malloc/free compatible and the existing 44FDE0
constructor still initializes the same 192-byte runtime. No new failure-path
policy, VM behavior, or guessed document destructor was added.

The old three C bodies and declarations are removed. Stage loading, map loading
and map-layer lookup now call the typed interfaces. Original 455880/455E40,
466100 and 465F70 evidence is retained in ../decompiler-cleanup-r126/
original-stage-evidence.json. The fresh IDA MCP 455890 evidence here confirms
that layer count reads the holder's document and source layer span at 208/212.
Original EXE hash was matched to the active IDA input. Baseline null/reversed
span guards remain; pointer-typing is not used to silently change that policy.

Squirrel is still source-backed. No VM internals or opcode decoding changed;
the prior source-object disassembly/reference evidence remains applicable.

## Verification handoff

Existing stage contracts compile against the pointer API; cleanup fixtures
continue to exercise the virtual callback and borrowed-source detach boundary.
No local tests or game will be executed. Quiet Win32 Release build status,
source commit, artifact hashes and staged DAT verification are recorded after
the build. All prior build/run directories are retained.
