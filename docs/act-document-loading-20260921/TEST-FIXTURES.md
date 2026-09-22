# Test fixture corrections (separate from production behavior)

- The new isolated stage-owner executable links stage_cleanup.cpp, which also
  defines the render-queue startup registration. Supply its unused initialization
  stub; the first batch-two build failed only at that unresolved test symbol.
- The pre-existing actor-record test registered animation 37 but selected 38 for
  a mirrored-bounds assertion. Register 38 explicitly. Keep the separate missing
  take 999 assertions unchanged; do not alter production lookup behavior.
- The pre-existing CStringLayout script concatenated unbraced if/throw statements
  on one source line. Squirrel 2.2.2 consumes the inner semicolon and the outer
  statement expects another separator. Give these source fragments real line
  endings. Preserve every property/method assertion and leave the compiler intact.

The earlier quiet run b16b9ce passed the new ACT document contract but failed the
actor-record and two stage-script tests. These are fixture fixes, not evidence of
regressions in original ACT behavior. Retain the failing build/test artifacts.
