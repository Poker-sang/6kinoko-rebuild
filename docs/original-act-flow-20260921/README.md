# R128: restore original ACT loading and destruction flow

Scope: PR #9 ACT behavior audit, following the user's instruction to withdraw
unsupported behavior. Typed interfaces are retained. No local tests or game run.

## Original evidence

Fresh IDA MCP captures in this directory: 427530, 428000, 428150, 4289C0,
450020, 450D80, 4513F0, 455E40, 465F70, 466100, 46F6D0 and 41EF20.
The original image SHA256 is
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
The prior review records x86 survey/import evidence; the same original file was
opened through the IDA skill script in session 009eb21c. Source at 4EB488 starts
with a zero byte: the caller passes an empty prefix, not the text "Source".
Squirrel remains source-backed; the retained Squirrel 2.2.2 source disassembly
under analysis/remaining-mapping-20260920 remains applicable. No VM change.

## Behavioral changes

- 427530: remove whole-document zeroing; initialize documented members only.
- 428000: restore payload virtual dispatch through slot 4, AL result semantics,
  and ignore the seek return as original does. Magic/offset reads and version
  value remain checked. Version is initialized to zero to avoid an undefined
  read for a truncated input; no claim of exact malformed-stack behavior.
- 4289C0: replace the broken lost-receiver C body with explicit C++ receiver,
  prefix assignment/reuse, exact QueryType order (2D/target/mesh/chip), virtual
  Load or Create(width,height), failure accumulation and callback end reload.
- 428150: remove eager texture/chip loading and chip-load rejection from parsing.
  Resource loading now occurs after parsing/binding, once, at the original
  stage 46618A and map 46F76E sites. Both ignore resource-pass failure as original.
- 466100: remove the failed-load rejection and whole partial-stage unwind owner.
  46F6D0 has an actual failed-load branch, so the map rejection remains.
- 465F70/450020: remove pre-delete source/active pointer detachment and destructor
  source-holder read. The runtime owns its active clone; its destructor deletes
  that clone without consulting an already freed source holder. Virtual source
  deletion still precedes runtime cleanup, which rereads owner.runtime.
- 450D80: clear the real command vector. Updating the historical end slot cannot
  clear elements after the native-container migration.

## Cleanup retained with new evidence

Reader RAII is supported by unwind state 0 -> 4D0C60 -> 407350; failed-open
non-null reader deletion is explicit at 428052/42805A. Runtime constructor raw
storage deletion is unwind state 1 -> 4D1220. Stage construction has two raw
allocation-delete actions (4CBE80/4CBE8B), not partial-stage destruction. Document
factory now guards its constructor allocation accordingly. /EHsc- remains for
C-linkage C++ unwinding; this does not add a whole-stage failure cleanup policy.

## Regression sources / limits

Added isolated actual resource-pass contract: genuine virtual endpoints, order,
prefix reuse, failure accumulation, unknown type and callback end mutation.
Updated document initialization/header, stage owner, file-lifetime and clock
fixtures to the recovered behavior; removed assertions requiring invented
partial-stage unwind or borrowed active-document ownership. Sources are retained
and will be compiled, not executed by this agent.

This batch is not a claim that all existing parser guards, layout logic, memory
allocation failure behavior or audio/shutdown behavior have been recovered.
Historical documents remain historical; their missing-46618A and safety claims
are superseded here. Old artifacts are retained. Build provenance follows.
