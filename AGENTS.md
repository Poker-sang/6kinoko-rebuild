---
name: reverse-skill
description: 6kinoko简单的逆向skill
---

..\6kinoko\6kinoko.exe是一个将近20年前的老游戏，我想加点新的内容：即用自己编写的exe，读取原版的6kinoko_*.dat。但第一版确实只需要还原windows上的运行效果即可。

逆向6kinoko项目以提供的C反编译源码为基础，使用IDA MCP，并且不要忘记使用 ..\squirrel-2.2.2 源码的反汇编结果作为辅助。但若确定是squirrel相关的函数，你可以选择直接引入源码。生成后必须复制 6kinoko_*.dat 文件到 exe 所在目录，不能直接指定工作目录。在原版目录中还有 marisa[A-C].dat 是存档文件。

主要逻辑在 .\src\decompiled\6kinoko_rebuilt.c 里，你尽量做到经过的函数完全相同（可以通过x64dbg MCP保证）。

x32dbg/x64dbg 在 C:\Users\poker\AppData\Local\Microsoft\WinGet\Packages\x64dbg.x64dbg_Microsoft.Winget.Source_8wekyb3d8bbwe\release\x32\x32dbg.exe 里，使若丢失可以手动拉起进程。

不要直接读取git更改，内容太多，有必要时截取少量读取或定向读取。
每次有大量或重要修改告一段落后提交备份，可以清理无用的构建目录。
C:\WorkSpace下的其他项目是以前的失败尝试，参考意义不大。

我随时可能给你提供一些6kinoko运行的主观线索，你可以作为参考去寻找bug，但不要为了迎合我的提示故意去写某些原版不存在的逻辑，而是恢复原版加载逻辑，让其自然而然呈现原版的行为。遇到的函数有必要（迁移后可读性更好，更好维护的话）可以继续迁移到c++

## 当前构建与运行流程

目录约定：

- `build-runs\<build-tree>\` 只存放 CMake 构建树。
- `runtime-builds\<run-dir>\` 只存放生成的 EXE 及其运行文件；不要再套配置名目录。
- 不要在仓库根目录新建构建树；需要新变体时放入 `build-runs\<build-tree>\`。
- 以下命令中的 `<...>` 都替换为本次实际名称或路径，不代表固定目录名。

变体构建命令（x86 Win32）：

```powershell
$BuildTree = "build-runs\<build-tree>"
cmake -S $BuildTree `
  -B "$BuildTree\out" `
  -G "<generator>" -A Win32
cmake --build "$BuildTree\out" `
  --config "<configuration>" --parallel 4
```

当前诊断/兼容构建使用的预处理宏：

```text
WIN32_LEAN_AND_MEAN
NOMINMAX
RETDEC_CAPTURE_EVERY_10
RETDEC_TRACE_FILTER
RETDEC_DIAGNOSTIC_MAP_LAYERS_FRONT
```

当前诊断/兼容构建使用的 MSVC flag：

```text
/TC /Gy /O2 /wd4100 /wd4244 /wd4267 /wd4706
/OPT:REF /OPT:ICF
```

链接库必须包含：`user32`、`gdi32`、`winmm`、`d3d9`、
`third_party\d3dx9_33.lib`、`dinput8`、`dxguid`、`imm32`、`ole32`、
`dbghelp`。

标准顶层 CMake 构建使用：

```powershell
$BuildTree = "build-runs\<build-tree>"
cmake -S . -B $BuildTree `
  -G "<generator>" -A Win32 `
  -DKINOKO_REFERENCE_DIR="<reference-dir>"
cmake --build $BuildTree --config "<configuration>" --parallel 4
ctest --test-dir $BuildTree -C "<configuration>" --output-on-failure
```

顶层 CMake 可选诊断/实验开关：

- `-DKINOKO_RETDEC_DISABLE_TRACE=ON`：关闭 RetDec trace 文件。
- `-DKINOKO_RETDEC_TRACE_FILTER=ON`：只保留筛选后的诊断 trace。
- `-DKINOKO_RETDEC_DIAGNOSTIC_NO_BGM_SERVICE=ON`：诊断时停用 BGM 环形缓冲服务。
- `-DKINOKO_RETDEC_DIAGNOSTIC_NO_BGM_WRITE=ON`：诊断时跳过 BGM 写入。
- `-DKINOKO_RETDEC_MAP_FILE=<path>`：输出链接 map 文件。
- `-DKINOKO_ENABLE_SQUIRREL_CPP_VM=ON`：启用实验性的 C++ Squirrel VM，要求
  `..\squirrel-2.2.2\SQUIRREL2` 存在。

生成 EXE 后必须把原版的三个 DAT 复制到 EXE 同目录：

```powershell
$Executable = "runtime-builds\<run-dir>\<executable>.exe"
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\stage_dat.ps1 `
  -Executable $Executable `
  -SourceDir "<reference-dir>"
```

`stage_dat.ps1` 只复制 `6kinoko_a.dat`、`6kinoko_b.dat`、
`6kinoko_c.dat`，并校验大小和 SHA256；不复制 `index.dat`。

运行时使用：

```powershell
$Executable = "runtime-builds\<run-dir>\<executable>.exe"
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_staged.ps1 `
  -Executable $Executable `
  -Wait
```

`run_staged.ps1` 只检查三个 DAT 已经位于 EXE 同目录，然后直接启动传入的
EXE。不得把参考目录作为 `WorkingDirectory`，也不得依赖 `-SourceDir`、
`-RunsRoot`、`-RunName` 或另一个 staging EXE；资源必须由 EXE 从自身目录加载。
普通运行不要使用 `--data-dir` 或 `KINOKO_DATA_DIR` 绕过这个规则。

## 反编译 Squirrel 经验

- RetDec 生成的 C 函数若出现“未初始化局部变量通过固定偏移访问”（例如
  `v1 + 24`、`v1 + 28`），优先判定为原始 `__thiscall` 的 ECX 接收者丢失；必须从
  原版汇编和调用点恢复显式 `this`，不能依赖偶然的栈布局。
- Squirrel 的复合算术指令要按 VM 字节码格式解码操作数。`COMPARITH` 中高 16
  位是左值/接收者索引，`arg2` 是键索引，低 16 位是增量索引；不要按 RetDec
  临时变量名称猜测。
- 关闭诊断时只在输出端静默日志并停用截图；不要把整个 VM 的 trace 调用编译掉，
  否则会改变栈形状和时序，掩盖或暴露与诊断无关的生命周期错误。
- 后续默认只构建和测试无日志版，不再要求诊断版与无日志版双测（2026-09-20 用户更新）。
  确认 DAT 仍从 EXE 自身目录加载、窗口响应且第二阶段画面存在。
- 每次游戏运行验证限于：进入第一关、尝试跳跃、看到怪物后退出。达到这三个
  条件就结束本次验证，不继续延长游玩或重复排查迁移前已有的崩溃。
- 每批测试前先提交当时代码，并记录测试产物对应的 commit。新的测试批次使用
  独立的构建和运行目录，不覆盖旧 EXE、日志、截图或其他构建产物。
- 所有测试产物均保留，不删除旧版本或清理构建目录，便于用户协助回查问题。

## 用户更新：测试交接（2026-09-20）

- 用户明确要求后续测试由其执行；继续修改、构建和提交，不再自行运行游戏或本地自动测试。
- 新的无日志版构建成功后复制并校验三个 DAT，告知版本和 EXE 路径即可，不要求用户立即测试。
- 保留新增回归测试源码供后续使用，但区分“已编译”和“已执行通过”。
- r59、r60、r66 已由用户确认正常；不得将用户验证记成代理亲自完成的冒烟测试。

## 用户更新：迁移优先级（2026-09-21）

- R125 已由用户确认正常，仍不得记为代理运行验证。
- 后续优先整理 `function_xxx` 地址名称、固定偏移、固定跳转和裸指针等反编译遗留；以原版证据恢复具名接口、字段布局和所有权，而非只改函数名。
- 此处“裸指针”特指硬编码数值地址再转为指针，不是禁止 `T*` 指针类型。合理的普通指针、借用指针和真实虚调用可以保留，不应为消除 `*` 而引入包装。
