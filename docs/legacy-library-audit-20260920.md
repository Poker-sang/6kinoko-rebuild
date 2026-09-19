# 旧库边界审计与分批替换

基线：`21e102ce73966c74a44c885595aa25c11f255f87`（master，stb_vorbis 已移入版本化目录）。
遵循 AGENTS.md 与 refactoring-plan.md；原始 `src/decompiled/6kinoko.exe.c` 未修改。
本次是有界的源码重构与死代码清理，不表示原版异常、所有旧调用约定或所有库边界已经恢复。

## 库版本与采用策略

| 库/运行库 | 可确认的证据 | 本次决定 |
| --- | --- | --- |
| Squirrel | `third_party/squirrel-2.2.2`、头文件版本、现有源码 VM 目标 | 继续使用 2.2.2，不再引入另一份 VM。 |
| zlib | `third_party/zlib-1.2.3`、现有构建目标 | 继续使用 1.2.3，不调整存档压缩行为。 |
| stb_vorbis | `third_party/stb_vorbis-1.22/stb_vorbis.c` | 保留 1.22；与请求描述不同，最新 master 已移出 decompiled。仅修复遗漏的 stage 测试 include 路径。 |
| Sqrat | 原始反编译的 Object/Class/FlexibleClass/NoConstructor RTTI、20 字节宿主对象及现有源码桥接 | 库家族明确，游戏使用的精确发行版未确认。继续通过现有薄桥直接调用 Squirrel 2.2.2，不导入最新版。 |
| SqPlus | ClassType/ClassTypeCopyImpl/ClassTypeBase RTTI、12 字节宿主对象、原生变量与方法桥 | 精确发行版未确认，不将其与 Sqrat 合并成同一种对象，也不重新引入完整绑定库。 |
| Boost | sp_counted_base/sp_counted_impl_p、bad_lexical_cast、error_info_injector/clone_impl/clone_base RTTI | 家族和部分调用路径可辨，精确版本未确认。不引入完整 Boost，不用现代 shared_ptr 覆盖旧控制块。 |
| MSVC CRT / Dinkumware | mangled imports、24 字节字符串、16 字节 inline buffer、原始增长和释放代码 | 不能只由布局认定具体 MSVC/CRT/STL 版本。将已确定的字符串边界收拢，未恢复的异常/locale 入口明确保留。 |

上游核对的范围与限制：Sqrat 历史主页 [U1] 列出 Squirrel 2.1.1–2.2.3 的测试范围，
因此存在兼容 2.2.x 的历史候选；这**不证明游戏使用 Sqrat 0.8.3**，也不证明其类布局、
错误处理和编译选项相同。对象引用语义另见 [U2]。SqPlus 官方项目 [U3] 能确认库用途，
不能据此推断本游戏版本。Boost 历史 lexical_cast 文档 [U4] 明确有流式转换、目标类型和
失败异常语义；不能仅凭函数名将它换成 atoi、stoi、from_chars 或 to_string。
本批没有复制或新供应任何第三方实现，也没有把历史候选版本写成已识别版本。

## 单独的构建修复

基线 PR CI `35458940887` 的 quiet/diagnostic 都在 `kinoko_stage_contract` 构建失败：
`tests/stage_audio_contract.cpp` 直接编译 audio_runtime.cpp，但测试目标缺少移库后的
stb_vorbis include 目录。单独的 `fix(build)` 提交只添加 `${KINOKO_STB_VORBIS_DIR}`。
修复后的精确基线 `6aedb9f95552315f6cec8c1b631939c877b5baef` 在运行
`35459136236` 中两套构建均通过，各 30 个既有无原版资源契约通过。
这不是游戏逻辑修复，也不是重新接入解码器。

## 字符串：从伪符号与固定偏移收拢到具名边界

原始证据入口：4038C0（追加）、4039E0（容量/收缩）、403BF0（子串追加）、
403CE0/403DBC（分配/增长及相邻清理）与 4066F0（赋值）。最后一项被错误标为
`_Init_locks`，但其函数体和调用者处理的是字符内容、长度和容量，并非锁对象。

新边界是 `kinoko_legacy_string`，由 `legacy_string.h/.hpp`、
`src/reconstructed/legacy_string.cpp` 和平台上的 `legacy_string_scan.cpp` 组成。
`StringRecord` 仅描述原 24 字节布局，借用的 `StringView` 通过已有 RecordView 和
memcpy 读写具名成员，接受非对齐的旧字节存储，不在其上构造现代 std::string。
长度、容量、SSO 判断和字符指针不再散落于属性 getter、ACT 容器和字符串函数中。

`retdec_string_assign_cstr` 替代锁初始化伪名称；主 C 的 31 个调用点及原生属性 setter
使用明确的字符源参数。保留原地址包装以兼容尚未迁移的 C 调用者，而不是强制一次改完 ABI。
SqPlus 字符串 getter、ACT 名称访问、原生属性和已有音频路径复用同一实现/声明。

局部新分配与别名快照由 `std::unique_ptr<char, FreeBuffer>` 管理，分配器仍是 malloc/free；
已发布到旧对象的缓冲区转回原所有者。视图本身不拥有外部记录，也不会在析构时释放宿主对象。
复制/移动使用标准库，但保留基线的容量增长、短字符串切换、自引用追加、substring 截断、
内嵌 NUL、返回值与失败时状态。低层 grow 返回的是历史整数地址，不承诺始终为保留的缓冲区。

**未在本批修改的失败行为：** 当前重建的非法长度/位置及分配失败路径不等于已恢复的
Dinkumware 异常展开。仍保持基线的返回/状态；没有臆造 throw、错误日志或游戏回退。
有界 C 字符串扫描原样移至平台文件，继续使用 VirtualQuery 与 1 MiB 上限，未改为 strlen。
原有页保护判定（包括尚未专门处理 PAGE_GUARD 的限制）也没有借重构名义改动。

## Sqrat / SqPlus 与 Boost 所有权

Sqrat 对象的 vtable、VM、HSQOBJECT、owns 字段通过 SqratStorage 的具名成员访问，
SqPlus ObjectView 同样统一到 RecordView<ObjectStorage>。消除这两处实现中的
`+4`、`+8`、`[16]` 等访问常量，并保留 static_assert 的 ABI 证据。
不是把固定偏移挪入另一个无名数字表。

保持如下不同语义：Sqrat 的栈守卫只缩栈、不补长；SqPlus 原有 StackTop 仍恢复指定栈顶。
SqPlus 赋值先 addref incoming 再 release 原值，以维持自赋值/别名的外部引用语义。
两者都没有改用内部 SQObjectPtr 或 std::shared_ptr，也没有删除 VM trace 调用。

Boost 风格 Actor 控制块已在 `actor_records.hpp` / `actor_lifecycle.cpp` 中具有具名字段，
包含外部可见的 strong/weak 计数和两阶段 dispose/destroy。当前已恢复的路径使用
Interlocked 操作，先释放 allocation 槽对应的分配，再处理弱引用；未知控制表仍走已有
精确 thiscall 方法签名。不能凭 smart_ptr 名称将其变成 `delete Actor` 或现代控制块。
本批对这段释放顺序没有再作修改，现有 actor_lifecycle_contract 继续作为回归保护。

## CRT 清理：源码和链接证据同时成立

`tools/audit_unused_crt.py` 是只读工具，按指定 git 提交读取源树并检查 map 旁的
source-commit.txt，拒绝错配。扫描运行代码、所有条件分支、头文件、第三方、测试及构建工具；
保留字符串文字和函数取址/表项引用，仅忽略 C/C++ 注释和不参与构建的原始反编译参考文件。
它不执行删除，也不将链接 map 的缺席当作唯一依据。

在修复后的基线，对 **278 个文本文件、164 个兼容函数定义、4 份 map**（quiet 和
诊断各自的游戏、stage-contract）交叉核对后，85 个入口没有有效源码引用且没有对应
x86 cdecl 链接符号；79 个入口保留。逐入口名称、原行号、函数体 SHA-256、源文件 SHA-256、
源集摘要、提交和 map 摘要见 [unused-crt.json](legacy-library-audit-20260920/unused-crt.json)。

已删除的包括未用的 ctype/stdio 小包装、算术/旧 x87 零返回桩、未引用的旧异常展开占位。
生成 C 尾部的注释式 import 清单不是有效调用。仍有引用的 __CxxThrowException_40_8、
locale/异常对象相关适配没有顺手删除，也没有冒充语义等价的现代运行库；文件说明已明确这一点。

复现（从运行 `35459136236` 的两个 artifact 解压；路径按实际解压位置调整）：

```sh
python tools/audit_unused_crt.py \
  --source-ref 6aedb9f95552315f6cec8c1b631939c877b5baef \
  --map build-runs/legacy-fixed-baseline-quiet/build-runs/ci-quiet/kinoko.map \
  --map build-runs/legacy-fixed-baseline-quiet/build-runs/ci-quiet/stage-contract.map \
  --map build-runs/legacy-fixed-baseline-diagnostic/build-runs/ci-diagnostic/kinoko.map \
  --map build-runs/legacy-fixed-baseline-diagnostic/build-runs/ci-diagnostic/stage-contract.map \
  --output build-runs/crt-recheck/audit.json
```

## 验证范围与未完成项

新增 legacy_string_contract 覆盖非对齐布局、SSO 临界值、原增长规则、别名/自追加、
内嵌 NUL、保留容量清空、收缩、非法长度/位置及页面扫描边界，包含 4,000 次确定性操作序列。
这是与标准字符串逻辑内容的对照，不是原版 EXE oracle，也未做分配失败故障注入。
native_property_contract 去掉假的赋值桩，使用真实字符串实现及真实 Squirrel getter/setter。
Windows CI 已把新契约纳入 quiet/diagnostic。最终各提交的验证状态以 PR 对应检查与 artifact
内 source-commit.txt 为准；旧构建/测试产物均保留，不能将基线通过泛化成新提交通过。

下一批需继续有证据地恢复：41A010/41A1D0、424430/424640 等 lexical_cast/流转换片段的
实参、结果寄存器、目标类型与函数边界，再确定空白、正负号、溢出、locale、浮点精度及失败
异常的 oracle。4151D0 的 bad_lexical_cast 文本仍经表项取址，不能因为缺少直接调用就删除。
__CxxThrowException_40_8 等仍有引用的占位，必须与完整调用者/异常对象/展开边界一起恢复。
本次没有将这些问题标记为已完成，也没有据猜测改成 from_chars 或 throw std::bad_cast。

原版 EXE / 三个 DAT 和原版运行环境未用于本次验证；没有完成“第一关→跳跃→怪物→退出”
的原版对照游玩。因此这里只报告构建与无原版资源契约证据，不声称游戏行为已被穷尽验证。

## 上游资料

- [U1: Sqrat 历史主页与兼容范围](https://scrat.sourceforge.net/)
- [U2: Sqrat Object 外部引用和绑定行为](https://scrat.sourceforge.net/binding.html)
- [U3: SqPlus 官方项目](https://sourceforge.net/projects/sqplus/)
- [U4: Boost 1.34.0 lexical_cast 历史语义](https://www.boost.org/doc/libs/1_34_0/libs/conversion/lexical_cast.htm)
