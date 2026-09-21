# R131: original ACT clone associations

R130 was confirmed working by the user on 2026-09-21; this is user validation,
not an agent-run smoke test. No game or automated tests are run in this batch.

## Evidence and scope

Original input/import anchor: ../act-association-r130/input-imports.json.
Original SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 8fe11ad8 freshly queried 427950 and captured association-asm.json.
Full decompilation: ../act-association-r130/0x427c30.json (containing 427950).
The supplied decompiled C's migrated clone entry delegates to native code.
Squirrel auxiliary source/object evidence remains in
analysis/remaining-mapping-20260920/source-sqvm-disassembly.txt; no VM changes.

427BA1/427CE3 index copied resource/layer IDs using unique insert (42A310),
so the first duplicate wins. 427D49..427DAF calls the matched resource setter
through virtual slot 0x18, then 427DB3..427DC8 replaces the shallow parent by
the referenced object's layer ID. 427DFE..427E89 similarly replaces child
slots in place. Missing IDs insert null via map operator[]; parent_id and
child count/order remain unchanged. A missing resource skips the setter.

DocumentCloneAssociations recovers these typed borrowed indices and ordered
operations. The former unordered pointer-keyed remap could throw on external
references and processed duplicate IDs differently. The final layout helper
no longer writes resource pointers directly or clears missing resources.

This is an association-only correction, not a claim that all cloning matches
the original: native Clone still copies records rather than dispatching all
original clone virtuals and retains source null array slots. Its separate
layout rebind remains necessary until that copying path is migrated. Container
storage and failure cleanup remain native implementations.

## Regression sources

Extended the production-linked association contract with duplicate IDs,
references to source objects outside the cloned array, missing references,
unchanged parent_id, child order/count, resource-before-hierarchy callbacks,
missing-resource preservation, and untouched key/layout bytes. Contracts are
to be compiled only, not executed.
