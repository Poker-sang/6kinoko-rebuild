# 工作流修复与 README 收尾

## 范围和依据

用户允许本批本地自动测试，游戏仍由用户运行。工作目录为 Codex 的 `pr11-original-parity/6kinoko-rebuild` 工作树，未修改 `C:/WorkSpace/6kinoko-rebuild` 主检出。

| 项目 | 失败原因与依据 | 修复 |
| --- | --- | --- |
| Migration preflight | 远端 36246495233 引用已移走的 `src/decompiled/6kinoko_rebuilt.c`；两处脚本生命周期符号已改名 | source-map 指向现有 C++ 宿主与具名函数，修复策略文档链接；不删除路线或符号检查 |
| stage_native / damage_pause | 远端 36246495322 在 `resource <- {}; global <- {};` 编译失败；Squirrel 2.2.2 `sqcompiler.cpp` 的 Compile/OptionalSemicolon 对顶层 `};` 后同一行的处理可复现 | 夹具在语句间换行；不修改 vendor 编译器或游戏脚本 |
| GC 断言 | 本地继续执行后发现条件表达式优先级令 `>= 0` 仅作用于 false 分支，合法的回收数 0 被判失败 | 对整个条件表达式加括号；保留链完整性、存活对象与栈检查 |
| ACT 文档生命周期 | 写入器 `object_vector` 的 `{range.begin, range.end}` 选择 initializer_list 构造，保存两个 `void**` 端点，而非范围内对象；随后虚调用崩溃 | 明确使用 `std::vector<void*>(range.begin, range.end)`；保留现有往返、父子关联、截断前缀和所有权测试，无加载规则改动 |
| 可读性清单 | 已有测试发现 `KinokoActor* acquire() noexcept(false)` 被漏识别 | 类型后允许直接跟 `*` / `&`，继续明确这是词法候选统计；预检执行已有 Python 单测，原生 contract 仍仅编译 |
| 主文档 | 根 README 覆盖 `.github` 英文入口且旧双语内容过时 | 删除根 README，英文 `.github/README.md` 为主，中文 `.github/README.zh-CN.md` 同章节、事实、命令与链接；保留图片与许可说明 |

## 本地验证

- `ci-repair-01` / `3d599c9a3db47c3bb3822791b20fbeef3070c066`：复现原有三个失败。
- `ci-repair-02` / `a68ee1ac3f686678279e925f64e6343661e75248`：脚本换行修复后，定位 GC 断言和 ACT 写入崩溃。
- `ci-repair-03` / `70186ac9dee0a58e125fa531ef42507eecff06f4`：stage_native 通过，ACT 仍失败；临时诊断用于定位，后续已删除。
- `ci-repair-04` / `89ca0f94ed74fba2d421cc66eaeb45cfb957e688`：Win32 Release 无日志全量构建成功，三个 DAT 大小与 SHA256 校验成功；与 Windows workflow 相同的 **45/45 contract 执行通过**，见 `build-runs/ci-repair-04/ctest-complete.log`。期间一次提前启动的测试因目标尚未生成而未执行，原日志和 Testing 目录保留为 `ctest.log` / `Testing-before-build-finished`，不记为代码失败或通过。
- `ci-repair-preflight-01` / `70186ac9dee0a58e125fa531ef42507eecff06f4`：source-map、边界检查通过；legacy 17、provenance 5、ACT evidence 8 个 Python 测试通过；Windows x64 extracted-rules 预检构建成功。可读性测试最初 1 项失败，修复后在 `ci-repair-04/readability-tests.log` 记录 **3/3 通过**。
- 所有批次产物、失败日志保留。未运行游戏；历史 `internal-types-59` 的运行正常结论仍是用户反馈。提取规则 x64 构建不代表游戏已支持 x64 或跨平台。

本批游戏 EXE：`runtime-builds/ci-repair-04/kinoko_retdec_rebuild.exe`。本地构建清单位于 `build-runs/ci-repair-04/artifacts.json`；其中 `tests_run=false` 是构建脚本结束时的快照，后续实际测试结果以上述日志为准。

## 远端验证

推送后记录 GitHub Actions 的实际结论，不以本地构建代替远端结果。
