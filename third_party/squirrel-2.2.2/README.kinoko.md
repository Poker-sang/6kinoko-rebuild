# Squirrel 2.2.2

The include/ and squirrel/ source directories, COPYRIGHT and HISTORY are
copied unchanged from the supplied ../squirrel-2.2.2/SQUIRREL2 tree.
See COPYRIGHT and the license notice in include/squirrel.h.

The game uses the source compiler in an isolated VM to compile original ACT
scripts to the original bytecode format. Verified SQObjectPtr operations in
src/squirrel/squirrel_value_bridge.cpp also use this source. The reconstructed
VM still owns live game objects and executes scripts by default. The optional
Execute experiment remains gated by both KINOKO_ENABLE_SQUIRREL_CPP_VM and
KINOKO_SQUIRREL_CPP_EXECUTE=1; it is not a complete VM migration.

KINOKO_SQUIRREL2_ROOT may point to the supplied external tree for comparison.
ABI assertions require the original 32-bit object and VM layouts.
