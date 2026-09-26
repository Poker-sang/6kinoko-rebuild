# Win32 迁移基线与独立构建

本批只整理搬运与构建入口，不修改游戏逻辑、不运行游戏或测试。前批 c4d8bc5 的游戏已由用户反馈“没有问题”；未提供专项测试明细，不推定 contract 执行通过。

## 可搬运范围

完整 Git 跟踪树需要一起带走：src、include、cmake、third_party、tests、tools、docs、已跟踪 analysis、资源图标与第三方来源记录。原版参考 C 和既有原版证据也随源码快照保留。不是只复制 reconstructed 目录。

export_baseline.py 从已提交且无已跟踪改动的 checkout 导出 source.zip、source.bundle、SOURCE_TREE.txt、SOURCE_REVISION.txt 和逐文件 SHA256 清单，在仓库之外解压源码并校验。不依赖导出目录存在 .git；解压目录额外加入源码 revision 标记。

未跟踪的旧构建、运行、分析文件、用户存档不自动包含在源码快照中，也不删除。本次另外封存已由用户反馈正常的 EXE 与三个 DAT，作为独立回退产物；没有复制用户存档。

## 编译依赖

- Windows、CMake >= 3.24、MSVC C/C++ x86 工具链、Windows SDK；本机选用 Visual Studio 17 2022 CMake generator 和已安装 MSVC 14.44。
- Squirrel 2.2.2、SqPlus、Sqrat、Boost 1.44、zlib 1.2.3、libogg 1.1.3、libvorbis 1.2.0 及 d3dx9_33.lib 均来自仓库 third_party；不需要相邻 squirrel-2.2.2 工程，也没有配置阶段下载。
- Python 3 仅用于导出/离线工具；Git 用于导出与恢复历史。PowerShell 用于构建与 staging。
- 游戏运行仍需 x86 VC++ 运行库/UCRT 和 d3dx9_33.dll（旧 DirectX 运行时），以及 Windows 图形/输入/音频 API。仓库中的 .lib 是链接库，不是运行 DLL。本机有该 DLL，不宣称在干净新系统已实测。
- IDA/x64dbg 用于后续逆向，不属于游戏构建依赖；历史证据内的旧绝对路径是来源记录，不改写成虚构的新来源。

## 统一构建入口

在任意当前目录调用，源码根目录由脚本自身确定。SourceDir 必须明确提供，只用于复制 DAT 和离线契约的配置参数，游戏仍从 EXE 目录加载。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File '<源码目录>\tools\build_staged.ps1' `
  -Name migration-quiet -SourceDir '<三个DAT所在目录>' -Generator 'Visual Studio 17 2022'
```

脚本依次配置、全量构建 Win32 Release 无日志版、stage_dat.ps1 校验；不运行测试、不启动游戏、不自动提交推送。已有同名 build-runs/runtime-builds 目录会拒绝覆盖。日志和机器清单在 build-runs/<Name>；游戏在 runtime-builds/<Name>，契约 EXE 在其 tools 子目录。

原 build_act_load.ps1/build_text_stage.ps1 改为调用统一入口，保留旧批次交接提交行为，但必须显式指定 SourceDir。独立源码快照使用 build_staged.ps1，不使用需要 Git 的历史批次助手。

修正 inspect_cv4.py 的默认 opcode 头文件位置为仓库内 third_party；act_layer_storage_contract 纳入统一产物目录。它此前已随 all-target 编译，但输出在构建树，不能把这次产物计数增加误记为新写了一个回归。

## 导出与恢复

```powershell
python tools/migration/export_baseline.py --destination '<新的仓库外目录>'
```

源码 zip 包含完整跟踪树；Git bundle 另存可恢复历史。恢复到另一个新目录时：

```powershell
git init '<新checkout>'
git -C '<新checkout>' fetch '<快照目录>\source.bundle' HEAD
git -C '<新checkout>' checkout -b migration-baseline FETCH_HEAD
```

本批实际独立构建的 revision、路径、哈希和结果见 HANDOFF.md / artifacts.json。当前基线来自 PR #12 分支，不冒充已经合并到 master。停止条件为源码独立目录能构建、DAT 就位并有完整交接；不借此继续游戏逻辑优化。
