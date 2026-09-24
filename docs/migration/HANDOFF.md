# 迁移准备批次交接与验收

## 修订与范围

原评审／现代化讨论基线：`08247b5431ff6d2459a6affe58c24f51ac90c573`。
当前工作分支：`refactor/migration-preparation`；[PR #11](https://github.com/Poker-sang/6kinoko-rebuild/pull/11)。
源码规则／Actor 边界修改提交：`6c15f3f6e54a2f7caf9d5c72e9d376d601de2f6b`，
其 tree 为 `a62eeebc94dee85ad995ee1e7a2bafb895823435`。
后续文档／编译工作流提交以 PR 和工作流的实际 head SHA 为准，不把旧构建当作新提交的验收。

本批是迁移前的边界整理，不是完整源码恢复、不是真正 x64 游戏，也未创建新仓库。
没有引入 SDL/bgfx/Godot/Unity、VFS/mod、替代 VM 或新的存档规则。
两份分析以归档版本存放，不将第一次的词法统计、历史 CI 或工作量估算升级成当前认证。

## 实际交付

| 交付 | 已做 | 刻意未做 |
|---|---|---|
| resource_rules.hpp | 固定宽度 LE 读取、运行时路径转换、payload key/XOR、script lookup/tag；三个生产文件直接使用 | 不迁移 HANDLE/reader ABI，不修改 CRC、大小写、挂载、fallback、seek/partial read |
| actor_pool.cpp | host/pool/lock 和具名调用保持真实指针，整数留在原 ABI 入口 | 不改变对象布局、锁、句柄、容器、构造回收释放顺序 |
| 独立编译入口 | 无 DAT／Windows SDK／Squirrel 依赖的规则契约目标 | 不构建其他游戏模块，不执行新契约程序 |
| 文档与清单 | 两份分析、10 条路线、明确阻断项、路径／符号文本和相对链接检查 | 不是 AST 全量函数审计、完整调用图或依赖闭包 |
| 快照与 CI | 完整 tracked source、Git bundle、revision、SHA 和独立两平台编译日志 | 不上传原 DAT，不用快照代替运行验证 |

本 PR 无原版参考文件和 vendor 源码更改。抽取前后规则的对应点在
[关键源码路径](source-map.md) 中，函数内注释保留原地址证据与重建已有的保护行为。

## 现在具备的起点与仍未满足的关口

可以让新仓库的实验目标直接包含 `include/kinoko/compat/resource_rules.hpp`，
使用独立契约源码对照；也能按源码清单找到每条现有行为链。
**不能只复制清单文件就预期链接成功**，更不能删除 Win32 检查后宣称整个运行时已可移植。

| 优先级 | 下一工作包 | 出口条件 |
|---|---|---|
| P0 | 修正函数统计漏检，逐模块 A/B/C/D/E 台账，区分布局与真实指针 | 不再把改名、短转发壳、未审查实现计为语义完成 |
| P1 | ACT layer 构造／克隆／资源与失败链；Actor／脚本寿命 | 字段与所有权闭合，相关原版差异明确，有针对性的验收材料 |
| P1 | 固定原版体验基线，输入记录、状态与渲染／音频观察点 | 记录对应 EXE/source/DAT/config；不假定全对象内存哈希或固定新帧率 |
| P2 | 小范围 x64 CV4／VM 数值与 game_math 验证 | 原字节码表示、数值语义和宿主指针宽度分别解释并验收 |
| P2 | 将 platform、资源模型、游戏规则分离为真实构建目标 | 新核心不依赖 Windows/D3D 头和旧整数槽，保持旧宿主可作对照 |
| P3 | 新后端 → x64 → 跨平台 → 数据／脚本 mod | 每次只改变可定位的一组变量，满足各自 M 阶段出口 |

完整 M0–M6 见 [现代化策略](modernization-strategy.md)；本表不是重新估算的总工期。
此前 30–60 人日只覆盖恢复收口的初步预算，不包括引擎现代化、mod 与编辑器。

## 后续新仓库应搬运什么

先在当前 PR 的已确认提交或其合并提交上固定基线，再导出。优先保留完整 Git 历史／
可访问的原仓库提交以及可复算快照，而不是将零散文件拷贝后丢掉来源。

`.github/workflows/source-snapshot.yml` 产物包含 `source.tar.gz`、`source.bundle`、
`SOURCE_REVISION.txt`、`SOURCE_TREE.txt`、`ORIGINAL_EVIDENCE_SHA256.txt`、`SHA256SUMS`。
Actions 保留期有限，长期基线应另行归档；其中 `source.bundle` 是历史来源，
`source.tar.gz` 是该提交的 tracked-tree 快照。核对 revision 和 SHA，再决定导入方式。

建议保留的材料是 `include`、`src`、`third_party`、`tests`、`tools`、`cmake`、
`analysis`、`docs`、构建入口、许可证、上游补丁及证据；不要只搬 reconstructed。
历史源码、legacy 参考和新可移植目标可以分开构建，不应把旧 D3DX 导入库自动链接到新平台。
上游许可／补丁需逐依赖复核，仓库 LICENSE 不代表原游戏资源的分发授权。

用户本地的 `6kinoko_[abc].dat`、`marisa[A-C].dat`、个人配置和运行痕迹不随新源码库提交。
构建与运行目录不作为源码搬运；旧验证产物另存供对照，不删除。
原 DAT 保持只读；新包格式、缓存、mod 目录和 profile 仍需单独设计和授权实现。

若从快照起步，可以在自己选择的空目录导入 bundle 的 HEAD 或解开 tar；
不要把此处写死成一个新仓库地址。本批没有为用户创建任何新仓库或改远程指向。
未来迁移时先共享／搬运核心，再更换平台，避免长期维护两份发散的游戏规则。

## 编译与静态交接检查

### Windows x64：只编译抽取规则

```powershell
$BuildTree = "build-runs/migration-preflight-<batch>"
python tools/migration/check_source_map.py --output "$BuildTree/source-map-report.json"
cmake -S cmake/migration-preflight -B $BuildTree -A x64 `
  -DKINOKO_MIGRATION_REGISTER_TESTS=OFF
cmake --build $BuildTree --config Release --parallel 2
```

### Linux x64：同一规则目标

```sh
build=build-runs/migration-preflight-<batch>
python3 tools/migration/check_source_map.py --output "$build/source-map-report.json"
cmake -S cmake/migration-preflight -B "$build" \
  -DCMAKE_BUILD_TYPE=Release -DKINOKO_MIGRATION_REGISTER_TESTS=OFF
cmake --build "$build" --parallel 2
```

`<batch>` 是占位符，应替换为独立批次目录；不要覆盖旧日志。
编译使用 C++17，与现有代码一致；未为未来建议的 C++20 提前修改游戏标准。

`check_source_map.py` 仅检查文件、符号文本和相对文档链接。符号可能是声明或调用文本，
因此它不证明调用边存在、实现已恢复或行为正确。报告记录检查时 HEAD 与 dirty 状态，
输出采用独占创建，避免覆盖上次记录。此脚本需要 Python 3.9+，没有第三方 Python 依赖。

游戏仍使用顶层 CMake 的 Windows/MSVC/Win32 构建要求，按 [AGENTS](../../AGENTS.md)
构建、DAT staging 和启动。不要把独立入口的 `-A x64` 套到游戏顶层后移除断言。

## 验证责任与明确的未执行项

| 检查 | 执行主体／范围 | 不能据此宣称 |
|---|---|---|
| 静态源码清单检查 | 新 CI 与文档整理过程；文件／符号文本／链接存在 | 调用关系、运行行为、完整依赖或完成率 |
| Linux/Windows x64 preflight | 新 CI **仅编译** helper 契约；默认不注册 CTest | 新契约运行通过、DAT/CV4 已跨平台兼容、游戏已 x64 |
| 现有 Windows x86 workflow | 保持其已有配置；构建和选择的现有 CI 契约 | 用户游戏体验验证或未包含在筛选中的测试已通过 |
| 本地游戏与自动测试 | 本批不运行，按 AGENTS 由用户执行 | 代理实测无退步 |
| 原 DAT/staging/存档 | 本环境未获取用户原资源，未复制、校验或运行 | DAT SHA/查找/解码或存档已经实测 |

`tests/migration_compat_contract.cpp` 提供路径边界、字节码短输入、XOR 长度与固定宽度的
契约源码；编译期 static_assert 的成功与整个测试程序执行通过不是同一回事。
未来由用户选择执行时，独立入口提供 `KINOKO_MIGRATION_REGISTER_TESTS=ON`，
本批工作流不启用它，也不运行二进制或 ctest。

既有游戏 CI 是否覆盖了 `file_archive_contract` 等应按其实际 regex 查阅，不能因这些
源码出现在清单里就记为已经运行。PR 的检查结果和最终评论记录每个实际提交的运行链接；
本文件不预写尚未返回的 CI 成功结论。

人工体验检查继续遵守 AGENTS 的范围：进入第一关、尝试跳跃、看到怪物后退出。
不要求用户立即测试，也不把历史用户确认记成当前提交或代理亲测。
全局迁移可以继续规划，但源码稳定性与原版体验仍需对应版本的证据才能结案。
