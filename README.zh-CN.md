# 6kinoko 重建项目

[English](README.md) | 简体中文

这是对 6kinoko Windows x86 版本进行的非官方源码级重建。重建后的可执行
文件会从自身所在目录读取原版的三个 `6kinoko_*.dat` 归档，并运行恢复出的
原生逻辑与脚本游戏逻辑。

目前游戏已经可以部分游玩，包括进入第一关、移动、跳跃以及显示敌人。项目尚未
实现完整的行为等价，也不能保证游玩过程中完全不崩溃。本仓库是一个工程与保存
项目，并不是原版游戏的完整替代品。

![重建运行时渲染的标题画面](docs/images/gameplay.png)

*重建运行时从原版 DAT 资源加载并渲染出的标题画面。*

## 目标与边界

- 在增加新游戏内容前，先还原原版的 Windows 运行效果。
- 保留原版 DAT 与 ACT/脚本加载路径，让恢复出的规则自然呈现原版行为。
- 只有在调用约定、对象布局和行为具有证据及测试支撑时，才把生成的 C 代码和
  手写 x86 汇编替换为可读的 C++。
- 当恢复出的函数能够可靠地判定为运行时函数时，使用原版 Squirrel 2.2.2 源码。
- 让诊断功能保持可选，不通过改变游戏行为来掩盖已知崩溃或迎合某项观察。

本仓库**不包含**原版 DAT 归档、存档文件或原版游戏可执行文件。运行重建版本
需要一份通过合法途径取得的原版游戏。

## 当前架构

当前运行时仍然是混合式重建。大部分恢复出的游戏及 VM 逻辑仍位于一个大型的
生成 C 翻译单元中，已验证的子系统则逐步迁移到职责明确的 C++ 模块。原版 ACT
脚本仍然负责关卡和实体行为。

| 位置 | 职责 |
| --- | --- |
| `src/platform/windows_entry.cpp` | Windows 入口点与异常边界 |
| `src/platform/diagnostics.cpp` | 可选的跟踪输出与崩溃转储 |
| `src/reconstructed/legacy_abi.cpp` | 由编译器生成的 x86 虚函数调用 |
| `src/reconstructed/actor_collision.cpp` | 恢复出的 Actor 碰撞方法 |
| `src/reconstructed/actor_methods.cpp` | Actor 图块查询、标志与优先级 |
| `src/reconstructed/act_resource.cpp` | ACT 时钟、唤醒期限与关卡清理 |
| `src/reconstructed/stage_cleanup.cpp` | 全局 ACT 所有权与声音退出清理 |
| `src/reconstructed/script_callbacks.cpp` | Actor/Camera 回调绑定与 SqPlus 临时对象生命周期 |
| `src/reconstructed/sprite.cpp` | 类型化精灵几何与 Direct3D 绘制 |
| `src/squirrel/squirrel_compile_bridge.cpp` | 将 ACT 源码编译为原版字节码 |
| `src/squirrel/squirrel_value_bridge.cpp` | 基于源码的对象与错误所有权管理 |
| `src/squirrel/squirrel_generator_bridge.cpp` | 原版生成器挂起/恢复与数组删除 |
| `src/decompiled/6kinoko_rebuilt.c` | 尚未迁移的游戏与 VM 恢复代码 |
| `src/decompiled/6kinoko.exe.c` | 未修改的反编译器参考输出 |
| `third_party/squirrel-2.2.2` | 仓库内的 Squirrel 2.2.2 源码与许可证 |
| `tests/stage_contract.c` | 原生方法与原版脚本契约 |
| `analysis/*/report.md` | 原版地址、证据及验证边界 |

## 环境要求

- Windows
- 带有 MSVC x86 工具链和 Windows SDK 的 Visual Studio
- CMake 3.24 或更高版本
- 已安装的 Visual Studio 版本所支持的 Win32 生成器
- 原版 `6kinoko_a.dat`、`6kinoko_b.dat` 和 `6kinoko_c.dat` 文件

当前构建所需的 Squirrel 2.2.2、zlib 1.2.3 和 D3DX9 导入库已包含在本仓库中。

## 构建与运行

请从仓库根目录运行 PowerShell。如果本机安装的 Visual Studio 使用不同的 CMake
生成器名称，请相应调整生成器。

```powershell
$BuildTree = "build-runs/release"
$RuntimeDir = Join-Path (Get-Location) "runtime-builds/release"
$ReferenceDir = (Resolve-Path "../6kinoko").Path

cmake -S . -B $BuildTree `
  -G "Visual Studio 18 2026" -A Win32 `
  -DKINOKO_REFERENCE_DIR="$ReferenceDir" `
  -DKINOKO_RUNTIME_DIR="$RuntimeDir"
cmake --build $BuildTree --config Release --parallel 4
ctest --test-dir $BuildTree -C Release --output-on-failure

powershell -NoProfile -ExecutionPolicy Bypass -File tools/stage_dat.ps1 `
  -Executable "$RuntimeDir/kinoko_retdec_rebuild.exe" `
  -SourceDir $ReferenceDir
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 `
  -Executable "$RuntimeDir/kinoko_retdec_rebuild.exe" -Wait
```

`stage_dat.ps1` 只会复制并校验三个必需的 DAT 归档。可执行文件从自身所在目录
解析资源；不要把原版游戏目录设为其工作目录，也不要使用数据目录覆盖。原版的
`marisaA.dat`、`marisaB.dat` 和 `marisaC.dat` 等存档是独立文件，本脚本不会
复制它们。

`KINOKO_RUNTIME_DIR` 用于选择可执行文件输出目录。
`KINOKO_REFERENCE_DIR` 为测试及资源复制提供归档输入，不会改变运行时资源解析。

## Squirrel 2.2.2

仓库内的 Squirrel 2.2.2 源码目前用于在独立 VM 中编译内联 ACT 源码，并提供
经过验证的对象/错误所有权、生成器挂起/恢复和数组删除实现。生成的字节码通常
仍在恢复出的游戏 VM 中执行。

完整的 C++ 执行后端仍处于实验阶段。启用它需要同时设置 CMake 选项
`KINOKO_ENABLE_SQUIRREL_CPP_VM=ON` 和运行时环境变量
`KINOKO_SQUIRREL_CPP_EXECUTE=1`。普通构建不会启用该后端。

## 诊断

普通构建默认静默，也不会自动截取渲染画面。剩余的 VM 跟踪调用点会继续保留，
因为部分恢复出的 C 代码对栈形状和时序较敏感。无需重新构建即可启用其输出：

```powershell
$env:KINOKO_TRACE = "1"
$env:KINOKO_CRASH_DUMP = "1"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 `
  -Executable "runtime-builds/release/kinoko_retdec_rebuild.exe" -Wait
Remove-Item Env:KINOKO_TRACE, Env:KINOKO_CRASH_DUMP
```

这些选项会在可执行文件旁写入 `retdec_trace.log`，并在发生未处理异常后写入
`retdec_crash.dmp`。两项功能默认都关闭。`KINOKO_TRACE=0` 会明确保持跟踪输出
静默。诊断构建可使用 `KINOKO_RETDEC_DISABLE_TRACE=OFF`，并可选择同时使用
`KINOKO_RETDEC_TRACE_FILTER=ON`。

其他归档、窗口捕获、进程转储和调试器工具记录在
[tools/README.md](tools/README.md) 中。

## 验证策略

每批游戏验证必须从已提交的源码版本开始，并使用新的构建及运行目录。构建文件、
可执行文件、日志、截图和失败尝试都要保留，同时记录对应的源码版本。

限定的游戏冒烟检查为：进入第一关，尝试跳跃，移动到看见敌人，然后立即退出。
迁移前已经存在的偶发访问异常应记录下来，但不能自动视为迁移导致的回归。

## 参与贡献

改动应以反编译源码证据、原版反汇编、调用点、对象布局和定向契约为依据。不要
添加原版程序中不存在的、针对表面症状的游戏规则。原版资源输入应保留在仓库外，
绝不能提交 DAT 归档、原版可执行文件、存档、崩溃转储或本地分析数据库。

## 许可证与第三方声明

本重建项目中新编写的原创代码使用 [MIT License](LICENSE) 发布。

MIT License 不授予对原版 6kinoko 游戏、资产、DAT 归档、可执行文件、脚本或
其他恢复出的受版权保护材料的任何权利。本项目为非官方项目，与原作者无隶属关系，
也未获得其认可。第三方组件继续遵循各自的许可证，包括随 Squirrel 2.2.2 与
zlib 1.2.3 分发的许可证声明。
