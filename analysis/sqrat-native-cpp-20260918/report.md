# Sqrat objects, closure registration and native properties in source C++

## Baseline and scope

Continue PR #1 from `06ad4bf1aafd817fc5bc75305d3c1ef0ab33c047`, retaining
its earlier 43-function SqPlus/native-argument migration. The user reported
local gameplay success for the preceding reported batch; this is not evidence
of gameplay testing of these new changes. `AGENTS.md` was read first.

This batch migrates **45 recovered function definitions (844 body lines)**
from `src/decompiled/6kinoko_rebuilt.c` into two compiled C++ modules. It does
not count the Squirrel compiler/interpreter/GC already integrated by previous
commits as new work, or merely rename a C file to C++.

## Object and callback implementation

`src/squirrel/sqrat_object_bridge.cpp` implements the root wrapper, external
reference pairs, lookup, raw versus new-slot setters, native closures, offset
userdata, table creation, delegates, native registration and callback calls.

- Sqrat's 20-byte layout is distinct from SqPlus's 12-byte layout. Typed
  storage descriptions have compile-time size/offset checks; reads and writes
  of legacy buffers use `memcpy`. Vtable identity remains in the C host, not
  guessed or replaced with a newly invented C++ class vtable.
- Owning returned pairs use `sq_addref`/`sq_release`'s **external reference
  table**, not `SQObjectPtr` internal reference-count semantics. Root release
  preserves the stored VM and resets only the original fields/owning byte;
  padding is untouched. Repeated release remains safe.
- `TrimStack` pops excess values, but never pads a depleted stack. Source
  `sq_settop` would not preserve this contract. Normal/raw setters still use
  different APIs; instance and class behavior is not silently unified.
- Table/userdata delegates call source `SQDelegable::SetDelegate`. Replacing
  this with `sq_setdelegate` would change the existing cycle/type failure
  error state, so this is deliberately a source-method call.
- `function_415550_this` preserves the masked static-slot byte (which is NOT
  an argument count), payload-copy ordering, failure stack behavior, and VM
  address return even if slot publication fails.
- `function_415810_this` reads the separate callback VM/environment/closure
  record, uses the existing source receiver scope, retains `g560`'s handler
  flag and the original pop/return convention. It does not invent exception
  fallback behavior or broadly reset the stack after failure.

## Native fields

`src/squirrel/native_property_bridge.cpp` implements CActLayer, C2DLayout,
ActingPlayer and signed-short-view property access. Registration names,
offsets, order, class construction and game/resource loading stay in the
existing callers unchanged.

- A single descriptor lookup uses actual source APIs for the native instance
  and the final captured userdata. Typed field helpers cover inline versus
  aliased integer/float fields without raw VM offsets or duplicate converters.
- These properties deliberately retain Squirrel's numeric coercions, unlike
  the stricter native-argument binding migrated earlier. Boolean truthiness,
  one-byte writes, color clamping, signed-short truncation/sign extension and
  float bit patterns follow the recovered behavior.
- ActingPlayer offset 8 is an inline byte; its other entries are pointers.
  The offsets selecting booleans/floats/string/integer are unchanged.
- The old 24-byte MSVC string record is not a modern `std::string`. Inline
  versus heap reads use its original capacity threshold. Assignment still
  calls the existing recovered host allocator/copy entry. Layer versus player
  null-string behavior and the layer getter's unusual signed slot-address
  return test are retained, not rewritten on an assumption about SQRESULT.

## Concrete safety and diagnostic corrections

1. The single-optional-pair closure API now rejects capture counts above one
   before touching the VM stack. Previously it pushed at most one pair but
   passed an arbitrary count to `sq_newclosure`, which consumes that many
   stack entries. Existing valid zero/one-capture registrations are unchanged.
2. Native descriptor access rejects missing stack entries, userdata shorter
   than four bytes, and a missing setter value. The vendored 2.2.2 stack APIs
   do not themselves validate indices. Failed descriptor lookup leaves the
   output untouched and does not mutate native storage.
3. Native integer/pointer/short reads and writes no longer assume aligned
   `int32_t*` objects at byte offsets. Values are copied into typed locals.
4. The `pl`/`player` setter no longer performs an extra scripted lookup solely
   for diagnostics. That lookup could invoke `_get` and change script state
   even with quiet logging. Simple metadata traces remain at the sink; no
   global trace-call elimination or VM stack-shape switch was introduced.
5. Repeated raw dumps of VM table nodes/closure prototype fields are removed
   from these migrated helpers. They are not part of game or loading logic.

## Validation design and limitations

Two Release-enabled contracts execute the actual vendored 2.2.2 VM:

- `sqrat_object_contract`: root layout and misaligned buffers, external
  ownership and release hooks, missing lookup, normal/raw setters, no scripted
  diagnostic readback, delegate cycles and unchanged last-error, zero/one
  captures, userdata copies, class static slots, child VM dispatch, normal and
  throwing callback execution, error-handler flags and stack contracts.
- `native_property_contract`: real native closures/captured descriptors and
  source instances; direct/aliased fields, unaligned storage with canaries,
  coercion and invalid types, null pointers, signed shorts, color bounds,
  legacy inline/heap string reads, assignment ABI and malformed descriptors.
  The string allocator is a recording host stub in this focused test, not a
  test of its unmodified C allocator implementation.

They are included along with all **12 existing asset-free contracts** in both
quiet and diagnostic Windows x86 Release configurations. Results and exact
source/merge revisions must be recorded from actual CI; this source report
makes no claim that an unexecuted test passed. Every batch is committed before
execution, with separate retained logs and binaries.

No original DATs/EXE or interactive Windows debugger are available here.
Asset-free CI is not gameplay/visual parity evidence. The eight remaining
main-file assembly blocks and two x87/caller-frame compatibility boundaries
are unchanged. Lost-receiver Sqrat variants, game/codec/CRT C remnants and
additional bindings remain; the whole repository is not yet pure C++.
