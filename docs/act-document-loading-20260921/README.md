# ACT document creation/loading boundary

Baseline: R127 (`825b039`), with source-snapshot bookkeeping in `95842c3`.

## Batch 1: representation and reader lifetime only

* Replace the C `427530` initializer with `kinoko_act_document_initialize` and
  `kinoko_act_document_create`. The complete 240-byte schema gives recovered
  fields names, typed vector pointers, and offset/size assertions. Unidentified
  words/padding remain explicit; the embedded 104-byte script still uses its
  existing constructor and destructor. No new C++ object is overlaid on raw or
  potentially unaligned storage: `RecordView` uses byte copies.
* `kinoko_act_document_load(KinokoActDocument*, const char*)` replaces `428000`.
  The document/path are borrowed. A scoped reader owns the existing archive
  reader from open until every normal return (and C++ exception unwind).
  Integer addresses exist only at the unconverted host reader/payload ports.
* Keep ACT1/version 1 checks, three 4-byte reads, relative payload seek, parser
  result forwarding, path handling and the four diagnostic call sites. Quiet
  builds silence the sink rather than dropping these calls.
* Update both stage loading and map loading. Map publication still explicitly
  converts to the old map-manager integer slots; document name and dimensions
  now use typed accessors. Remove both old address-named interfaces completely.
* This batch intentionally does not change partial-document cleanup in the
  stage owner or add the missing post-load virtual call.

## Evidence and boundaries

The original decompiler reference remains unmodified. Its `427530` range and
R127's recovered initializer agree on record size/defaults. Serialized names
are independently present in `act_texture_io.cpp`: resolutionMs/screenWidth/
screenHeight/stName, four margins, two offsets and visible. The string at 44
is used as the path by `4289C0`; byte 204 is the suspend/resume flag in
`428AF0`/`428BD0`. Unknown fields are not assigned invented semantics.

Captured original assembly in
`docs/decompiler-cleanup-r126/original-stage-evidence.json` shows `46618A`
calling document vtable slot 0x18 with `Source`, before allocating the source
holder. The target in the retained table is `4289C0`, whose current C body has
an uninitialized receiver. Simply inserting a call to that body is unsafe.
The current payload parser already resolves resources and binds layouts;
recovering the original method also requires checking for duplicate work and
resource ownership. This is not proven by the call site alone.

That same capture shows normal `465F70` cleanup ordering: holder, document
virtual deleting destructor, runtime destructor/storage, owner storage. It
also shows no conditional failure branch between `428000` and `46618A`.
Therefore normal cleanup evidence must not be presented as proof of an
original rejected-ACT cleanup branch.

## Validation

Production changes must be committed before building or running tests. The
new `act_document_contract` compiles the production creation/loading unit with
real native string storage and deterministic reader/script/payload host ports.
It checks unaligned guarded storage, defaults, path/seek semantics, every
truncated header prefix, bad magic/version, failed seek, payload result
forwarding, partial-document ownership and single reader close on exception.
Existing `stage_native_contract` is updated to the typed interface.

The Windows workflow selects the no-log Release variant only and now includes
this new contract. Results and binary/source hashes are recorded separately
once the committed version has actually built/run. These asset-free tests do
not establish DAT compatibility, rendering or gameplay correctness.
