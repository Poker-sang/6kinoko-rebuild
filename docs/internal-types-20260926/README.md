# 内部类型、字段布局与旧接口迁移交接

本轮范围已收尾：恢复已确认内部指针的类型、具名字段和所有权，删除重复整数转发接口；不迁移 Windows、DirectX、输入、音频设备或 x64 调用约定。原版反编译文件 `src/decompiled/6kinoko.exe.c` 未修改，也不作为 rebuild 的实现重新引入。

最终构建与源码版本见 [HANDOFF.md](HANDOFF.md)。**构建通过不等于运行验证通过**；按用户要求，没有启动游戏、CTest 或任何 contract 程序。

## 已完成清单

| 范围 | 结果与依据 |
| --- | --- |
| Squirrel 源码接口 | VM、共享状态、对象、GC 接口使用源码指针；删除旧地址别名和无调用者 GC 转发。映射见 `source-vm-pointer-map.json`、`removed-squirrel-aliases.json`。 |
| 原生脚本入口 | 回调参数使用 `SQVM*`，描述符方法目标、类型身份、复制回调和元数据输出使用指针；方法的接收者调整量仍是数值。见 `native-entry-map.json`、`squirrel_binding_detail.hpp`、`squirrel_variable_record.hpp`。 |
| VM 所有权与宿主缓存 | 根对象缓存、ACT VM、共享状态和延迟销毁列表使用指针；仍先释放当前 VM 的外部引用，再切换 VM；延迟状态仍按原列表顺序在退出时销毁。 |
| ACT 文档、资源、加载与保存 | 工厂返回指针；读取器、写入器、资源绑定、序列化查询和生命周期入口类型同步。见 `act-factory-pointer-returns.json`、`act-loader-reader-map.json`、`act-stream-pointer-map.json`。 |
| ACT 阶段与属性别名 | `RuntimeRecord`、阶段属性别名、脚本回调和层对象记录替代整数槽；脚本注册保持对环境字段的实时借用，未将其改为回调前快照。 |
| 克隆与销毁 | 层、Key、2D/地图/文字克隆及删除入口使用真实指针；原版浅复制、部分字节复制、数组 cookie 和逆序销毁约定保留。见 `act_key_records.hpp`、`act_layer_storage.hpp`、`map_layout_lifetime.cpp`。 |
| ACT 容器 | 数组保存 `void*`；双向链节点保存节点指针和借用对象指针；阶段列表有独立拥有者，阶段文档、运行实例和 holder 的所有权未合并。 |
| 对象池与动画 | Actor 池接收者、返回 Actor、所有权列表和虚表字段类型化；动画索引节点保存 `KinokoAnimation*`，索引只借用，动画列表仍拥有帧与 payload。清理顺序不变。 |
| 文字与字体 | 布局、字体渲染器、atlas、队列、矩形顶点使用已恢复记录；保留字体初始化的局部清零、atlas 像素浅复制和原来的负零值。映射见 `string-pointer-map.json`、`string-lifecycle-pointer-map.json`。 |
| 地图与网格 | 删除重复地图转发；布局/相机/渲染队列入口类型化；网格子项范围、查找结果和虚调用对象使用指针。见 `removed-map-aliases.json`。 |
| 每帧与绘制 | 更新、准备绘制、绘制和 BitBlt 入口使用运行实例/资源指针；队列保留插入顺序和重复项，不获得图层所有权。 |
| 存档流 | 流前缀明确为 buffer 指针、position、limit；经 memcpy 支持既有非对齐夹具。对象临时值使用有名记录，磁盘类型标签及数值位不变。 |
| 宿主旧接口 | 删除只供旧测试调用的层构造、脚本注册、碰撞、地图、动画和 render append 整数转发；测试改走生产具名接口。消息字符串入口类型化，移除无意义 `_abi` 名称。 |
| C/C++ 虚表声明与夹具 | C 声明与 C++ `decltype` 对齐，克隆/销毁探针匹配真实签名；测试保留原版原始布局与机器字的断言用途。 |
| 资源虚表名称 | 六个地址式资源发布名称恢复为 Chip/Texture/RenderTarget 的 Object/Table 绑定，顺序和行为不变。见 `resource-binding-entry-map.json`。 |

## 逐项保留台账

以下不是未完成的同一轮重构任务，不能仅凭出现数字、`int32_t` 或偏移再次整体重开。

| 保留项 | 依据与边界 |
| --- | --- |
| 文件格式偏移、长度、标签、资源 ID、纹理/音频/对象池句柄 | 它们是协议整数或索引。活动 BGM 值由播放管理器分配，是句柄编号，不是对象地址。 |
| Squirrel 对象 type/data 两字表示 | data 随 type 可以是整数、float 位、bool 或引用；在真实 SQObject 边界转换，不能全部当指针。脚本表中的类型身份键依原版仍是整数键。 |
| `Variable::offset` 与读取器 source | 原版 SqPlus `VarRef` 同一字段同时表示字段偏移、常量位或静态地址；`Constant/Static` 标志决定解释方式。描述符身份已类型化，混合载荷必须保留。写入函数的最终目标已改为 `void*`。 |
| 脚本可见的地址形整数返回 | SetTake、阶段遍历、清空队列等原入口可能返回节点/哨兵的机器字；内部对象是指针，只在保留原脚本返回约定时转为整数。不可擅自改成 bool、0 或另一个节点身份。 |
| 原版字段尺寸和部分复制区间 | 编译期 `sizeof/offsetof` 断言约束当前 Win32 记录。未知字节、padding 和部分 sprite/atlas 复制保持原宽度，不杜撰字段含义，也不扩大复制范围。 |
| 动态属性的字段偏移 | 脚本属性描述符根据属性名选择偏移与数值类型，是数据驱动接口。实际接收者使用指针，不能删除描述符偏移而改变属性映射。 |
| 真实虚调用和 Win32 thiscall/fastcall | 虚表目标、ECX 接收者、未使用 EDX、栈参数/清栈顺序依原版。具体入口已经类型化；`legacy_abi` 的通用机器字参数还用于 float 位、按值对象以及调用约定夹具，不是再次调用原 EXE 地址。 |
| 外部布局的 raw `int32_t*` 视图 | 它们是指向字节/机器字记录的指针，并非以整数保存地址；用于已有脚本对象边界与原始布局夹具，配合具名记录或 memcpy。 |
| 数值诊断接口 | `kinoko_trace_ref_watch`、`kinoko_trace_squirrel_table_entries` 等输出原来的地址位/对象数据位；不赋予所有权。无日志版仅在输出端静默，不裁掉 VM trace 调用。 |
| 平台字段、COM/HWND/HANDLE 与导入桥 | 本轮不替换 Windows/DirectX/DirectInput/DirectSound 实现，不改变设备失败/线程/事件策略，不宣称已能 x64 或跨平台构建。 |
| 原始反编译文本、历史证据、构建兼容名称 | 原始地址名及 retdec 文本在证据和稳定 CMake 目标/宏/EXE 名中保留。它们不能被算作仍在编译旧主 C 文件的证据。 |

## 证据与验证状态

- 本目录保存原版 IDA 的 0x41E790、0x450950、0x450E30、0x471160、0x48A400 查询记录；其余已恢复行为沿用相应模块既有证据，Squirrel 对照 vendored 2.2.2 / SqPlus / Sqrat 源码。
- `pointer-candidates.json` 是本轮初始候选，`abi-baseline.json` 是历史引用快照，均不是待办清单。最终核对初始候选中仍为整数的接收参数，剩余为表诊断/引用诊断的数值载荷。
- 本轮不新增游戏规则，不增加失败回滚或修改绑定/克隆/析构的先后顺序。整数改指针的 Win32 宽度由记录断言及全量编译约束。
- 游戏及 65 个 contract EXE 编译；实际游戏、异常路径、存档往返和 contract 执行均待用户验证。过去用户确认的版本不代表本轮运行通过。
- 每次构建前提交源码；所有独立构建目录、失败日志及 DAT 均保留。完整批次索引见 `build-history.json`，最终产物及 SHA256 见 `artifacts.json`。

下一步不再重复开启 ACT 全链整理。先以本轮产物接收用户运行反馈；平台后端迁移属于另一个范围，尚未开始。
