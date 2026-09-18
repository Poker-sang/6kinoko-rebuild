# Squirrel host object and native argument migration

## Baseline and scope

Continue PR #1 on `refactor/squirrel222-native-cpp-20260918` from
`588172ed69abb995752ecd0f34d7de0e7c40bf27`. The user reported successful local
playtesting of that preceding revision. That report does not validate this
new batch. `AGENTS.md` was read before making changes.

The vendored 2.2.2 interpreter/compiler/GC/standard libraries were already
connected. This batch removes another **43 recovered function definitions**
(36 host object helpers, seven native argument helpers; 763 original body
lines) from the decompiled C file, rather than adding another VM or changing
only file extensions. Existing address-named entry points are retained only
where the generated game callers still need a C ABI.

## Implementation

- `squirrel_host_object.hpp`: typed views over the original 12-byte SqPlus
  object record, using `HSQOBJECT` and the actual `sq_addref`/`sq_release`.
  External reference-table ownership is NOT interchangeable with
  `SQObjectPtr`'s internal reference count. Acquire before release for copy
  assignment and stack capture, including self-assignment. Access the old
  `int32_t[3]` temporaries through `memcpy`, not C++ object type-punning.
- `squirrel_host_compat.cpp`: constructors/destruction, copy/reset, capture,
  integer/object/string keyed access, array mutation, iteration, userdata,
  delegates, type tags, native pointers and thread assignment directly use
  the supplied APIs. Restricted size/type rules and unusual return values
  remain unchanged. Begin/Next/End iteration deliberately keep their
  container/iterator stack state. Lookup-success and conversion-success are
  distinct for the legacy userdata getters. Delegate cycle errors retain
  the preexisting failure stack behavior.
- Thread assignment now uses `SQObjectPtr` temporaries and the source VM's
  reservation/push operations instead of `thread+4` reference-count writes
  and hand-dispatched Release calls. Its external wrapper reference still
  uses `sq_addref`/`sq_release`. Native callback receiver switching remains
  the existing `ReceiverScope` mechanism; no new VM is opened by a wrapper.
- `squirrel_object.cpp` and `native_instance.cpp` now call the supplied source
  API directly rather than locally redeclaring a parallel set of numeric
  legacy VM APIs. Native creation uses `sq_createinstance`, NOT a script
  constructor call. `__ot`/`__ca` publication, the excluded last class-array
  element, release-hook installation order and failure stack restoration
  remain the recovered original behavior.
- `squirrel_native_arguments.cpp`: replace VM offsets +24/+48/+52 with
  `sq_gettop`/`sq_getstackobj`, preserve borrowed versus owning argument
  extraction, and preserve STRICT native integer/float/string requirements
  and original error strings. The source numeric getters allow coercion;
  the game's native binding does not. Captured userdata remains the final
  stack slot after the user arguments, not a guessed fixed slot.

## Concrete safety fixes

1. The old integer-key string setter `function_4a9730_this` dereferenced its
   integer key as a pointer repeatedly while preparing debug arguments,
   including when logging was disabled. The replacement logs the integer
   value, never reads memory through that key. The contract uses key 1.
2. `retdec_native_target_from_userdata` read an uninitialized payload pointer
   after a failed `sq_getuserdata`, then could dereference it in diagnostics.
   The output now starts as null and diagnostic dereference requires a
   successful conversion. An empty stack returns failure before attempting
   `sq_getuserdata(-1)`. Callback payload words are copied without unaligned
   integer-pointer dereferences.
3. Positive stack-pair helpers explicitly reject out-of-range indices before
   calling `sq_getstackobj`: the vendored 2.2.2 API does not validate indices.
   Valid arguments retain the same values, ownership and stack behavior.

## Validation contract

The new `squirrel_host_object_contract` executes the real source VM; only
host services (vtable identity, tracing, allocator, native type identity) are
small test implementations. It checks eight complete repetitions of:

- external ownership, copy/self-assignment/reset/destruction and exactly-once
  release hooks; unaligned legacy wrapper buffers with canaries;
- integer-key regression, three key/value setters/getters, missing/wrong
  types, array append/reverse/count and iteration stack lifetime;
- userdata/tag output semantics, normal versus raw delegate lookup,
  delegate capture/clear and native instance/type-tag behavior;
- child-VM bytecode calling the migrated host helpers, host receiver restore,
  stack expansion during thread assignment and weak-reference invalidation;
- actual native closures with captured userdata and multiple script
  arguments, strict numeric typing, original errors, borrowed/owning pairs,
  invalid-index and failed-userdata paths;
- native instance `__ot` mapping, original `__ca` last-element exclusion,
  no script constructor execution, native release exactly once, and missing
  class failure with stack restoration.

The Windows x86 quiet and diagnostic CI selection includes this test along
with all eleven existing asset-free contracts. Each run retains its source
revision, build/test logs and executables. Actual results and the exact tested
PR merge revision are recorded in the PR after the run finishes; this source
report does not claim an unexecuted test passed.

## Unchanged and remaining work

No archive or DAT loading order, current-directory policy, paths, stage/game
logic, frame timing, physics, assets, or vendored interpreter behavior is
changed. No source/library download is required.

This is not a claim that the entire C/assembly migration is complete. The
eight remaining inline assembly blocks in the main C file and the two
caller-frame/x87 compatibility boundaries are unchanged. Unrecovered
receiverless variants and additional game/codec/CRT/host-binding C functions
remain. Their signatures and lifetimes must be recovered before removal.

No original EXE/DAT assets or interactive Windows debugger are available in
this execution environment. Asset-free tests do not prove gameplay or visual
parity. The user's previous gameplay report is baseline evidence only.
