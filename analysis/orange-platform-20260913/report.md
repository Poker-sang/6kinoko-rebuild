# Orange returning platform

Scope: user's orange platform in world-two Stage 1, distinct from green platforms. Carry original identity/imports and skills from prior reports; IDA MCP 3dba13d3 survey confirms the same original EXE hash. No gameplay-specific smoothing or attachment rule is authorized.

Effective lift.cv4 and item.pat are archive-2 overrides. w2-c01c.act includes chip 1223/04C7 at (608,672), (800,608), (992,544), (1184,448). Original InitLiftBack selects take 1823 for 04C7. PAT 1823 references ws-yellow_0003.bmp with bounds (-64,-8,63,8); green images are separate takes 1830..1833 using ws-green_000*.bmp. Original UpdateLiftBack accelerates down by 0.2 while player.step is this platform, otherwise returns at vy=-2 until its saved starting Y and clears its update callback.

Original SetRide assigns Actor::SetStep and tests player velocity/bottom and platform top. Original 45EC60 carries parent displacement, resolves collisions, refreshes bounds, and clears step when bounds no longer overlap. The new offline fixture uses the original orange initializer/callback/update, original platform/player PAT bounds, a minimal gravity rider, and the manager's real callback/motion pass. Baseline is committed before execution.

## Reproduced fault and restoration

Baseline 2082898 initially omitted the map chip ID expected by Init04c7/InitLiftBack. 7271123 supplies 1223. The next fixture exposed an unestablished initial script step value, so e9c4fda starts the rider through SetStep(null), the normal detached support state. These preliminary test outputs are retained; no runtime correction was based on their setup failures.

The before3 baseline then reproduces the reported mechanism with valid original callbacks. Frames 0..2 carry correctly. At frame 3 the platform reaches Y=674, top=666, but player.bottom is 665.999939. Floor contact is lost and the Actor::Update overlap check clears step. On the next script update LiftBack sees no rider and selects its return branch. This feedback explains the alternating fall/return (jitter).

Original 4689D0 assembly at 469151..469162 loads old bottom/new top, performs fadd/fsub in x87, and only then stores the resulting bottom as a float. actor_collision.cpp evaluated old.bottom + moved.top - old.top with a rounded single-precision intermediate sum. The subsequent subtraction can undershoot the supporting floor by one float ULP even when the requested parent displacement was correct.

Runtime checkpoint bf833fc promotes that one expression's intermediate arithmetic to double and stores the final float once. The original geometry overlap and floor rules remain unchanged. No epsilon, snapping, damping, special platform ID/color branch, script change or speed change was added. Green ws-green takes remain distinct from the ws-yellow returning platforms. Original 45EC60 continues to carry displacement and release support normally.

## Final validation

Source checkpoint 4f2d498 adds jump/reboard coverage; runtime fix remains bf833fc. Both orange-platform-20260913-r2-diag and r2-quiet pass CTest 8/8. The platform test loads actual original item and player PATs and lift.cv4; it uses the real ActorManager collision callback/script/motion sequence and a minimal standing/gravity rider (not the entire player script). It verifies 45 consecutive falling frames with stable support/contact, jump detachment with separate player/platform displacements, return to the original Y, repeated boarding, walk-off detachment and another complete return. The final quiet original-DAT enemy reentry/death regression also passes. Full motion output is retained in r2-platform-motion.log.

The original scripts dictate acceleration, return speed and support decisions. Tests do not claim a full green rail-platform scenario or live gameplay replay; green identification is grounded in the PAT resource mapping and scripts, and no green-specific behavior was altered. Existing general collision, damage-mask, crystal-countdown, GC/ABI and texture-lifetime regressions remain passing.

All final EXEs have three DATs copied and SHA256-verified beside them. The latest crystal-countdown quiet marisaA.dat is copied to each new final runtime directory without modifying it. Each candidate/baseline has a validation manifest tying artifacts to source commits; prior builds/EXEs/logs remain. Live gameplay remains with the user under the established session preference.

Squirrel source/compiled auxiliary evidence carries forward from crystal-bgm-20260913 and actor-step-stone-20260907: callback call ABI and object ownership were checked there. Here the original Squirrel scripts execute unmodified; the confirmed repair is native collision arithmetic.

Checklist: correct orange/green resources distinguished; original scripts and native assembly compared; baseline jitter mechanism reproduced; original intermediate precision restored; no tolerance/behavior patch; both build modes pass; jump/walk-off/return/reboard checked; DATs staged; save copied; old artifacts retained; source committed before testing.
