# Batch 96 - native-callback-entries-quiet-r96

Source committed before build: 0264eda (`Name native callback adapter entries`).

This batch names the five native callback adapter entry points used by the Input
binding: string callback, three-integer callback, boolean two-integer callback,
integer two-integer callback, and receiver capture pair. The original
`function_46b490`, `function_46b500`, `function_46b610`, `function_46b6f0`, and
`function_46c6b0_pair` symbols remain as ABI-preserving aliases, while the Input
registration path now calls the named entries directly. Argument order and
return behavior are unchanged.

Build: `build-runs/native-callback-entries-quiet-r96`.
EXE: `runtime-builds/native-callback-entries-quiet-r96/kinoko_retdec_rebuild.exe`.
EXE SHA256: `B74AC9991DAECAB8ADFFF870D180BD3215AAED7934895D4C9BCC16952DED17E6`.

Quiet Win32 Release configure and full build succeeded, including contract
compilation. Three original DATs were staged beside the EXE and size/SHA256
verified.

No game, CTest, contract executable, or local automated test was executed; runtime
verification remains delegated to the user.