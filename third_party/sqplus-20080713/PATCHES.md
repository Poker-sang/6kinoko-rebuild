# Host/build adaptations to the 20080713 snapshot

Original archive members and SHA-256 values are recorded in `UPSTREAM.json`.
This is an **altered source version**, not an unmodified release. `PATCHES.json`
records the reviewed replacement hashes. No bundled Squirrel 2.1.1 VM is used.

* `SquirrelVM.h`: a scoped host-only exchange entry and thread-local borrowed
  `_VM` context let the existing explicit-VM runtime use upstream object methods
  on nested/child and independently owned thread VMs without introducing a new
  process-global data race. The snapshot's VM bootstrap/owner is not compiled.
* `SquirrelObject.cpp`: include existing Squirrel 2.2.2 private headers (with
  their precompiled-header prerequisites) instead of the absent bundled VM;
  match the declared integer-key overload and use SQInteger API output storage.
* `SqPlusFunctionCallImpl.h`: SQInteger output variables for sq_getinteger.
* `SqPlus.cpp` and `sqplus.h`: native callback declarations/definitions use
  SQInteger, the callback return type in Squirrel 2.2.2. On Win32 it is int.
* `sqplus.h`: declare string functions for standard template lookup, correct
  unused `_strcmp` spelling to `scstrcmp`, and select POSIX strcasecmp only in
  the non-Windows contract build. The game remains the narrow-character Win32
  target and does not instantiate that case-insensitive string helper.

* `SQPLUS_HOST_OBJECT_ONLY` gates standalone registration and non-scalar variable helpers in
  `SqPlus.cpp` and root-registry type-name overloads in `SquirrelObject.cpp`.
  They require the snapshot VM bootstrap, native ClassTypeBase objects and
  its string/error policies. The Win32 VarRef layout is in fact the same 20
  bytes as the recovered record, now checked member by member. Layout equality
  does not justify overlaying a constructed upstream object or changing those
  policies. MSVC resolves even discardable COMDAT references, so dead-stripping
  alone is not sufficient. No zero-return replacement implementations exist.
* `SquirrelVM.cpp`: the same host-only gate excludes the independent VM owner,
  compiler/call-state and cached root, while compiling the original stateless
  factories. Their algorithms are unchanged. Host registration keeps its
  capture-before-publication order and the pre-existing oversized-mask policy.
* `sqplus.h`: test the getVarNameTag input limit before examining the next
  character, preserving the host's 255-byte maximum read (including a bounded
  prefix with no NUL). Valid source strings have identical tags.

The object copy/assignment/Reset, factory and CreateClass algorithms are unchanged.
The host transfers *external* references by adopting/detaching public object
handles; it never overlays an upstream polymorphic class on legacy storage.

* `SqPlus.cpp`: compile the original signed/unsigned/float/bool getVar/setVar
  switch arms and expose two narrow forwarding entries. No scalar conversion
  or result algorithm is copied into the host. The host stages aligned scalar
  objects from byte storage and normalizes its constant representation before
  calling the source. Access rejection and failed float conversion remain host
  policy; native-instance/string operations still require separate adaptation.
  The setter is parameterized only on its stack handler: the host handler
  commits staged bytes immediately before forwarding Return to StackHandler.
  This preserves store-before-result-push ordering; narrowing and conversions
  still execute the original source body once. The standalone instantiation
  continues to use the original StackHandler.

## Shared declaring-base selection

`SqPlus.cpp` factors the existing typetag / `__ot` branch from
`getInstanceVarInfo` into `instanceVarPointer`; the original standalone caller
and `ReadInstanceBaseForHost` call that same source body. It does not construct
snapshot descriptors in the old byte records or reinterpret the host vtable.
The host checks metadata and converts `SquirrelError` to the native VM error
boundary. Field-offset arithmetic and static/constant bypass stay outside.

## Instance factory

The host-only gate now also compiles the original `CreateInstance` body and
`SquirrelError` constructor. Initialize the error-string pointer to NULL before
`sq_getstring`: a non-string last error must use the existing fallback rather
than read an indeterminate pointer. The host catches source factory failures,
restores the entry stack height (the source error constructor pushes an error),
and preserves the VM last error and the recovered null-wrapper result. The
successful factory's external reference transfers directly into the host record.

## Native instance, hierarchy and named-function factories

`SquirrelBindingsUtils.cpp` is now compiled for the host. The standalone VM Init
and unrelated standalone registration entries remain excluded. Its original
CreateNativeClassInstance algorithm receives the existing host void-type identity
explicitly instead of manufacturing a second registry identity. PopulateAncestry
shares its original body via PopulateAncestryWithType; normal source callers
still obtain the identity from ClassType<T>. The same reference factoring lets
the host transfer a consumed class argument to setupClassHierarchyBody, without
copying hierarchy construction into a separate host algorithm.

CreateFunction accepts an optional capture hook immediately after
AttachToStackObject. The default source API is unchanged. Production transfers
the external reference into its output record there, before the source publishes
the slot (a _newslot callback may inspect that output). The source owns typemask
formatting/checking and publication. Its SquirrelError now replaces the former
hand-written oversized-mask fallback: original 4A940F/4A9421 also throws on a
negative _snprintf result. At the C boundary, failure releases the captured
reference, restores the stack, returns failure and retains the source error text.

## Variable metadata registration

VarRef accepts an explicit borrowed root object so the embedding uses its existing
VM root instead of introducing the standalone root owner. The original constructor
delegates to this overload in standalone builds. Field initialization and the
__SqTypes lookup/create/type-name publication remain in the source constructor.
An optional publication hook copies the initialized, aligned value into the old
byte record before registry callbacks can observe it; it does not perform any
registration. Contract fixtures now use actual source ClassType objects.

Variable userdata creation and instance handler installation now use createVarRef
and createInstanceSetGetHandlers. The host supplies its ABI callbacks to the
latter; default standalone callbacks are unchanged. An optional minimum userdata
size check preserves an existing malformed slot without casting or replacing it.
Host variable keys keep the recovered 258-byte capacity. String reads use the
source getVar const-string arm; legacy string layouts are decoded by the host
and supplied as a borrowed const char pointer, never overlaid with modern STL.
