# 6kinoko 现代化迁移交接入口

> 当前基线与独立构建见 [2026-09-26 迁移基线](../migration-baseline-20260926/README.md)。下面是早期迁移规划，旧待办不代表当前实现状态。

基线：`08247b5431ff6d2459a6affe58c24f51ac90c573`（PR #10 合并）。
整理日期：2026-09-24。目标：保留原版体验，为以后**另开仓库**做 x64、跨平台、mod 准备。
本批没有创建新仓库，也没有选择并接入新引擎。

最终轮的源码、检查与合并交接见 [final-preparation.md](final-preparation.md)。

## 阅读顺序

| 文档 | 用途 |
|---|---|
| [源码恢复评审](source-recovery-review.md) | 保存第一次分析：统计口径、实际债务、P0–P3、30–60 人日的假设和收尾条件 |
| [现代化策略](modernization-strategy.md) | 保存第二次分析：技术栈选择、保真契约、M0–M6、DAT、CV4、mod、存档、安全 |
| [关键源码路径](source-map.md) | 找到实际执行路径、所有权交接、原版证据和不可误合并的接口 |
| [结构化清单](source-map.json) | 可机器检查的文件／符号导航；**不是完整调用图或依赖闭包** |
| [交接与验收](HANDOFF.md) | 本次修改、未完成事项、新仓库搬运方法、编译和测试责任 |

## 本次实际源码整理

`include/kinoko/compat/resource_rules.hpp` 抽取了无 Windows／VM 依赖的 LE 字段读取、
运行时归档路径转换、payload XOR、脚本查找路径和字节码标记规则；
`archive_store.cpp`、`file_io.cpp`、`src/squirrel/script_file.cpp` 的原生产路径直接使用它们。
Actor 池的 host/pool/lock 和具名入口保持 `KinokoActorPool*`，整数转换收缩到旧 ABI 入口。

没有把工具 `DatArchive` 当作游戏 reader；没有新建全局 VFS/mod 回退；
没有修改原版参考、上游源码、脚本栈协议、数值规则或资源加载顺序。
最终轮对 ACT 的构造／清理／克隆做具名布局整理，保留基线行为；详见最终交接。
旧 Win32 宿主布局、调用约定和未解释字段仍是迁移阻断项。

## 编译与证据

独立入口：`cmake/migration-preflight`，默认只构建资源规则契约程序，**不运行**。
`tools/migration/check_source_map.py` 只检查文件、符号文本和本目录相对文档链接，
不会加载 DAT、运行游戏或执行测试。

`.github/workflows/migration-preflight.yml` 在 Linux x64／Windows x64 编译这个小目标；
现有 Windows x86 工作流仍负责当前游戏的构建与其原先配置的 CI 契约。
二者结果不得互相代替。schema-2 词法清单单独输出到带提交号的 CI 产物，不覆盖历史报告。
源码快照工作流也覆盖 master，保存完整 source.tar.gz、source.bundle、revision 和校验值。
测试执行、原 DAT 校验和游戏体验结论的具体边界见 [交接记录](HANDOFF.md)。
