# Enemy stomp and stage-clear crash

Follow-up to 9620b2d. User confirms moving-actor disappearance no longer occurs,
requests that repair be committed, then reports an intermittent stomp crash
and a subsequent stage-clear crash. Carry forward skills, scope, original
SHA256/survey/imports and tools from ../moving-actor-lifetime-20260907/report.md.

The corrected diagnostic runtime (PID 23036) captured the stage-clear failure:
retdec_trace.log line 18681673, access violation at rebuilt RVA 66FD4,
fault address 4. The native array.sort call receives an array of six objects
and a script comparison closure. Linker map resolves the stack as:
491820_this <- 4A25C0 (_qsort_compare) <- 4A3120 (_qsort) <- 4A3530
(array_sort) <- script VM <- global stage update.

The prior stomp crash was in the no-log build and has not been reproduced.
Do not infer it has the same cause without evidence.

- [x] Commit the verified disappearance repair before investigating this crash.
- [x] Capture and map a real failure; compare the original sorting cluster.
- [ ] Restore confirmed array sorting/ownership/VM receiver defects.
- [ ] Run focused regressions, both builds, staged DATs and gameplay verification.
- [ ] Record and commit the next checkpoint.
