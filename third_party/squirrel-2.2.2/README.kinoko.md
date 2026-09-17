# Squirrel 2.2.2

The include/ and squirrel/ source directories, COPYRIGHT and HISTORY are
copied unchanged from the supplied ../squirrel-2.2.2/SQUIRREL2 tree.
See COPYRIGHT and the license notice in include/squirrel.h.

The build directly compiles all 12 core squirrel/*.cpp files into a static
library. ACT source-to-bytecode compilation uses an isolated compiler VM;
sq_compile/sq_compilebuffer/compilestring also use the source compiler on the
current game VM, preserving its constants, enums and error callback.
Verified object, stack, array, thread and other operations are accessed through
our separate src/squirrel/ bridge files. The reconstructed VM still executes
game scripts by default. The optional
Execute experiment remains gated by both KINOKO_ENABLE_SQUIRREL_CPP_VM and
KINOKO_SQUIRREL_CPP_EXECUTE=1; it is not a complete VM migration.

KINOKO_SQUIRREL2_ROOT may point to the supplied external tree for comparison.
ABI assertions require the original 32-bit object and VM layouts.

This README is project documentation, not an upstream source file. On
2026-09-17 all 39 vendored upstream files were compared byte-for-byte with both
the supplied source tree and squirrel_2.2.2_stable.tar.gz: no differences.
Per-file SHA256 results are in
analysis/function-inventory-20260917/squirrel-source-verification.json.
