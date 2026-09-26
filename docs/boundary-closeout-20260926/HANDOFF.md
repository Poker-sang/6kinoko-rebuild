# 本批交接

源码提交：`c4d8bc507d78c594bd6687bd6dfe13e9196bf400`。游戏行为修改为 `615a6b6` 加 `e7966c0` 的布局使用修复；最后两次提交只处理新增测试的解压检查构建依赖。

- 最终独立构建树：`build-runs/boundary-final`，Win32 Release 无日志版。
- 全量构建成功（退出码 0），游戏与 **64 个 contract EXE 已编译/链接**。
- EXE：`runtime-builds/boundary-final/kinoko_retdec_rebuild.exe`。
- EXE SHA256：`F08916258C1C6B6597CB171825BB6BB8A00FCBAE0D2C67B0239AECDA9D680DB9`；大小 912384 字节。
- 三个原版 DAT 已复制至 EXE 同目录，大小与 SHA256 校验成功（stage_dat.ps1 退出码 0）。
- **未运行游戏、CTest 或任何 contract**。本批没有用户运行结果，不要求立即测试。

源码变化与每项保留理由见 [README](README.md) 和 [台账](retention-ledger.json)。新增存档测试已编译；纹理 HRESULT 和应用失败策略回归也已编译。不能把编译成功写成往返或故障注入执行通过。

## 保留的构建历史

1. `615a6b6`：编译发现 ACT 回调遍历仍引用旧发布字段，未生成游戏 EXE。
2. `e7966c0` / boundary-fix：游戏及原有回归成功，新存档测试缺少独立解压入口而链接失败。该游戏 EXE 亦已 staging 三个 DAT，保留供追溯。
3. `3ee17e1`：尝试添加上游 uncompr.c，但当前 vendor 子集不含该文件，配置失败。没有添加或修改第三方源码。
4. `c4d8bc5` / boundary-final：测试改用已存在的 inflate API，要求完整 zlib 流结束；全量构建成功。

所有构建目录、EXE、日志和既有产物保留；对应 commit/哈希见 [artifacts.json](artifacts.json)。本批仅推送现有 PR 分支，不合并 PR。
