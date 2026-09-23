# Squirrel savedata table chain, batch 52

Original evidence: IDA MCP session `779370e6`, original functions
`4722E0` (recursive read), `472820` (recursive write), `472C90` (file load)
and `472E50` (file save). The two recursive functions consume SqPlus
object references, use OT_NULL to end a container, distinguish a one-byte
boolean from four-byte integer/float, and handle arrays and tables recursively.
The writer skips null/nonserializable values before emitting a tag. The file
entries wrap a length-prefixed compressed stream in a 0x20000-byte buffer.
Squirrel 2.2.2 `SQTable::Next` and `SQVM::FOREACH_OP` provide auxiliary
context for the table iteration and reference behavior; no upstream VM code
was changed.

The four large address-named implementations in `table_serialization.cpp`
were replaced with named C++ read/write/file routines. A bounded typed stream
owns the read/write cursor, a three-word SqPlus object record carries type and
value without raw stack-address casts, and file/buffer scopes own their native
resources. Four short `function_472xxx` adapters remain because the original
C ABI registration table still refers to those addresses. Recursion now calls
named routines directly. The original tags, field order, iterator cleanup,
publication before descending into a nested container, and failure paths were
preserved; malformed-stream bounds are the same reconstruction guards as
before this batch.

Source commit before build: `4b12cc5fdbb972fddc0e658fe146d7b8215d90a1`.
Quiet Win32 Release configure/full build succeeded in
`build-runs/savedata-table-chain-quiet`, including contract compilation.
Three original DAT files were staged next to
`runtime-builds/savedata-table-chain-quiet/kinoko_retdec_rebuild.exe` and
verified by size/SHA256. EXE SHA256:
`B3E57864BCBC84A10D81C5894AF16514799E422C8833A84FD3B8BE7CC32D1A58`.
See `../act-load-continuation-20260923/BATCH52.md` for the build handoff.
No game, CTest or contract executable was run by the agent. Previous test
artifacts and the preexisting modification to
`docs/decompiler-cleanup-r126/original-stage-evidence.json` were retained.
