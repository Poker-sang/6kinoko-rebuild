# Sprite drawing and angle math recovery

Original: ../6kinoko/6kinoko.exe, SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155. IDA MCP database 4f3e4dab. JSON files retain original assembly/decompilation and xrefs.

E-imports: imports.json records the full IAT: window/input/font APIs, file/heap/thread APIs, Direct3D9/D3DX and COM. No static network, crypto or remote-process injection imports. LoadLibrary/GetProcAddress exist. These native rendering functions are unrelated to Squirrel; source VM behavior is unchanged.

Batch 1: restore CSprite 404E10 as a typed receiver and four stack floats, retain half-pixel placement and all non-position attributes, submit the original triangle strip. Shared draw now ignores bind/FVF failure as the original does and returns the draw HRESULT. Existing missing-device diagnostic guard remains. Regression uses a mock COM table and a real thiscall invocation. Source committed before build/test; results will be appended after validation.
