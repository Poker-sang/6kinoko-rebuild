# 新仓库交接入口（最终准备轮）

最新的源码边界、已完成事项、验证限制和未关闭关口见 [最终准备交接](final-preparation.md)。
PR #11 后的原版行为修正见 [原版行为修正](pr11-original-parity-20260924.md)，
DAT 到脚本的最后一批加载链交接见 [加载链收尾](final-loading-chain-20260924.md)。
第一批 `8c7c199a` 的完整交接记录保存在 [preparation-round-one.md](preparation-round-one.md)，
其中的历史构建与待办不能当作最终提交状态。

## 导入起点

使用 PR #11 **实际合并后的 master 提交**，并保存其 source revision、tree、源码快照、Git bundle、
SHA256、构建配置和对应验证记录。合并 SHA 与各 CI 链接在 PR 的最终评论；没有取得结果前不预写成功。
Actions 产物保留期有限，应将选定基线另行保存。源码快照现在也在 master 上生成。

不要只复制 `src/reconstructed`：`include`、`src/squirrel`、平台/ABI边界、`third_party`、
构建入口、`tests`、`tools`、`analysis`、`docs`、原版参考与上游补丁来源都需要随迁移保留。
具体阅读路线见 [source-map.md](source-map.md)，机器检查的导航见 [source-map.json](source-map.json)。

原版 DAT、用户存档和个人配置不纳入源码包；旧验证产物另行保存，不删除。
现有游戏仍需三个 DAT 位于 EXE 同目录，不通过工作目录或新fallback绕过。

## 检查与构建

```sh
python tools/migration/check_source_map.py --output build-runs/<batch>/source-map.json
python tools/audit_readability.py --source-ref HEAD --scope project --output build-runs/<batch>/readability.json
cmake -S cmake/migration-preflight -B build-runs/<batch> -DKINOKO_MIGRATION_REGISTER_TESTS=OFF
cmake --build build-runs/<batch> --config Release --parallel 2
```

`<batch>` 替换为新的独立目录。Windows 的独立规则目标可加 `-A x64`，Linux 单配置构建可加
`-DCMAKE_BUILD_TYPE=Release`。这只编译资源规则，不是整游戏的 x64 构建。
游戏顶层继续使用 [AGENTS.md](../../AGENTS.md) 的 Win32/MSVC 约定。

本批不执行游戏或本地自动测试。新增契约默认仅编译；运行时源码提交、CI筛选集、用户实机验证
必须分别记录。CV4/VM数值、平台分离和完整行为验收仍是下一阶段的关口，不因本PR合并而自动通过。

[源码恢复评审](source-recovery-review.md) · [现代化/DAT/mod策略](modernization-strategy.md)
