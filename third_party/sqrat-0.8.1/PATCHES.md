# Local ownership fix to Sqrat 0.8.1

This is an altered source version. `UPSTREAM.json` retains the original archive
and member hashes; `PATCHES.json` identifies changed members. Original line
endings and unrelated source are retained.

`Object::GetSlot` used to pop the VM result before constructing an owning
`Object`. A native `_get` that creates otherwise unowned userdata therefore
released the userdata before the constructor's `sq_addref`, a heap-use-after-free.
Acquire the external reference first, then pop the two stack slots. Copy elision
is not required: the normal Object copy/destructor rules balance references.

Evidence: the committed `slot_lifetime` case in `upstream_bindings_contract`
fails with AddressSanitizer on source before this fix. It uses the real source
VM and a native `_get` with a release hook, not a fake reference counter.
The test runs on root and child VMs, including independent concurrent VMs,
with a separate `-fno-elide-constructors` build. Production's previous direct-API
GetSlot wrapper already retained before popping; fix the upstream source before
routing that production call through it, rather than introducing a regression.

No Squirrel VM, lookup, last-error or missing-slot policy is changed here.

## Optional lookup status for the legacy host

`GetSlot` additionally accepts an optional `bool* found`. The default one-argument
usage is unchanged. The host must distinguish an existing null-valued slot from
a missing key without invoking `_get` twice or inspecting stale last-error state.
This exposes the status already computed by the upstream method; it does not
change lookup, error, stack or reference behavior.

## Function execution with an explicit host context

`Function::Execute()` delegates to `ExecuteWithErrorHandling`, which contains
its original push/call/pop algorithm. The latter accepts the recovered per-call
error flag and an optional call entry (default `sq_call`). Production supplies
its existing receiver-scoped call entry. This avoids changing a process-global
Sqrat error setting across nested or concurrent VMs. No second hand-written
callback execution body remains in the host. The host borrows Function handles
in a normally constructed stack object and detaches them before destruction.

## Value publication and accessor payloads

BindFunc and BindValue expose the status of their existing slot publication;
callers that ignored the previous void result retain the same behavior. BindValue
accepts a publication entry defaulting to sq_newslot; the recovered raw path
supplies sq_rawset. All object/key/value pushes and cleanup remain the source
algorithm. Host values are borrowed actual Object instances or strings. Offset
accessor closures now use BindFunc's source userdata/copy/closure implementation.

## Property dispatch

sqVarGet/sqVarSet delegate to their original bodies with explicit error-handling
and call-entry arguments. Defaults remain ErrorHandling::IsEnabled and sq_call.
The host supplies the recovered scoped call entry and error byte, so nested VMs
do not mutate a shared global. ClassWeakref is invoked directly from a source
Class specialization without constructing a second class registry.

Including Class on current MSVC requires qualifying two dependent-base VM
references in DerivedClass with this->; no runtime algorithm changes.
