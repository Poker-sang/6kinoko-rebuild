# 最后一轮名称与宿主字段清理

源码版本：`c7cb30012691234637165512db5667b035f5ee9c`（主体修改 `0550a11`）。基线为 `9510ab0`，承接 [主宿主 C++ 迁移](../runtime-host-cpp-20260926/README.md)。

## 完成内容与依据

| 项目 | 修改与依据 | 保留边界 |
| --- | --- | --- |
| 历史 RetDec 名称 | [symbol-map.json](symbol-map.json) 记录 268 个标识映射，包括函数、宏和状态；同步生产声明、调用和测试桩，不新增旧名包装 | 两个 reader 接口增加 `_abi` 后缀，区分已有类型化接口 |
| 脚本扩展字符串 | `KinokoScriptExtension` 替换匿名数组和 g554/555/556 槽；具名 characters/length/capacity/reserved，断言原 28 字节大小及 StringRecord 字段位置 | 原保留字、初始容量与存储布局不变 |
| SqPlus 临时对象 | `ObjectStorage`、`HSQOBJECT`、`binding::Variable` 替换 3/2/5 个整数槽；使用既有 Squirrel/SqPlus 恢复布局，完整元数据复制仍为 20 字节 | 显式创建、绑定、销毁顺序保留；不引入 RAII 时序变化 |
| VM 与绑定接口 | 宿主 active/explicit VM 使用 `SQVM*`；对象与输出接口使用指针 | 未迁移整数 ABI、上下文交换及诊断接口仍在明确边界转换 |
| 平台文件 | memory/address-space/CRT 文件改用职责名称；移除无用途的 `unistd.h` 及四处反编译目录 include 依赖 | 原始 `src/decompiled/6kinoko.exe.c` 未改动，仅作证据 |
| ACT 关联虚调用 | [原版 0x4252E0 汇编](association-dispatch-asm.json) 为 `mov eax,[ecx]; jmp [eax+18h]` | 槽 6 是真实虚调用，保留原有 thiscall 桥接，不猜测完整对象布局 |

本批字段恢复复用 `legacy_string.hpp`、`squirrel_host_object.hpp`、`squirrel_variable_record.hpp` 的既有布局；Squirrel 源码辅助依据继续见 `analysis/remaining-mapping-20260920/source-disassembly-excerpts.json`。没有新增游戏分支或重做已完成的 ACT 所有权链。

## 明确保留及剩余工作

- `RETDEC_*` 构建宏、CMake target/EXE 名称及 `retdec_trace.log` 仍作为兼容接口保留；历史文档、原版证据、诊断字符串和审计样本保留旧称。它们不等于仍在调用 RetDec 占位函数。
- 仍有地址命名的旧 ABI 入口、整数指针交接、对象内部偏移和属性偏移表。本批没有声称全项目逐字段完成类型恢复，也不以改名代替语义恢复。
- 二进制格式、Win32 ABI、真实虚表及布局断言中的固定数值有实际含义，不为清零搜索结果删除。
- 跨平台和 x64 后端尚未实现；原版行为一致性不能由构建成功证明。

## 构建交接

最终产物记录见 [artifacts.json](artifacts.json)。Win32 Release 无日志全量构建，65 个 contract EXE **仅编译**；三个 DAT 已放入 EXE 同目录，并完成大小与 SHA256 校验。

运行目录：`runtime-builds/final-cleanup-02/`，EXE 为 `kinoko_retdec_rebuild.exe`。源码提交后才构建；前一轮成功产物 `final-cleanup-01` 及全部日志保留。

**未运行游戏、CTest、contract 或本地自动测试。运行验证由用户执行，本批不要求立即测试。**
