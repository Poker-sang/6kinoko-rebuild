# E-triage: original PE

Recorded 2026-08-23 from IDA MCP session `bd667883`.

## Identity

- Input: `D:\6kinoko\6kinoko.exe`
- Architecture: x86 / 32-bit PE
- Image base: `0x400000`
- Image size: `0x130000`
- Entry point: `0x4aca23` (`start`)
- GUI subsystem; original PE header reports entry point `0x4aca23`
- IDA database: `D:\6kinoko\6kinoko.exe.i64`
- Functions: 3,963 total; 2,247 strings; 4 segments

## Imports (hard gate)

IDA reported 137 imports. The important categories are:

- File and runtime data: `CreateFileA`, `CreateFileW`, `ReadFile`, `WriteFile`, `GetFileSize`, `SetFilePointer`, `CloseHandle`, `FindFirstFileA`, `FindNextFileA`, `FindClose`, `SetCurrentDirectoryA`, `GetModuleFileNameA/W`.
- Window and message loop: `RegisterClassExA`, `CreateWindowExA`, `ShowWindow`, `UpdateWindow`, `PeekMessageA`, `GetMessageA`, `TranslateMessage`, `DispatchMessageA`, `DefWindowProcA`, `PostQuitMessage`, `SetWindowPos`.
- Direct3D: `Direct3DCreate9` from `d3d9`, and `D3DXCreateTexture`, `D3DXMatrixTranslation`, `D3DXMatrixScaling`, `D3DXMatrixMultiply`, `D3DXMatrixRotationYawPitchRoll` from `d3dx9_33`.
- Timing and concurrency: `timeBeginPeriod`, `timeGetTime`, `Sleep`, `CreateThread`, `CreateEventA`, `WaitForSingleObject`, `SetEvent`, `EnterCriticalSection` and related APIs.
- COM and input/text: `CoInitialize`, `CoCreateInstance`, `CoUninitialize`, GDI font APIs, and IMM32 composition APIs.
- Dynamic loading: `LoadLibraryW` and `GetProcAddress` are present, so a clean static import table does not rule out runtime-loaded dependencies.
- No obvious network imports were present in the 137-entry import table.

## Key strings

- `0x4d59c0`: `data/script/boot.nut`
- `0x4d5c8c`: `data/script/class_def.nut`
- `0x4d5d68`: `LoadMap`
- `0x4d5de4`: `CreateActorFromMap`
- `0x4d5e44`: `SaveTable`
- `0x4d5eb0`: `6kinoko_c.dat`
- `0x4d5ec0`: `6kinoko_b.dat`
- `0x4d5ed0`: `6kinoko_a.dat`
- `0x4d5ee0`: `Marisaland2`
- `0x4ec4f0`: `textureID`
- `0x4eccd8`: `LoadTexture`
- `0x4ed1c4`: `BeginStage`
- `0x4ed1d0`: `EndStage`

## Initial hypothesis

The first rebuild boundary is the original startup path: CRT -> `_WinMain@16` -> window/D3D setup -> archive/script bootstrap -> message/render loop. DAT parsing is deferred until a loader call and its file offsets are identified.
