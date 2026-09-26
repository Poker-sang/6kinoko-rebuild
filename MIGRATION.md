# 现代运行时迁移准备

当前可搬运基线先看 [独立目录构建与交接](docs/migration-baseline-20260926/README.md)。
历史迁移路线见 [迁移交接入口](docs/migration/README.md)。
最终轮变更、验证界限和新仓库起点见 [最终准备交接](docs/migration/final-preparation.md)。

本仓库仍是 Win32/x86 原版行为恢复基线。新增的跨平台编译入口只覆盖抽取的资源规则，
不是已经可运行的 x64 游戏。本批没有引入 SDL、bgfx、Godot、Unity 或 mod 加载行为。

[源码恢复评审](docs/migration/source-recovery-review.md) ·
[现代化／DAT／mod 分析](docs/migration/modernization-strategy.md) ·
[关键路径](docs/migration/source-map.md) ·
[新仓库交接与验收](docs/migration/HANDOFF.md)

修改与验证继续遵守 [AGENTS.md](AGENTS.md)。
