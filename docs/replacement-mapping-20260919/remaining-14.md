# 最后 14 个入口核对（2026-09-20）

**后续修复：** 两项缺口已于 `710e0de` 补齐，当前 identified_gap=0；
双构建各 47 项测试通过，用户实测正常。详见 [修复与验证](gap-fixes.md)。
以下为修复前的历史调查记录。

起始代码：194487ab8761104a008fc28550ac7bf263b558f4。
本轮以原版 IDA 调用点、汇编/反编译、提供的 Squirrel 2.2.2 源码及
其 Win32 编译对象反汇编核对。没有只按地址区间判断，也没有把
“功能有对应源码”当成“宿主生命周期已正确接上”。

结果：14 个入口均已明确身份及当前去向；其中 **12 个有明确替代，
2 个发现尚未修复的行为/生命周期缺口**。不是 14 个全部重构完成。
账本中 unresolved=0，identified_gap=2；这两个缺口仍是待完成工作。

| 原入口 | 原版职责 | 当前去向／结论 |
|---|---|---|
| 40A5F0 | 音频句柄退出活动队列并排队释放 | BGM 路径由 audio_runtime.cpp::function_470360 释放；全局声音清理由 stage_cleanup.cpp::kinoko_clear_global_sound 承接。队列/所有权结构已改变，不宣称逐指令或时序等价 |
| 40A8D0 | PauseBgm：播放中停止，已停时循环播放 | **确认差异**：当前 function_470300 只 Stop 并 SetCurrentPosition(0)，没有切换恢复分支 |
| 40A9A0 | 指定句柄的音量渐变 | function_470320 → retdec_bgm_begin_fade_for_handle；原版调用者同样将百分比除以 100 |
| 46B450 | 取字符串参数；类型失败抛宿主错误 | 原调用者 471880/471960 已直接内联 sq_getstring；删除失效声明并纠正“有同名 C++ 实现”的旧注释 |
| 46F9F0 | Map 类绑定状态构造 | camera_map_binding.cpp::construct，由 function_46fac0 调用；对应实际 null-parent 调用场景 |
| 489EF0 | SQRefCounted 标量删除析构入口 | kinoko_sq_delete_refcounted，显式基类析构与 flags&1 控制释放 |
| 489F20 | SQObjectPtr 默认构造 | sqobject.h::SQObjectPtr()；原版和源码汇编均写 OT_NULL=0x1000001 与零载荷。IDA 自动贴的 DNameNode 名称不是可靠语义 |
| 48C080 | SQInstance::Get：成员表索引→字段/方法值 | sqclass.h::SQInstance::Get；由源码 SQVM::Get 调用，已比对源对象反汇编 |
| 490040 | SQVM::GrowCallStack | sqvm.h 同名方法；倍增 +104，重建 +108 向量，发布 +96 指针，源码对象汇编对应 |
| 495360 | SQVM::Execute | retdec_execute_clean_vm → kinoko_sq_execute → 源码 SQVM::Execute；保留显式 resume 类型转换 |
| 49C350 | SQSharedState 析构 | sqstate.cpp::~SQSharedState，经源码 sq_close 可到达；不代表 g643 宿主链已正确清理，见 4A8D60 |
| 4A1760 | 空脚本构造回调，返回 0 | kinoko_sq_noop_constructor，ACT 注册显式使用 |
| 4A1850 | seterrorhandler 标准库包装 | sqbaselib.cpp::base_seterrorhandler；原版注册字符串为 seterrorhandler，48A300 是设置错误处理器，不能误认成 sq_close |
| 4A8D60 | 销毁 g643 链后续节点与共享状态 | **确认缺口**：原版 4D4B30 退出清理头节点，再调此函数递归清理；当前 bootstrap 只向 g643 追加节点，没有对应项目清理链 |

## 两项待修复问题

1. **PauseBgm 的暂停/恢复语义**。原版 40A8D0 读取缓冲区状态，
   播放中调用 Stop，停止时调用 Play(0,0,DSBPLAY_LOOPING)，没有归零
   播放位置。当前停止辅助函数还修改 started/playing/play_offset，
   修复时必须同时核对 BGM 环形缓冲服务状态，不能只改一处 Play。
   应增加暂停→恢复的模拟缓冲区契约，并验证位置与服务不会错位。
2. **宿主共享状态链的退出清理**。g643 存储 state/next 两个字。
   原版出口 4D4B30 调用 49C350、释放 state、递归释放后续节点，
   最后清空 g643。当前项目只找到定义和追加链的访问。
   源码析构存在不等于宿主会调用它；修复前要明确全局脚本对象、
   root VM、shared state 的释放顺序，避免重复释放或悬空回调。
   本轮没有动态测量泄漏数量，也不将该问题描述为已观察到的崩溃。

## 修改及验证范围

- 账本增加具体 source_method_mapping 与 identified_gap，并保留缺口详情。
- 审计工具检查源码库的编译单元、已审核源码锚点和独立链接符号。
- 主 C 仅删除一个没有使用者的 46B450 声明并纠正注释；可执行函数体
  未修改。没有新建游戏 EXE 或重新启动游戏。
- 原版 14 项及相关调用者证据、源码编译对象的汇编摘录保存在
  analysis/remaining-mapping-20260920。5 个用到的上游文件与提供的
  ../squirrel-2.2.2/SQUIRREL2 对应文件字节一致。
- 链接核对仍使用 input-181b23c-{quiet,diag}-20260919 两份旧构建，
  不冒充本轮新游戏测试。原对象名称受内联/ICF 影响，找不到独立
  链接符号不等于源码实现缺失。

复核：python tests/test_replacement_mapping.py；
python tools/audit_replacement_mapping.py --link-map <quiet 或 diag 的 runtime.map> --output <本次独立输出文件>。

最终验证：4 项审计工具回归、迁移边界检查、quiet/diag 两份链接 map
审计均通过。审计输出明确列出 40A8D0 与 4A8D60 两个 open_implementation_gaps，
未将它们算作完成实现。可执行函数体未变的定向源码比较也已通过。
