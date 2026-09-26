# 内部边界、存档覆盖与失败策略收尾

基线 `f4c5be2`，当前 PR #12 分支。本批的停止条件是完成下列具体修改、补覆盖源码、记录保留依据，并完成无日志构建和 DAT 校验。不是全项目全部整数槽清零，也不重开已完成的 ACT 所有权/对象场景生命周期。

## 原版核对

使用 ida-reverse/reverse-engineering 工作流，通过技能 start.ps1/open.ps1 复用服务并打开原版临时副本。IDA MCP `18c78977` 成功取得 survey、imports、40D790/40E630 反编译及汇编；见同目录 JSON。原版 SHA256 为 `2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`，x86 原生 Windows EXE。

E-imports：同目录 imports.json 保留导入明细，涉及窗口/文件/同步、COM、WinMM、D3D 等运行边界。没有把无关导入表检查作为新增游戏修改。

旧 session a04add3d 不可达；新 session 取证完成后也出现 worker 不可达。失败查询原文存放 rpc-attempts，不作为成功反编译证据。补充依据使用既有同一原版的 [输入证据](../input-timer/)、[存档证据](../../analysis/script-modules-20260919/)、[应用退出证据](../application-lifecycle/40d940.json)。Squirrel 使用现有 2.2.2 源码 VM，辅助参考既有 source-disassembly-excerpts.json；未修改 VM 指令、引用计数或 trace 调用。

## 本批结果

1. 纹理：40E7E0 仅在 LockRect 恰好返回零时上传；40E705 返回创建纹理的 HRESULT。恢复非零锁定结果仍交出已创建纹理、返回创建结果的行为。40E815 本来就忽略 UnlockRect，因此忽略解锁错误不再作为待修复项。原有非法像素/格式/尺寸保护仍明确保留。
2. 应用：恢复启用输入时初始化→键盘→控制器枚举的失败短路；鼠标尝试失败不阻断。启用音频时，初始化失败不再静默降级进入游戏。保留既有线程 join、事件发布顺序和部分初始化资源保护，依据逐项列入台账。
3. 内部表达：属性读写的对象地址使用真实借用指针；纹理描述符/auto_size 使用已有具名布局；ACT 发布复用 DocumentRecord，层属性使用已有记录字段偏移。存档具名文件入口使用 const char*，整数路径只保留在四个原版地址别名的 ABI 边界。
4. 存档覆盖：新增 savedata_file_contract，链接真实 VM、序列化器、文件 I/O 和 zlib。覆盖嵌套往返、独立格式样本、空容器/标量宽度、失败文件和栈平衡。运行时只创建独立临时目录，不访问 marisa[A-C].dat。
5. 纹理与应用现有 contract 增加失败返回、非零成功 HRESULT、失败短路顺序、配置关闭和低字节返回覆盖。

## 保留依据台账

[retention-ledger.json](retention-ledger.json) 按 T01–T03、A01–A05、P01–P08、S01–S03 逐项记录修改/原版保留/保护性保留/范围外、原版锚点及验证状态。可保留项目不是“已证明与原版所有边界完全一致”；待定或范围外也不伪装成本批已消除。

特别保留：真实属性偏移协议与 thiscall/虚表、SqPlus 原始对象槽、非法输入保护、线程与 COM 资源平衡。其他内部槽和偏移仅在以后有具体目标与证据时整理，不能据此再次宣布整个 ACT 未完成。

## 验证状态

遵守用户测试交接：代理不运行游戏、不执行 CTest/contract。新增源码与既有回归仅构建；运行结果仍待用户验证。构建 commit、EXE 与 DAT 校验将在 HANDOFF.md 记录。本批不覆盖旧构建目录或运行产物。
