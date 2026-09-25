# 本批交接

源码版本：`7ca77b169ab649ba38795ac0ecf13c7d757d6a6b`。行为修改在 `6cf91ef`，后续提交仅补齐新增回归目标的头文件/包含目录。

- 无日志 Win32 Release **全量构建成功**，包括游戏与 62 个 contract EXE。
- **没有运行游戏，没有执行任何 contract、ctest 或本地自动测试。** 编译成功不代表回归执行通过。
- 三个原版 DAT 已复制到 EXE 同目录，大小及 SHA256 通过 staging 脚本校验。
- 旧构建 `6cf91ef` 的游戏 EXE 已生成，但全量构建因新增回归目标缺头文件失败；产物和日志均保留，并已为其游戏 EXE 同样复制校验 DAT。
- 仍有仓库既有编译警告；完整日志随此记录保存。没有修改用户原工作区的未提交证据文件。

## 最新产物

```text
C:\Users\poker\.codex\worktrees\pr11-original-parity\6kinoko-rebuild\runtime-builds\act-ownership-7ca77b1\kinoko_retdec_rebuild.exe
```

EXE 大小：912384 bytes。

SHA256：`32E097FFB1AEC32C00E024D551F19237A06FC8724A9C27346BC244B1CE27F400`。

构建树：`build-runs/act-ownership-7ca77b1`。配置、编译、DAT 校验日志分别为 `configure-7ca77b1.log`、`build-7ca77b1.log`、`stage-dat-7ca77b1.log`；机器可读记录见 `artifacts.json`。

## 审查入口

[完整所有权和失败矩阵](README.md) 区分原版恢复与保留的 native 安全边界。本批收口 ACT 文档、嵌套层/key/layout、资源加载/克隆、BeginStage、阶段/地图入口与释放顺序，不宣称整个项目所有裸地址/整数 ABI 已迁移完毕。

新增回归源码覆盖文档虚 Dispose 顺序、容器存活时点、数组逆序析构、key 的 layout 虚释放、层载荷先于脚本、活动文档虚析构/替换、地图初始化负返回值继续执行、实例异常不回滚 manager。现有 ACT 文件回归增加重复加载保留所有权，关联回归增加旧父解绑。执行交给用户，不要求立即测试。

继续推送至 PR #12 的 `codex/pr11-original-parity`，不合并。
