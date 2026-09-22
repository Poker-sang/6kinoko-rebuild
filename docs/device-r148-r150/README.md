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
