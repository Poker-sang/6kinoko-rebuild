# R134: restore original lazy map binding with the virtual ACT clone chain

R132 enabled the original map Clone/SetLayer path, but reconstructed map Update
and GetChipByPosition omitted the original second, lazy SetLayer invocation.
The first SetLayer intentionally consumes the clone suppression flag and leaves
the resource cache empty. R131's final forced bind had masked these omissions.
Original IDA decompilation and assembly are in ../r132-root-cause/.

This version re-enables the original R132 constructor/resource/layer/key/layout
clone chain together with these consumer fixes:

- Update follows 434B86..434BBE: skip invisible layers; when the resource cache
  is empty, dispatch SetLayer through the actual layout vtable and read the cache
  again. Reject a remaining null cache or a cache different from layer.resource.
  A non-null stale cache is not silently rebound.
- GetChipByPosition follows its original 435220 query prologue at
  435243..43526B: bind an empty cache before reading chip data, independently of
  layer visibility. The generic chip-data accessor remains unchanged because
  not all its other callers have this original lazy-binding behavior.
- Preserve the original one-shot clone flag and the first SetLayer behavior.
  The document-wide forced rebind from the rollback baseline is removed again.

The regression source tests the actual document/resource/layer/key/map clones,
checks the expected empty resource cache after cloning, then calls Update or
the published Squirrel GetChipByPosition method without manually rebinding.
It includes invisible event queries, invisible Update, stale-resource rejection,
missing resources and unchanged source state. It is included in stage_contract
and registered as map_lazy_binding_contract for later user execution.

The changes do not add a BGM override or scene-specific behavior. The music
symptom was not independently established as an audio defect; runtime recovery
of that symptom must not be claimed from a successful build.

Build delivery: independent Release Win32 quiet R134 tree/run directory;
the exact source commit and hashes are recorded in artifacts.json after build.
No game or local automated tests are run by the agent, per user instructions.
