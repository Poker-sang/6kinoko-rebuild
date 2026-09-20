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
