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
