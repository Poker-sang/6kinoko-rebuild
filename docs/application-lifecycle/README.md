# Application loop and window lifetime — R1

## Original evidence

Original executable SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
IDA database `c0653aac`. Saved evidence covers 40D790 initialization, 40D940
shutdown, 40DA60 scene activation, 40DB30 messages, 40DBE0 update, 40DF60 display,
40E130/40E220 window messages, 40E2B0/40E2D0/40E2F0/40E320 workers and 473B30
entry. The failed exploratory 40DF50 query is superseded by 40DF60.

## Native ownership and interfaces

- Move the application entry, initialization, shutdown, messages, game/display/
  scene-loading/retirement workers to `application_runtime.cpp`.
- Remove the old 512-byte backing block, split application globals, duplicate
  critical sections and bidirectional synchronization copies. One native State
  owns worker/event handles and the scene lock. Configuration preserves the
  original 40-byte layout with named flags and typed HWND/HINSTANCE/object fields.
- Scene, Manager and Transition have distinct named virtual interfaces. Manager
  ports receive one explicit ABI adapter; create-scene is no longer dispatched
  through the generated cdecl entry as though it were a thiscall method.
- The scene queue owns typed Scene pointers until their deleting destructor has
  run. Switching preserves old.leave(new ID), new.enter(old ID), and asynchronous
  old-scene destruction. List synchronization remains outside user callbacks.
- Windows entry passes a real HINSTANCE. C-host access is restricted to named
  boot/archive/string ports and existing game/input/IME implementation boundaries.
  Diagnostic frame observations use an accessor, not another mutable counter.

## Restored original control flow

- 40DD8C stores the scene update result as the requested scene ID. `-1` stops the
  application; a changed ID starts the manager's scene-loading worker. The prior
  loop ignored this result and omitted that loading path.
- 40DBE0 updates before its timer wait and maintains the optional FPS counters.
  The existing game floating-point scope and timer registry remain authoritative.
- Draw callbacks all execute, accumulating readiness without short-circuiting.
  The renderer pending flag uses its graphics lock. 40DF60's separate-draw mode
  and up-to-eight 1 ms presentation attempts are represented explicitly.
- 40E130 suppresses WM_SYSKEYUP, not WM_NCMOUSEMOVE/WM_DISPLAYCHANGE. Alt+Enter,
  Alt+F4, IME notification, screensaver/power and quit behavior follow the saved
  original dispatch. No new focus-pause behavior is invented.
- Workers use DWORD WINAPI(void*) and SUCCEEDED(CoInitialize). This corrects the
  generated unsigned HRESULT comparison that always accepted failure.

## Reconstruction boundaries

Wake events are created before workers start and held until joins complete,
closing the prior race between shutdown and event publication. Shutdown joins
game/loading workers before finishing scene retirement and releasing managers,
then tears down audio/input/graphics. Successfully acquired COM/timer-period
resources are balanced explicitly; normal and partial initialization share the
same cleanup path. These are native ownership safeguards, not claims that the
original binary had identical error handling or thread timing.

The prior missing-input/audio-device degradation remains, although 40D790 in the
original returns failure for some such cases. The event-allocation Sleep fallback,
safe path-length checks and existing device/input/decoder implementations remain.
The batch does not claim that every standalone platform API has been migrated.
No Squirrel VM instruction, trace call, DAT format or resource search path changes.
Resources still load beside the staged executable, never from the reference CWD.

## Validation policy

The new contract source checks update/draw callback order, readiness accumulation,
old/new scene IDs, deferred destruction and the `-1` exit request using synthetic
objects. It will be compiled/linked only. Per user instruction, do not execute
the game, CTest or any local contract executable. Runtime behavior is unverified.
