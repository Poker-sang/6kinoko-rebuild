# World-two Stage-5 hidden green platform

Scope: user's w2 Stage 5 hidden-map platform, preserving original scripts and loading logic. Carry identity/import/source auxiliary evidence from previous reports; original IDA MCP session f7e2c558 opened through the skill scripts. No platform/map-specific movement workaround.

The original w2-c05b.act contains green rail chip 1207 at (288,448). lift.cv4 InitRail receives that ID and selects take 1833, speed 1.5, normal CallbackRail/SetRide. The new offline fixture reads the actual hidden-map ACT/MCD rail and terrain layers, original lift script and item/player PATs. It drives the real native map chip query and actor collision/motion passes with a minimal standing rider. Both terrain and lift collision masks are enabled, to cover interactions absent from a platform-only fixture. Existing orange-platform fixture shares unchanged setup.

Commit this baseline before tests. Check support identity and carried displacement; derive fixes from original native assembly if it fails.
