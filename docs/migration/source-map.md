# 关键源码路径：从当前运行时到新仓库

基线为 `08247b5431ff6d2459a6affe58c24f51ac90c573`，叠加本 PR 的小范围边界抽取。
以下是人工核对后的**阅读／迁移路线**，不是自动生成的完整调用图；同一段中的文件不一定直接互相调用。
最终轮变更与未关闭关口见 [final-preparation.md](final-preparation.md)。
准确的可检查文件、符号、契约源码和阻断项见 [source-map.json](source-map.json)。

## 1. 启动、宿主对象、DAT 挂载、boot 脚本

入口是 `src/platform/windows_entry.cpp::WinMain`，主体在
`src/reconstructed/application_runtime.cpp::kinoko_application_run`。
`runtime_bootstrap.cpp::kinoko_runtime_initialize_objects` 初始化宿主对象；
`process_paths.cpp` 和 application 的 Win32 路径处理需要一起看。

不要遗漏 `src/decompiled/6kinoko_rebuilt.c` 中的具名宿主接口：
`kinoko_application_initialize_host`、`kinoko_application_open_archives`、
`kinoko_game_objects`、`kinoko_game_load_boot_script` 仍提供实际对象和入口。
挂载调用顺序是 `6kinoko_a.dat`、`6kinoko_b.dat`、`6kinoko_c.dat`；boot 请求是
`data/script/boot.nut`，打包路径的改写发生在脚本加载器中。

迁移边界：窗口、工作线程、COM、EXE 所在目录定位可以换后端；借用对象、启动顺序、
资源根和退出顺序不能隐式改变。只复制 `src/reconstructed` 并不足以形成可工作的核心。

## 2. DAT 容器、索引、查找与 reader

生产路径在 `archive_store.cpp` 的 mount/open-entry 和 `file_io.cpp` 的 reader。
`archive_random.cpp::kinoko_decode_archive_index` 与 payload XOR 是不同层：
前者用于索引解码，后者按条目偏移生成 key。索引解码涉及共享随机引擎，不能未经核对
就换成工具的私有 PRNG 实例。

本 PR 的 `include/kinoko/compat/resource_rules.hpp` 只抽取纯规则：LE16/LE32、
一次前缀处理、斜杠转换、payload key/XOR 和脚本选择；生产调用点已使用这些函数。
最终轮还抽取了 `compat/archive_index.hpp` 的 LE 索引 cursor 和共享 MT19937 解码算法。
生产 wrapper 仍传入同一个随机引擎，不改变后续随机数状态。没有抽走 HANDLE、挂载容器、
大小写处理或 CRC 冲突策略；损坏索引仍保持此前的部分挂载状态，不自动回滚。

必须保留的当前规则：

- 查找先移除**一个** `./`，再将 `\\` 转为 `/`；`.\\x` 不等于 `./x` 的处理结果。
- 哈希使用 `CharLowerBuffA` 处理副本，CRC 包含结尾 NUL；名字保留原字节。
  多元素链使用 `_stricmp`；单元素 CRC 链会跳过名字比较。
- `kinoko_reader_open` 在已挂载归档时走 package 分支；失败不会自动退回磁盘目录。
- 旧虚 reader 解码 clamped request，而不是 transferred count；它的 seek 还具有
  相对／绝对位置不一致的既有行为。`read_exact`／`seek_relative` 是另一个受检查的接口。
  本 PR 只共享 XOR 循环，**没有合并两种流语义**。

`src/reconstructed/archive.cpp` 的 `DatArchive` 和 `asset_store.cpp` 是另一组工具接口：
工具路径使用 ASCII 规则并处理重复前缀，`DatArchive::read` 返回原始 payload。
不能因为它更像跨平台 C++ 就直接替代上述游戏路径；也不能把工具读取结果误当作已完成游戏解码。

待迁移：HANDLE、Win32 case/locale、12/28 字节 reader 宿主布局、异常包的部分挂载状态。
恶意 mod 的安全读取可以另立规则，但不能把更改默默塞进原 DAT 兼容实现。

## 3. 独立脚本、ACT 内嵌脚本、VM 与绑定

独立文件入口：`src/squirrel/script_file.cpp::kinoko_script_load_file`。
其顺序是路径选择、打开 reader、读完整文件、识别 tag，再选择 bytecode/text 执行。
打包模式替换最后四个字节，**不是只识别 .nut 扩展名**；原有长度保护保留。
新 helper 只负责路径和 `0xFAFA` 检测，尚未实现跨位宽的 CV4 decoder。

bytecode 使用捕获的 Sqrat VM 槽，text 使用 SqPlus 的主 VM 槽。
bytecode 打开成功后，即使 load/run 报 VM 错误仍可能返回成功；text 的环境与异常协议不同。
不要统一成一个“失败全部返回 false”的执行接口，也不要凭习惯添加全栈回滚。
ACT 内嵌脚本在 `squirrel_game_objects.cpp`，其闭包持有和环境协议要独立审查。

继续阅读 `squirrel_vm_bootstrap.cpp`、`upstream_sqplus.cpp`、`upstream_sqrat.cpp`，
以及 `third_party/squirrel-2.2.2/include/squirrel.h`、`squirrel/sqobject.cpp`。
原 CV4 的数值字段宽度、原脚本整数语义与宿主指针／SQHash 宽度是三个问题。
不能只关闭 `_SQ64`，也不能只加宽 `SQInteger` 就宣布兼容。

原版文件执行证据：[original-402d40.json](../script-file-execution/original-402d40.json)。
未来 mod 源码入口可以另做，但本 PR 不改变旧 `.cv4` 选择规则或执行权限。

## 4. ACT 创建、加载、资源、阶段／地图与销毁

从 `act_document_io.cpp` 的 create/initialize/load 开始；读取期间的 reader 由局部
`ReaderOwner` 管理，payload 虚调用借用 reader，返回后按当前顺序关闭。
`act_document_resources.cpp::kinoko_act_document_load_resources` 负责资源绑定阶段。

两条调用方要分别看：`stage_runtime.cpp::kinoko_stage_load` 和
`map_loading.cpp::kinoko_map_manager_load`。阶段路径忽略部分返回值，地图路径有自己的
检查和对象交接，不能用“应当对称”推导出相同失败处理。
`act_document_clone.cpp`、`stage_cleanup.cpp` 和资源相关记录共同决定 clone、borrow、destroy。

最终轮已将图层初始化／清理移到 `act_layer_lifecycle.cpp`，脚本初始化／清理移到
`act_script_lifecycle.cpp`；`act_layer_storage.hpp` 的同一具名布局用于构造、清理、克隆及
动态图层分配。旧地址入口只转接新实现，分配大小来自 schema。
这些仍是 x86 宿主布局；key/list 的旧 ABI 和外层整数槽未全部迁移。脚本 callback 的 VM 槽和
未知 padding 不得额外清零；图层保留的是重建基线原有的整块 zero-fill，不声称原版逐字节如此。
原版和既有恢复证据须随新仓库保存。

## 5. Actor 池、更新、碰撞与脚本引用

`actor_pool.cpp` 的 construct/acquire/retire 是本 PR 第二个实际整理点：
host/pool/Lock 内部使用 `KinokoActorPool*`，旧 fastcall／虚入口才接受整数槽并转换。
80 字节宿主布局与 `CRITICAL_SECTION` 仍在，不能把这一步当作完整 x64 池。

句柄低 16 位是 slot，高 16 位是 generation；generation 超过 `0xffff` 回到 1。
先发布句柄再构造 Actor，回收槽按 LIFO 复用；retire 与最终池销毁的 delete flags 不同。
这些顺序没有改变，也没有额外修复半完成 allocation 的异常行为。

接着看 `src/squirrel/actor_lifecycle.cpp`、`actor_manager.cpp`、`actor_motion.cpp`、
`collision_queries.cpp` 与 `actor_records.hpp`。脚本引用、Actor 对象寿命、碰撞可见性和
管理器遍历时机必须一起验收。原证据：[actor-pool-evidence-r104.json](../upstream-library-audit/actor-pool-evidence-r104.json)。

## 6. 一帧的时间、输入与数值

`application_runtime.cpp::update_frame`、`timer_events.cpp::kinoko_frame_timer_wait`、
`game_runtime.cpp::kinoko_game_update` 是更新链的阅读入口。
输入在 `input_frame.cpp`／`physical_input.cpp`／`direct_input.cpp`，不能只替换设备采样
而忽略失焦、重复状态、callback 与 mask snapshot 的顺序。

数学路径是 **`src/platform/game_math.cpp`**，不是 reconstructed 目录。
`kinoko_enter_game_math` 改变舍入模式，`kinoko_run_game_math` 用作用域恢复。
x87/SSE、编译器、优化、ARM 和库函数的等价性都需要证据；不将 float 全改 double，
不预设新的 60 Hz delta-time 更新模型。

## 7. 渲染与 GPU 生命周期

从 `game_runtime.cpp::kinoko_game_draw` 看上层次序，再读 `render_queue.cpp`、
`quad_submit.cpp`、`texture_image.cpp`、`render_target.cpp`、`device_runtime.cpp` 和 `act_mesh.cpp`。

迁移重点是图层／透明提交顺序、blend、采样坐标、像素中心、render target、设备重建和
资源拥有／借用关系。未来后端不能默认排序后再靠截图补救；手感验收还包括 present 队列延迟。
本 PR 没有引入 bgfx、SDL GPU、Godot 或 Unity。

## 8. 音频设备、解码与播放调度

`audio_runtime.cpp` 的 initialize-device/play-BGM/shutdown-resources 是主入口；
`vorbis_decoder.cpp` 与 `include/kinoko/vorbis_decoder.hpp` 管理解码，
`src/platform/audio_math.cpp` 保留相关数值计算。

DirectSound／线程同步属于后端边界；循环点、gain、buffer 生命周期和播放事件是行为。
新仓库先替换设备输出，再分别验证 codec 与 mixer；不要在一次提交中同时升级全部三层。

## 9. 存档表与压缩缓冲

`src/squirrel/table_serialization.cpp` 中
`kinoko_savedata_load_file_entry`／`kinoko_savedata_save_file_entry` 连接
`read_table_entry`／`write_table_entry`；`buffer_codec.cpp` 提供压缩缓冲能力。

标签、NULL 终止、布尔单字节和整数／浮点四字节是格式，不是可随宿主改宽的对象布局。
现有记录：[savedata-table-20260923](../savedata-table-20260923/README.md)。
清单没有把某个无关契约当作完整存档 round-trip 测试；这条验证仍待补齐。
mod profile 与存档迁移只是后续方案，本 PR 不改变原存档查找和写入位置。

## 10. 证据、依赖和构建也属于要搬走的内容

保留 `AGENTS.md`、`CMakeLists.txt`、原版 decompile／analysis／docs、
`third_party` 上游版本和补丁记录，以及 `tools/verify_upstream.py` 等校验入口。
`src/decompiled/6kinoko.exe.c` 是原版证据，不等同于仍参与构建的 `6kinoko_rebuilt.c`。
`tools/audit_readability.py` 的 schema-2 修复已知 pointer/noexcept 签名漏检，并独立记录
函数外 marker、头文件和 pinned source ref。仍不是 AST，不用于认证“恢复完成”；
历史 schema-1 报告保留，不与新口径直接比较百分比。

新库的目录划分可以是 core、legacy-format、platform、tools，但这是**未来组织方案**，
不是本 PR 已移动的文件。搬运方法及前置验收见 [HANDOFF.md](HANDOFF.md)。
