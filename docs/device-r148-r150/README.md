# Device lifetime and render targets: R148¨CR150

Base R147: source 8b93125, handoff bfa50fc. User authorized all three batches.
Use original EXE SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155,
IDA MCP session 5adc1098. E-imports and metadata reuse the same verified binary
in ../render-camera-r145-r147/imports.json and survey.json. Those record Win32
file/window/thread APIs, D3D9/D3DX, COM and WinMM; no static network/crypto/
process-injection family. Adjacent files retain fresh method evidence.

## R148: device initialization

KinokoGraphics now owns typed factory/device/swap-chain references and SDK
capabilities, display mode and presentation parameters. All active device
consumers use the pointer; legacy scalar words no longer duplicate SDK records.
Create preserves HAL hardware/multithreaded, HAL software/multithreaded,
REF software/multithreaded order; D24S8, discard swap/depth and interval one.
Client dimensions use the actual window rectangle, without invented 640x480
fallback. Keep the existing valid WINDOWINFO size and startup failure cleanup
as reconstruction boundaries. Release order is swap chain, device, factory.
TextureCaps now refers to GetDeviceCaps output, not an unpopulated split byte.

## R149: device reset and frame lifecycle

Message-loop poll calls TestCooperativeLevel and only resets on
DEVICENOTRESET, retaining that status until the next poll. Reset holds the
original recursive graphics lock, notifies listeners before Reset, releases
swap chain, calls Reset, reacquires swap chain, then notifies after Reset.
Failure skips reacquisition/after callbacks; DEVICELOST returns without locking.
Window toggle reverts Windowed on failure and retains the original metric-based
positioning on success. No invented window style change or resize policy.
Listeners now borrow actual pointers and call typed virtual entries in original
registration order, suppressing duplicates. Original xrefs show only renderer
registration; no speculative extra texture reload listener is introduced.
BeginScene holds the lock only on exact D3D_OK; EndScene releases it. Present
uses DONOTWAIT and preserves pending on failure. Existing null-device/swap-chain
startup guards remain explicit compatibility boundaries.

## R150: renderer state and render-target ownership

KinokoRenderer restores the original 100-byte prefix, with 40-byte named state,
borrowed device and typed acquired backbuffer/depth-surface references. Constructor
and initialization now live in C++, retaining trace calls and initial clear order.
Reset snapshots named state, clears caches, reacquires surfaces and replays only
the original state writes; unknown words are not restored. Before-reset Release
retains original slot bits (no invented retry or extra resource reload policy).
Filter assembly confirms MAG/MIN/MIP all receive the selected value. Cull retains
the original graphics-device receiver. The temporary level-zero surface is
released after binding, and that Release result is returned, as in 401E60.
ACT render-target creation uses a checked layout schema, preserves requested
image dimensions even when allocation is square, and preserves its true return
on creation failure. Listener ownership remains borrowed; sets own only nodes.

Added a fake-COM contract covering reset order, duplicate listeners, reset failure,
DEVICELOST, temporary surface release and exact reset-state writes. It is to be
compiled only. No game, CTest, or automated test execution is authorized here.

## Build handoff (2026-09-22)

- R148 commit: c929f54; R149 commit: 4ecc34d.
- R150 source commit: be9e4405de7ea20cc4c33ddb91bdbb9861ec3297.
- Build tree: build-runs/device-r150-quiet (fresh, retained).
- Generator: Visual Studio 18 2026, Win32 Release, RETDEC_DISABLE_TRACE=ON.
- EXE: runtime-builds/device-r150-quiet/kinoko_retdec_rebuild.exe.
- EXE SHA256: 2E48FDD01AD55C7C554AC086639D70CCCE60FBCA8FAE54F6B46981E9E3182242.
- All build targets completed, including device_lifecycle_contract. No compiler
  or linker errors found; existing warning output retained in build.log.
- Configure/build/DAT logs retained under the build tree. stage_dat.ps1 copied
  all three DAT files next to the EXE and verified sizes/SHA256 successfully.
- No game, CTest, or local automated tests executed. Contracts are compiled only;
  runtime behavior remains for user validation. Older artifacts retained.
