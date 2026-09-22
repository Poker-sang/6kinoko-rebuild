# Shared script binding completion

This batch completes the shared SqPlus/Sqrat host-interface cleanup proposed
after Actor completion. The original Squirrel 2.2.2 VM, upstream library
implementations, script-visible names, and registration order remain in use.

## Scope

| Area | Implementation and result |
| --- | --- |
| External object ownership | `squirrel_host_compat.cpp`: named construct, copy, assign, capture, reset, destroy, lookup and iteration interfaces; pointer parameters and pointer-valued results |
| Class registration | `squirrel_class_binding.cpp`: named factories and registration; class state uses borrowed VM/name/parent pointers and checked named member offsets |
| Property access | `squirrel_native_variables.cpp`: named lookup/get/set/instance-resolution paths, named stack-count/VM context; original lookup and error distinctions retained |
| Method dispatch | `squirrel_method_dispatch.cpp`: explicit receiver, captured method payload, function pointer and receiver adjustment; caller argument count determines capture location |
| Argument conversion | `squirrel_native_arguments.cpp`: strict source type checks, typed VM inputs, separate borrowed value pair and owning wrapper operations |
| Sqrat host records | `sqrat_object_bridge.cpp`: typed VM/object/callback-address interfaces, borrowed versus owning external pairs, trim-only stack restoration |
| Current VM and root cache | `squirrel_vm_bootstrap.cpp`: typed current/requested VM, root-cache ownership recovered from original destructor calls |
| Call sites | Production callers and contract fixtures migrated together; obsolete constructor, instance-factory and static-result adapters removed |

`api-mapping.json` records 110 reviewed entry points, including already named
entries whose pointer interfaces changed and removed aliases. This count is an
interface inventory, not a count of newly reverse-engineered functions.

Object storage parameters deliberately accept byte storage: the embedding has
12-byte SqPlus records, 20-byte Sqrat records, and unaligned legacy fixtures.
`ObjectView`/record views use memcpy instead of overlaying live C++ objects.
SqPlus and Sqrat references remain external GC roots; internal `SQObjectPtr`
ownership is not substituted for them.

## Original-backed behavior correction

Original `4A8DB0` destroys the cached root object with deleting flag 1 before
clearing the current VM; the previous reconstruction merely discarded its
address during VM switching. The new code destroys/releases that external
object and frees its factory-owned allocation while the outgoing VM remains
current. Same-VM selection preserves the cache. Original `4A8C50` uses the
same destruction sequence during wrapper shutdown; it is now explicit too.
Deferred owned shared states are still destroyed separately in their existing
newest-first order, and external VMs are not added to that ownership list.

The new stage-contract probe replaces the cached value with a release-hook
userdata, checks same-VM preservation, and observes both current VM and cache
identity during a switch. It is compiled, **not executed**, in this batch.

## Evidence and preserved contracts

- Live IDA MCP exports accompany this document: assignment `4A95C0`, object
  destruction `4A9D70`, cached root `4A8CC0`, VM selection `4A8DB0`, wrapper
  shutdown `4A8C50`, method resolution `460540`, Sqrat binding `415550`.
- Original EXE SHA256:
  `2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
- Supplied `../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp`: external
  `sq_addref`/`sq_release`, typetags and instance pointers. Existing source
  disassembly excerpts in `analysis/remaining-mapping-20260920/` corroborate
  the distinction from internal `SQObjectPtr` reference counting.
- Retain before release on assignment, aliasing snapshots, consuming native
  by-value objects, inherited `__ot` receiver lookup and exact argument types
  are preserved. The rectangle result still uses the original low byte.
- Failed getters preserve outputs where required; missing table setters keep
  the prior last-error behavior. Sqrat restoration only trims excess stack.
- Existing trace-bearing VM calls and diagnostic labels remain present.

## Explicit representation boundaries

Variable metadata is still the recovered 20-byte record: its offset slot can
hold an instance offset, a static address or an immediate constant. Value
type/data pairs likewise contain numbers as well as pointers. They are not
blindly converted to pointer types. Erased native function addresses are
converted to their actual calling convention at dispatch. Generated C and
fixed-layout fixtures retain explicit integer-slot conversions at their edges.
Lower VM diagnostic/legacy ports and game-specific ACT resource records are
separate existing interfaces, not a new interpreter or new gameplay behavior.

## Verification

Only compile/link and resource staging are authorized for the agent. All game
and contract execution is left to the user. See `HANDOFF.md` for the delivered
source commit, build, executable hash and DAT verification. Failed/intermediate
builds are preserved in separate directories.
