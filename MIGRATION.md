# 现代运行时迁移现状

当前合并基线为 PR #13 的 `76443ccbcb15b93112fcb677f57b1c00c3615ac8`。构建与运行入口见 [README](.github/README.md)。

## 已收尾

C++ 主宿主迁移、内部指针类型、已恢复字段布局、所有权和重复旧接口整理已合并。用户已确认 `internal-types-59` 运行正常；构建源码为 `8047e62c1c75a1aabc611a6bff812c5148ec3433`。之后的验收与说明更新仅修改文档。

完整范围见 [内部类型迁移台账](docs/internal-types-20260926/README.md)，最终产物见 [交接记录](docs/internal-types-20260926/HANDOFF.md)。ACT 加载、对象绑定、更新销毁、切关及重新开始不应因旧文档中的待办而再次整体重开。

## 下一阶段

最终目标仍是跨平台。下一阶段需要单独规划 Windows 窗口／输入／音频／图形后端和 x86 调用约定、布局依赖的替换，并以原版行为和当前用户确认的产物作为对照。当前尚未完成可运行的 x64 或跨平台游戏，也未决定引擎替换或 mod 行为。

协议整数、动态属性偏移、未知字节和真实虚调用的保留依据已逐项记录；不能为了消除数字或普通指针而改变游戏行为。`cmake/migration-preflight` 的独立资源规则入口只覆盖抽取规则，不代表整个游戏已跨平台。

65 个 contract 程序已编译；代理没有运行游戏、CTest 或 contract。用户反馈不等于所有异常路径、存档往返和自动测试均已逐项验收。

## 历史证据

[独立目录构建基线](docs/migration-baseline-20260926/README.md)、[早期迁移路线](docs/migration/README.md)、[源码恢复评审](docs/migration/source-recovery-review.md)、[现代化策略](docs/migration/modernization-strategy.md)及[关键路径](docs/migration/source-map.md)保留供追溯。它们的旧待办和工期估计不能直接作为当前缺口清单。

后续修改、构建和用户验证交接遵守 [AGENTS.md](AGENTS.md)。
