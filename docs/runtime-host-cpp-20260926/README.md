# 主运行宿主 C++ 迁移交接

本批从已合并 PR #12 的 `e68dd73` 开始，源码版本为 `7caa3b8a525a4c18a8bd5a1c59e2f025632385fc`。范围为主 C 文件、宿主存储、方法表及相关构建/夹具边界；不重开已完成的 ACT、对象与场景行为批次，不实施 x64 或替换平台后端。

## 已完成

- 删除参与构建的 `src/decompiled/6kinoko_rebuilt.c`。宿主实现现为 `src/reconstructed/runtime_host.cpp`（825 行），虚表为 `runtime_method_tables.cpp`（250 行），私有共享声明为 `runtime_host_internal.h`（645 行）。原始 `src/decompiled/6kinoko.exe.c` 未修改，继续作为证据。不是扩展名变更，也没有用 `.inc` 保留另一份 C 实现。
- 移除 508 项无外部源码引用的生成声明，以及后续依赖整理中的 42 个函数/声明记录。两份删除台账保存原文；这个计数不是全项目完成率。另清理重复及过时声明、地址注释和已迁走实现的空占位说明。
- 24 张虚表使用具名字段和目标函数的真实签名，保留原顺序、`__fastcall` 接收者及删除标志。所有表有大小断言，初始化使用函数指针而非函数地址转整数。[逐表清单](method-table-ledger.json)记录全部目标及顺序对照。
- 角色、地图、相机、输入、碰撞存储使用已有字段记录和明确的预留尾部；继续保留此前的存储大小、对齐、零初始化及原来的显式启动流程。全局回调使用 `KinokoScriptCallback`，地图诊断使用已恢复的字段。没有引入自动资源析构或改写关卡清理顺序。
- 主 VM 改为 `SQVM*`，同步宿主槽、源码 VM 桥接和夹具声明。VM 栈诊断通过源码字段获取，不再在宿主读取 `vm+24/48/52`。原诊断标签、门控及活跃调用保留；移除的诊断函数没有调用者。
- 清除主实现中的地址式函数名及 `gN` 标识，具名接口按现有调用用途恢复；整数注册/旧布局接口以 `_abi` 标明边界。[名称对照](symbol-map.json)辅助追踪，不据此认定全部接口都已可跨平台。
- 阶段夹具继续是 C 文件，但现在独立链接同一份生产 C++ 宿主；`stage_host_fixture.h` 仅提供测试所需的借用字节视图，不包含生产实现。虚调用探针按真实签名保存、替换和恢复函数指针。
- 删除无活跃调用者的 `retdec_asm_stubs.c/.h` 及其构建项。没有保留返回零的机器指令空桩。实际使用的内存和旧 CRT 兼容实现没有随之删除。
- 更新当前源码导航及两个检查工具入口。历史替换台账仍读取当时的 C 文件；旧 revision 的 island 检查仍选择历史路径。

## 原版依据及行为差异

| 项目 | 依据 | 处理 |
| --- | --- | --- |
| 24 张表的目标与顺序 | 合并基线的主文件及已经恢复的目标函数声明；`method-table-ledger.json` | 24/24 的具名目标序列相同；去除掩盖真实签名的转换 |
| 芯片四边形删除入口 | IDA MCP `chip-quad-delete-asm.json`：原 `44FD30` 写入 IColor 表，按 flags bit 0 释放，返回接收者 | 旧 `g25` 仍是硬编码 `0x44FD30`；改为本程序 `kinoko_color_destroy` 的函数指针，保留与 IColor 表不同的表身份。这是本批明确修复的一处行为缺口 |
| 相机类型复制桥 | IDA MCP `camera-copy-asm.json`：`466540` 将目的地址置 ECX、源地址压栈调用 `4664A0` | 保留真实接收者及既有 `kinoko_camera_copy` 实现 |
| 源码 VM 字段 | 当前 vendored Squirrel 2.2.2 的 `SQVM::_stack/_top/_stackbase`，以及既有 `analysis/remaining-mapping-20260920/source-disassembly-excerpts.json` | 只增加只读诊断视图，不改字节码解释行为 |
| 宿主对象与回调 | 既有 actor/map/camera/collision records、`KinokoInputManager`、`KinokoScriptCallback` 的字段/大小断言 | 以记录声明取代字节块及回调整数索引；预留大小不作为原版完整类大小的推断 |

本批 IDA 概览、导入及两处反汇编均已保存。未根据主观运行提示增加新游戏逻辑，未调整加载失败策略、资源目录或启动 VM 回退行为。

## 保留边界和结束条件

本批主文件迁移结束，不把以下条目重新计作 ACT 行为未完成：

1. Win32 x86、D3D9、DirectInput/DirectSound、IME 和旧 CRT 兼容层仍是当前平台基线。此批不声称 Linux/macOS 或 x64 可构建。
2. 真实虚调用的第六索引槽、原版调用约定、SqPlus/Sqrat 对象/变量描述符和 ACT/DAT 文件布局仍保留必要的 ABI/格式边界。主文件已消除 VM 与地图的匿名字段读取，但不是全项目固定偏移归零。
3. 若干宿主共享槽仍用旧整数表示，如 IME 句柄槽、阶段列表槽、音频设备槽和 `kinoko_act_vm_abi_slot`；名称和用途已明确，不能把它们误报为已经完成类型迁移。下一平台适配应按所属模块恢复类型或收进平台后端。
4. 私有头里仍声明来自其他模块的地址别名，诊断与兼容 API 仍有 `retdec` 名称；这些不再是主 C 文件、另一份解释器或机器指令空实现。全项目后续工作需按真实平台依赖和残留边界立项，不能仅以关键词命中重复优化。
5. 本批只完成编译及 DAT 校验。游戏行为、失败分支、存档往返及 contract 执行均等待用户验证；此前用户报告正常的版本不等于本版已验证。

## 构建交接

- 最终目录：`build-runs/host-cpp-03`、`runtime-builds/host-cpp-03`。
- 版本：`7caa3b8`，Win32 Release，无日志，C++17，保留 `/permissive-`，主文件不使用 `/TC`。
- 完整构建成功，65 个 contract EXE 已编译。没有运行游戏、CTest 或任何 contract。
- 三个 DAT 已复制到 EXE 自身目录并校验大小及 SHA256；不依赖原版目录作为工作目录。
- EXE SHA256：`2D5FF442E33C7226069EA26A6AEB1CD7FC66B4B95D6BFA2E457C53F2C6B7CD11`。
- [完整产物清单](artifacts.json)包含绝对路径、源码 commit 及各文件哈希。
- `host-cpp-01`（`837ed64`）和 `host-cpp-02`（`f44f115`）的失败日志及已有产物全部保留。第二批游戏曾构建成功，但夹具尚未全部编译，最终交接以第三批为准。

EXE：`C:\Users\poker\.codex\worktrees\pr11-original-parity\6kinoko-rebuild\runtime-builds\host-cpp-03\kinoko_retdec_rebuild.exe`。
