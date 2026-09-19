# Native ACT continuation

Source checkpoint: `80fd305f7abe9468da4910d45105c4c1919739c8`.
Starting point for this continuation: `644b75ab29cb62264f665181fcc8b85bbb9d9933`.

The ACT extraction itself adds 5,875 lines and removes 5,057 lines (10,932
changed lines), independently of the earlier unreachable-code removal in PR #4.
106 live definitions (5,008 original function-body lines) moved into six C++
translation units: lifetime, document loading, containers, layout, map rendering,
and Squirrel class/resource binding. The C-facing ports retain original names;
read-only host symbols retain original vtable identity and resource readers.

Changes include native Direct3D C++ dispatch, explicit typed Win32 pointer/field
adapters, C-linkage headers shared with the remaining host, and typed access to
the existing source-backed Sqrat boundary. This does not yet eliminate every
numeric function name or every legacy object-layout offset.

The checkpoint generator verified SHA256 for every changed source/header and
CMake file against the reviewed local files before committing. Local source
boundary check passes (191 source/header files, no inline assembly/naked entry,
original reference unchanged). Linux syntax probes are not Windows execution
validation; Windows results will be recorded after CI finishes.

Known limitation: the unresolved `_memcpy2` adapter retains its historical
operand-selection heuristic. ABI forwarding and deterministic selection are now
separate contracts; neither proves that every unresolved call site supplies
correct operands. No original EXE/DAT assets are present in this environment,
so asset-free tests cannot establish gameplay parity.


## Continuation checkpoint

The ACT property parser and byte-offset property mappings are now isolated in
`src/reconstructed/act_properties.cpp`. The asset-free parser contract links
that unit directly, so it no longer drags unrelated render/texture host symbols
into the test executable.

A further native-callback batch moved 16 Squirrel argument/closure adapters out
of `6kinoko_rebuilt.c` and into `squirrel_native_calls.cpp`. Truthiness,
string/integer/float conversion, stack access and object ownership now go
through the vendored Squirrel 2.2.2 API. Legacy ABI names remain only at the
C boundary while callers are incrementally migrated.
