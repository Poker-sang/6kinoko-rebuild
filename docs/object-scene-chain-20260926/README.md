# 对象、每帧生命周期、切关与重开链收口

用户确认上一版 `7ca77b1` 运行正常；这是用户验证，不是代理运行记录。本轮只完成用户指定的三条链，交接后停止。

## 原版证据

`original-*.json` 是本轮通过 IDA MCP 从原版 `6kinoko.exe` 获取的反编译结果，数据库 `a04add3d`。原版 SHA256：`2DB975A408E260499D52126F25ECF2FBC529CAD52D1BFFC2A0B7CA2FF695F155`。
`original-46f660.json` 的请求地址在 `46F620` 函数内部，返回内容是完整的地图清理函数。

脚本证据直接读取原版三个 DAT，经现有资源优先级选择实际覆盖版本；例如 stage.cv4 来自 c 包，WorldMap.cv4 来自 a 包。路径、偏移、大小见 `*-asset.txt`，哈希见 `script-evidence.json`，指令见 `*-bytecode.txt`。本轮只做静态读取，未执行这些脚本。原始摘出文件保留在本地并忽略提交，不更改或随 EXE 替换原版脚本。

Squirrel 外部引用依据 `C:/WorkSpace/squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp:137` 的 sq_addref/sq_release，以及原工作区 `analysis/remaining-mapping-20260920/source-sqstate-disassembly.txt` 中 RefTable::AddRef/Release；本目录保存对应反汇编摘录。实例工厂继续采用已引入的 SqPlus 源码，通过 `upstream::sqplus_create_instance` 保留 4A90C0 的 SquirrelError 行为。

## 本轮恢复

| 链 | 原版依据 | 本轮行为 |
|---|---|---|
| 创建、脚本绑定 | 463B40、45E5E0、4A90C0 | Init 在字段替换前保留 argument，再保留 callback；返回和异常均先释放 callback，再释放 argument。恢复工厂抛异常，异常时不额外回收池槽、不回滚已写字段；显式返回 false 仍按 463CB6 回收。 |
| 原生 owner 发布 | 45E68F–45E715 | 新控制块在释放旧 owner 时仍有临时强引用；释放临时强引用后才写入 Actor*，且重新读取 owner 槽。 |
| Init 回调临时对象 | 45E95E–45E9E7 | 传参前的临时引用、环境和闭包由作用域持有；交给 consuming invoke 后不二次释放。负脚本返回值仍被忽略，回调后才计算边界。 |
| 对象 Reset | 45EB00 | 弱 parent → 强 owner → 清脚本 → 重放保存的 Init → 以回调后的 priority 重建索引。引用复制收口到 Init 的借用入口，避免两套复制及异常泄漏。 |
| 每帧刷新 | 463D40 | 先发布树节点数；待销毁 Actor 先写入当前输出槽，析构返回后将当前计数减一；两个缓冲按 iteration 的容量条件一起调整。 |
| 空场景刷新 | 463D56–463D5B | 空树只写 count=0，保留 dirty 和旧渲染范围；非空刷新不覆盖 layer0.begin。取代 R141 文档中的旧兼容行为。 |
| 清场 | 463730、463800 | Reset 先 count=0；逐个减 owner 引用，必要时退休；析构回调返回后再寻找下一节点，让回调新插入的后继按原版参与清理；最后清树并置 dirty。 |
| 析构 | 45E460 | 先恢复 Actor vtable；保留既有清脚本、弱/强释放和七个脚本成员逆序释放；末尾再读取弱/强成员，释放回调期间重新安装的引用。 |

C++ 普通指针和具名布局继续表示真实对象。整数转换只用于尚未迁移的 native-control/host ABI，不新增硬编码数值地址转指针，也不以简单改名代替字段和所有权恢复。

## 核对后保留的整条调用链

- 脚本 CreateActor 的 by-value 外部引用由现有脚本适配器持有，返回对象引用先取得，再释放传入参数；地图 Init 查找结果由 InitCallback 持有。manager 仍借用这些外层参数，本轮只恢复原版传给 Init 的独立持有层。
- pool acquire 使用虚表，句柄在构造前发布；retire 先使 generation 失效，再虚析构，最后放入空闲链。native owner 控制块拥有单独 Actor* 槽，不拥有 Actor；Actor 存储属于池。没有加入异常后的猜测性回滚。
- 每帧保持 refresh → 碰撞回调 → refresh → Actor 脚本/动画 → refresh → 碰撞刷新 → motion。脚本遍历保留缓冲快照但逐个重读 count/mask；回调创建的 Actor 在后续 refresh 后参加 motion。tick 仍用调用前的 take 推进动画。
- 碰撞回调保持候选快照；一方回调后重新读另一方的 mask/function。弱碰撞代理不延长 Actor 寿命；ClearCollision 清布局和弱 parent 列表、绑定 manager 并清候选 count，缓冲本身可复用。
- ClearActor 只清活跃对象，保留动画/PAT/纹理；ClearRenderLayer 只清借用队列；ReleaseMap 只清地图脚本、容器、runtime、holder、document。全局阶段、声音和 VM 的整体释放仍属于程序 shutdown。

## 切关与重新开始：以原版脚本为准

1. `stage.nut::LoadStage` 指令 0038–0046 只有 stageName 不同才调用 LoadMap；0039/0040 的比较和跳转不能替换成每次强制重载。随后 0075–0083 无条件执行 ClearActor → ClearRenderLayer → ClearCollision → InitCamera，再建立层、碰撞、事件与地图对象。
2. `InitStage` 先清 stageActorFlagTable，并将 stageName 置空再进入 LoadStage。因此重新开始整关与同名子场景重建不是同一条资源路径，不新增统一 Restart 清理函数。
3. `UpdateStageChange` 在计数条件满足时 SavePlayerState → SetGlobalUpdateFunction(UpdateGlobal) → LoadStage(stageNameNext)。全局 update 回调执行在当帧 camera/actor/map/stage 更新之前，update mask 在回调后采样。
4. `WorldMap::Update` 0032–0037 的首帧路径是 ClearRenderLayer → ClearActor → ReleaseMap，不自动附加 ClearCollision 或全局资源清理。
5. 死亡、暂停返回和标题返回继续由 global/StagePause/stage 原版字节码的条件、存档状态和回调选择决定；没有按主观提示添加重生、复活或存档规则。

已经逐项核对以上原生边界和脚本路径；第三条链的修正落在其共用的清场、Reset 和生命周期接口中，不把脚本控制流重写到 C++。

## 回归源码与验证界限

- 新增 `actor_initialization_contract`：输入别名被替换、工厂异常、回调异常、复制中异常、外部引用释放次序、负脚本结果继续、回调后边界和 tick 的旧 take。
- 扩充 `actor_manager_contract`：析构时可见 count/输出槽、空场景 dirty、保留 layer0.begin、清场回调插入后继、update 回调触发清场后停止剩余对象工作。
- 扩充 `actor_lifecycle_contract`：析构前恢复 vtable，以及脚本释放钩子重新安装 native 弱/强引用后的末尾释放。
- `stage_contract` 中原版 LoadStage 的已有通道增加 a → a → b → a：同图重建保持 document/holder/runtime，旧 native 弱 owner 失效，动画资源保留。
- 原有 actor_state、game_script、game_lifecycle、collision、map_manager、scene_operations、application 回归仍保留。

本次要求是只构建、不运行游戏或任何自动测试。编译与 DAT 结果单独记录在 HANDOFF.md，不能把新增断言描述为执行通过。仍保留既有 native 空指针/越界句柄/分配失败保护；这些保护不被宣称为原版任意非法输入或 OOM 下的逐指令等价，也不扩张本轮范围去重写所有底层容器。
