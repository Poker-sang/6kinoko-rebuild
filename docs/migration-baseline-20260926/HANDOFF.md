# 独立迁移基线交接

本批完成源码封存、统一构建入口和独立目录构建；未修改游戏逻辑。源码提交 `4e8e81cfceef2f1ca30bb4632272f7323ec133d0`，来自 PR #12 分支，未合并 PR。

## 可搬运目录

`C:\KinokoMigration\win32 baseline 4e8e81c`

- `source.zip`：完整 2364 个跟踪文件，含源码、构建入口、third_party、原版参考与已跟踪证据。
- `source.bundle`：完整 Git 历史，git bundle verify 成功。
- `SOURCE_TREE.txt` / `SOURCE_REVISION.txt` / `source-manifest.json`：树、提交与逐文件大小/SHA256。
- `user-validated-runtime`：上一版 c4d8bc5 EXE 和三个 DAT，逐一复制校验。该版仅记为用户反馈正常，不记为代理运行验证或所有测试通过。
- `source`：由 zip 解压的独立源码，没有 .git，也不依赖相邻旧项目；额外 revision 标记用于构建记账。
- `source/build-runs/migration-quiet`：本次独立配置/编译日志和机器清单，保留作为证据。
- `source/runtime-builds/migration-quiet`：新游戏与 DAT，工具/契约在 tools 子目录。

搬到其他路径/机器后重新使用 build_staged.ps1 创建新的 Name，不复用包含旧绝对路径的 CMakeCache。源码 zip 与 bundle 是干净恢复入口；已有构建产物保留用于追溯。

## 构建结果与界限

从 `C:\KinokoMigration` 调用含空格路径下的脚本，显式 SourceDir 指向封存的 user-validated-runtime。全量 Win32 Release 无日志构建成功（退出码 0），游戏与 **65 个 contract EXE 已编译**；三个 DAT 在 EXE 同目录，大小/SHA256 校验成功。

65 比此前统计的 64 多一个，是把原本输出到构建树的 act_layer_storage_contract 归入统一目录；它此前已参与全量编译，不是本批新增行为测试。

EXE：`C:\KinokoMigration\win32 baseline 4e8e81c\source\runtime-builds\migration-quiet\kinoko_retdec_rebuild.exe`

SHA256：`5B4FABDA4E5764A97DBFC3CD7227D9E3A2D303E666ECA450CC1BECAC2D53C542`。不同构建的 EXE 哈希分别保存，不声称字节级可重复构建。

构建后所有 2364 个源码文件仍与导出清单一致；扫描 102 个生成工程/属性/缓存/契约配置文件，未发现旧 C:\WorkSpace 或旧 worktree 路径引用。当前工具链仍依赖本机安装的 MSVC/Windows SDK；这是同机独立路径构建验证，不是干净新系统验证。

**未启动游戏、CTest 或任何契约 EXE。** 新版运行状态待用户验证，不要求立即测试。运行依赖包括 x86 VC++ runtime/UCRT 和 d3dx9_33.dll；见 runtime-dependencies.txt。没有复制系统 DLL 或用户存档。

## 本批入口修改

- build_staged.ps1 统一配置、构建、DAT staging 与清单；从脚本位置确定根目录，拒绝覆盖旧产物，支持无 Git 的源码快照。
- 两个历史批次助手改用上述入口，不再写死工作区或构建器路径，资产目录显式提供。
- inspect_cv4.py 默认使用仓库内 opcode 头文件；旧相邻 Squirrel 路径不再必需。
- act_layer_storage_contract 统一输出到 runtime-builds/.../tools。

详细机器记录见 [artifacts.json](artifacts.json)；工具与恢复步骤见 [README](README.md)。未跟踪旧分析/构建目录继续原地保留，不假称它们都包含在 Git 快照中。本批到此关闭，不扩展为新的游戏逻辑整理。
