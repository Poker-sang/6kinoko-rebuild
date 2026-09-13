# Orange returning platform

Scope: user's orange platform in world-two Stage 1, distinct from green platforms. Carry original identity/imports and skills from prior reports; IDA MCP 3dba13d3 survey confirms the same original EXE hash. No gameplay-specific smoothing or attachment rule is authorized.

Effective lift.cv4 and item.pat are archive-2 overrides. w2-c01c.act includes chip 1223/04C7 at (608,672), (800,608), (992,544), (1184,448). Original InitLiftBack selects take 1823 for 04C7. PAT 1823 references ws-yellow_0003.bmp with bounds (-64,-8,63,8); green images are separate takes 1830..1833 using ws-green_000*.bmp. Original UpdateLiftBack accelerates down by 0.2 while player.step is this platform, otherwise returns at vy=-2 until its saved starting Y and clears its update callback.

Original SetRide assigns Actor::SetStep and tests player velocity/bottom and platform top. Original 45EC60 carries parent displacement, resolves collisions, refreshes bounds, and clears step when bounds no longer overlap. The new offline fixture uses the original orange initializer/callback/update, original platform/player PAT bounds, a minimal gravity rider, and the manager's real callback/motion pass. Baseline is committed before execution.
