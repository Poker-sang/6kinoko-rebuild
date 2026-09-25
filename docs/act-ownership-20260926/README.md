# ACT、资源、阶段/地图：失败与所有权链

本批覆盖创建、解析、发布、克隆/替换、失败保留、最终释放。以原版 EXE 的 IDA MCP 静态证据为依据，不把重构中的安全处理说成原版行为；不宣称所有输入和脚本回调均已动态验证。

原版：`C:/WorkSpace/6kinoko/6kinoko.exe`，SHA256 `2DB975A408E260499D52126F25ECF2FBC529CAD52D1BFFC2A0B7CA2FF695F155`。本目录 `original-*.json` 为本批重新取得的 IDA 反编译/汇编证据；函数内地址请求可能归一化到函数入口。此前证据见 `../original-act-flow-20260921/`、`../map-manager/`、`../mesh-3d-layouts/`。

## 所有权与失败矩阵

| 链 | 所有者 / 借用关系 | 失败、移交与释放规则 | 原版证据 |
|---|---|---|---|
| 文档创建 / 文件读取 | 调用者拥有文档；文件调用拥有 reader | 构造时只初始化确定字段；reader 在返回/展开时关闭，解析失败不销毁调用者文档；seek 返回不作失败条件 | 427530、428000；原有 unwind 证据 |
| ACT 解析 | 文档拥有已发布的层/资源；临时关联表只借用 | 成功对象逐个追加，失败保留已发布对象；父关系在资源计数之前解析；再次加载不替换旧数组 | 428150、4295D0、429630、455FD0 |
| 层 / key / timeline / layout | 层拥有两条列表的载荷；key 拥有 layout；父子/资源边只借用 | 层先按顺序虚释放 key、timeline，再释放脚本包装及脚本，随后 timeline 节点、key 节点、名字、子指针容器；layout 使用 Dispose 槽，不能当作 CAct deleting destructor | 41E5C0、426580 汇编、420900 |
| 文档克隆 | 新文档拥有克隆资源和层；关联索引只借用 | 先资源后层，通过各自 Clone 虚函数；成功追加后移交所有权，再建立关联。保留已有 native ABI 的异常转空返回边界 | 427950 |
| 资源加载 / suspend / resume | 文档拥有资源；加载 pass 借用 | query 顺序纹理、目标、mesh、chip；累计结果但继续后续对象；先改 suspended 状态再调用资源；不因 mesh 返回 0 改写成功语义 | 4289C0、428AF0、428BD0、44CA50 |
| 纹理 / 渲染目标 | 自有句柄与 borrowed 标志分开；native clone 的额外 store 引用单独记账 | borrowed 且无额外引用不能释放句柄；销毁自有目标先恢复默认目标；派生成员先于 base name 释放 | 4462C0、449360、401E60 |
| Chip / MCD | resource 对共享 MCD control 持有一个 strong 引用；MCD 拥有纹理集合 | loaded_path → shared data → source_name → base name；重载临时数据成功才替换旧数据；字段 +68 改为具名真实 control 指针 | 42F1B0；原有 42FB 资源加载证据 |
| Mesh | resource 拥有 controller/state 和 render records；lookup/replacement 句柄借用 | 保留 controller 先于 draw records 的释放；保留原版 load 返回 0；文档 Dispose 虚调用可到达 mesh 专属实现 | 44C1C0、44CA50；mesh 目录证据 |
| 阶段 Load | owner 分别拥有 source、holder、runtime；holder/runtime 借用 source | 忽略文档和资源 load 结果，继续构造并发布；没有全阶段回滚。无全局列表时归调用者拥有 | 466100、455E40 |
| 阶段 Clear | 全局列表拥有已发布 owner | holder → source 虚析构 → 重新读取 runtime → runtime/free → owner；不改成地图顺序 | 465F70 |
| 地图 Load / Clear | manager 拥有 source、holder、runtime，脚本对象拥有 external ref | 文档失败只删 source；资源结果、450E30/450950 返回值不作回滚条件；CreateInstance 失败按 SqPlus 抛错且保留 manager 所有权；clear 为脚本/容器 → runtime → holder → source | 46F6D0、46F620、4A90C0 |
| BeginStage / 活动克隆 | runtime 拥有 active_document 和 active_holder | 锁内先 resume source，验证表后克隆；虚 Clone → 虚删旧 clone（新旧相同不删）→ 存新 clone → 构造/替换 holder；资源/层登记和 Init 返回值不阻止后续对象；异常路径释放锁 | 450950 |
| Runtime Dispose | source_holder 借用，active_document 自有 | stop/unregister、find handles、lock/name/draw storage、active holder、active doc 虚析构、native environment external ref；不读取可能已经被阶段释放的 source | 450020、4513F0 |
| 文档最终释放 | 文档拥有两组 payload 和数组 backing | 层 Dispose 全部完成 → 资源 Dispose 全部完成 → resource backing → layer backing → script/path/name；数组 deleting dtor 从末项向前 | 427610、417DD0、4AB399 |

## 本批修正

- 撤回地图基于负 HRESULT 清空 manager 的分支；恢复原版 SqPlus CreateInstance 异常传播，不把失败变成 null 实例或地图回滚。
- 将文档/运行时/阶段/地图/克隆临时对象的释放收口到具名 `act_ownership.hpp`，区分 Dispose 与 deleting destructor。
- 恢复文档、层、key、纹理、chip、render target 的析构顺序、借用规则和虚分派；CAct 数组析构改为逆序。
- ACT 解析改为追加与明确移交；再次加载父关系时移除旧父的借用边。未发布对象的 native RAII 不会清空文档中先前已发布的对象。
- BeginStage 恢复 source resume、先验证 ACT/global/resource 表、虚克隆、替换顺序与虚登记；忽略原版未检查的登记/Init 返回值；锁以 RAII 覆盖异常退出。
- 新增无运行依赖的地图入口故障注入源码、生产析构虚回调源码；扩展实际 ACT 文件再次加载、旧父解绑的回归源码。均只交付编译，不执行。

## 明确保留的实现边界

- 不是逐指令复刻 VC8/Boost 内存布局。列表和数组使用 native 容器；第三个数组槽是 backing owner，不是原版 capacity end。SqPlus/Sqrat external reference 与 VM 内部引用分开管理。
- native texture store 需要给克隆额外 retain，防止阶段先释放 source 后 clone 悬空；原版 borrowed 位仍保持，额外引用仅释放一次。这是移植的所有权适配，不能声称原版进行了同样 retain。
- 无效空参数、malloc 返回空、短文件、计数上限 65536、尚未发布载荷的 RAII、克隆 ABI catch 等安全边界仍保留；原版在部分同类输入上异常/泄漏/解引用空地址。未人为增加整阶段/整地图失败回滚。
- 解析/克隆中未发布对象的异常清理是现有 native 边界的补齐；不宣称原版所有工厂失败均会清理。
- 运行时绑定仍有整数 ABI 槽和诊断入口，未以“本批完成”冒充整个项目已无反编译遗留。动态脚本重入、OOM、损坏文件、各关卡行为的最终一致性仍需运行证据。

## Squirrel 辅助依据

`third_party/sqplus-20080713/sqplus/SquirrelVM.cpp` 的 `CreateInstance`：保存栈、调用 `sq_createinstance`、失败恢复栈并抛 `SquirrelError`。与原版 4A90C0 一致。本批新增窄的 throwing factory，其他仍需 bool 结果的调用保持原接口。

对照 `C:/WorkSpace/squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp` 的 `sq_createinstance`（原版 48B490）：类型必须是 OT_CLASS，成功压入实例。没有修改 VM 字节码或其 trace 调用。

## 验证交接

源码必须先提交再构建。仅构建 Win32 Release 无日志版与回归目标，不运行 EXE、ctest 或本地自动测试。产物及 DAT 大小/SHA256 结果在构建后的交接文件记录；游戏验证由用户执行。
