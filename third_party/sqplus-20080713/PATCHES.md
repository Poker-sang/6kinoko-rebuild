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

* `SQPLUS_HOST_OBJECT_ONLY` gates standalone registration/variable helpers in
  `SqPlus.cpp` and root-registry type-name overloads in `SquirrelObject.cpp`.
  They require the snapshot VM bootstrap and a different VarRef ABI. MSVC
  resolves even discardable COMDAT references, so dead-stripping alone is not
  sufficient. No zero-return replacement implementations are supplied.

The object copy/assignment/Reset and CreateClass algorithms are unchanged.
The host transfers *external* references by adopting/detaching public object
handles; it never overlays an upstream polymorphic class on legacy storage.
