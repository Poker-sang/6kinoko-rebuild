# R132 回归定位：漏迁移地图资源的延迟绑定

## 已确认结论

R132（335d71d）启用真实地图 Clone / SetLayer 链，同时删除 R131 的末尾强制绑定，暴露了地图运行时原本就缺失的延迟绑定。不是原版的 Clone 顺序有错，也不是应该删除首次绑定抑制标志。

静态证据可以确定地图绘制和位置查询的失败路径；未运行游戏或自动测试，不能声称所有用户报告症状都已验证，尤其音乐异常尚未独立归因。

## 完整调用链

1. 原版 `433AA0` 在 `433ACE` 将新地图 `+460` 置为 1。R132 启用的 `kinoko_clone_map_layout` 正确保留这一步。
2. 原版层 Clone `41EA50` 在 `41EC6D` 调用新布局虚表 `+24` 的 SetLayer。R132 的 `act_layer_clone.cpp::clone_list` 也调用它。
3. 地图 SetLayer `434380` 在发现 `+460 != 0` 时，确实在 `434554` 清空 `+316` 的资源指针，在 `43455E` 清除此标志，并记录所属层。这是原版行为；当前 `kinoko_method_map_set_layer` 在这一点没有错。
4. 文档后续通过 `41EF20` 将克隆资源写入层 `+100`。该 setter 仅写资源和 ID，不再次绑定布局。
5. **原版绘制更新 `434B60` 在 `434B93..434BAF` 检查地图 `+316`，为空时在 `434BA5` 再次调用虚表 `+24`，随后重新读取资源；只有仍为空才失败。** 之后 `434BBA` 还检查该资源是否等于所属层的当前资源。
6. **当前 `src/reconstructed/act_map.cpp::kinoko_map_update` 缺少第 5 步，直接在 `layer == 0 || resource == 0` 时返回 E_FAIL。** 所以 R132 的正常克隆状态会永久停在空资源，不能构造地图渲染内容。
7. **原版位置查询也有相同补绑定：`435720`（GetChipByPosition）调用 `435220`，后者在 `435243..435265` 检查资源，并在 `43525B` 调用 SetLayer。** 当前 `src/squirrel/act_binding.cpp::retdec_map_get_chip_by_position` 直接经 `retdec_map_chip_data` 读取空的 `+316`，跳过查询并返回 -1，且没有补绑定。

这两个消费端的遗漏共同解释了为什么只恢复 Clone 链并不等于恢复原版的完整生命周期。

## 为什么 R131 正常而 R132 失败

R131 的私有 `Clone::act` 最后执行 `retdec_act_bind_cloned_layouts`，地图走 `retdec_c2dmaplayout_set_layer_impl`，直接将层的克隆资源写进地图 `+316`。因此更新和位置查询从来不需要面对原版 Clone 所产生的首次空资源状态。

R132 删除这一末尾绑定，并改用地图的真正虚方法：首次 SetLayer 消耗 `+460` 并留下空资源。迁移没有同时恢复原版 Update / 查询中的延迟绑定，因此回归。

## 实际资源与症状

离线读取原版 DAT 的三个 ACT，记录见 `asset-scripts.json`，解析器见 `inspect_assets.py`。这是静态数据读取，不执行游戏、脚本 VM 或测试。

- `worldmap.act` 有 22 个地图层，包括 `bg_a`、`w5_bg`、`rail`、`event` 等。`event` 的 visible=0，不能指望可见层渲染顺带补好它；位置查询自身的绑定逻辑必须恢复。
- 第一关 `w1-c01a.act` 的 `bg1`、`bg2`、`terrain`、`enemy`、`event` 等均为地图布局。
- 原始 `WorldMap.nut` 的既有字节码反汇编 `analysis/world-atlas-20260919/worldmap-bytecode.txt` 显示：`GetStageNo` 在位置查询返回 -1 后返回空关卡名；`GetChipFlag` 在同类失败后返回 0。这提供关卡入口和道路识别失败的具体下游路径。
- 原始脚本 `SetBgm` 根据 `world/worldSub` 选曲。地图查询失效可能影响场景状态，但本次尚未完成音乐异常的独立因果链，不能宣称音频问题已经全部解释或修好。

## 已排除及不能当作根因的线索

- 标题、世界地图、第一关检查到的根脚本与层脚本均为文本模式，compiled=0，filePath 为空。因此“克隆把这些脚本改成 compiled 提示注释”不是其回归原因。
- 当前 Register 没有在成功后清除脚本 dirty 字段；检查的资源初始 dirty=1。不能仅凭 R132 SetText 置 dirty=1 就声称其新增了重复初始化。
- BeginStage 缺少原版资源 Resume 调用仍是另一个既有差异，不作为这次 R132 根因证据。
- R132 的编译通过不代表执行通过。已有 `test_map_set_layer` 只手动调用第二次 SetLayer，未覆盖消费者是否自行补绑定；R132 Clone 合约也只验证早期调用顺序。

## 修复边界

应按原版恢复 **地图 Update 的空资源补绑定、绑定后重读和资源一致性检查**，以及 **GetChipByPosition 所替代的 `435220` 查询入口的空资源补绑定**。不要全局改变 `retdec_map_chip_data` 的语义：其他调用者如 PreArrangement 并非都具有同样的原版补绑定路径。

后续回归源码应覆盖：真实地图虚 Clone → 首次 SetLayer 后资源为空 → 层关联克隆资源 → Update / 不可见 event 层位置查询各自补绑定。不能以手动提前 SetLayer 代替消费者行为。

本批为定位报告，没有修改生产代码、重新启用 R132、构建新 EXE 或运行测试。当前生产代码仍为 R133 的 R131 回退基线。

## 原版证据来源

原版：`C:/WorkSpace/6kinoko/6kinoko.exe`，SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`。
IDA MCP 会话 `d73baa9b`，通过标准 open 脚本打开同一 EXE 的临时副本。
本目录保存 `433AA0`、`434380`、`434B60`、`435220`、`435720`、`41EF20` 的原版反编译与地址标记；层 Clone 的原版证据沿用 `../act-virtual-clone-r132/0x41ea50.json`。
