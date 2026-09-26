# 分析二：保真现代化、技术栈、DAT 与 mod

2026-09-24 对话中第二次分析的仓库化记录。基线 `08247b5431ff6d2459a6affe58c24f51ac90c573`。
以下是**建议与验收目标**，不是已经实现的功能。本次只做前置整理，见 [HANDOFF](HANDOFF.md)。

## 方向与时机

现在可以开始现代化，但先现代化边界，不先重写游戏。源码恢复与平台无关核心的建立可以并行，
不必等每个地址名字清零。暂时不要同时切换架构、操作系统、图形 API、脚本语言和物理规则，
否则很难定位一次细小的手感变化。保留现有 C++ 游戏核心，不让引擎替代原版碰撞、动画与更新规则。

当前硬关口来自实际源码：[顶层构建](../../CMakeLists.txt) 限制 Windows/MSVC/32 位；
[reader](../../src/reconstructed/file_io.cpp) 有 12/28 字节 Win32 布局；
[Squirrel 类型](../../third_party/squirrel-2.2.2/include/squirrel.h) 在 _WIN64/_LP64 加宽 SQInteger；
[数学入口](../../src/platform/game_math.cpp) 设置 _RC_UP。删除编译限制不会自动解决这些问题。

## 推荐技术栈及取舍

| 路线 | 对本项目的建议 | 主要取舍 |
|---|---|---|
| C++ 核心 + SDL3 + bgfx | 首选验证方向 | 保留规则、逐层替换；需要自己组织编辑器与内容工作流 |
| C++ 核心 + Godot 宿主 | 第二选择；编辑器优先时有价值 | 两套生命周期／循环／渲染接口需要衔接 |
| C++ 核心 + Unity 宿主 | 可行，但非默认方向 | 原生桥接和 Unity 资源工作流有额外成本 |
| 在 Godot/Unity 全部重写 | 当前不推荐 | 已恢复行为需要再次实现并重新验证 |

建议分工：C++ 游戏核心保留 Actor、碰撞、动画、场景；SDL3 负责窗口、输入、计时与音频设备；
bgfx 负责图形 API 后端；继续 CMake，C++20 可以逐步采用而非前置门槛；
原版脚本保留 Squirrel 2.2.2 语义；DAT/ACT 等由自己的兼容层解析；mod 采用引擎无关的模型和版本化 API。
这些组件只是拟议选择，本 PR 没有引入依赖或锁定具体发行版本。

SDL3 提供跨平台窗口和输入／音频设施；bgfx 提供多图形 API 的渲染库。
bgfx 提交排序不能按默认行为照搬透明图层，需验证顺序模式、视图次序和延迟。
SDL GPU 是合理备选：用同一批场景验证后选一个，不长期维护两套现代渲染后端。
不建议先投入通用 ECS、通用物理或一套自制通用引擎。

Godot 可通过 GDExtension 承载 C++，也可只做独立 mod 编辑器、输出本项目内容格式，
不要求运行时依赖 tscn/pck。Godot 的 PCK/ZIP 加载并不自动提供版本化 API 或安全沙箱。
Unity 支持原生插件，但 AssetBundle 是平台相关资源产物，不能作为任意新增 C# 程序集的通用扩展包。
引擎不能替你解决旧 DAT、脚本、时序和存档兼容。

音频先替换设备输出，保留解码、循环点、增益和调度，分别验证；不要同时更换混音和所有解码逻辑。

## 不改变手感的三个契约

**逻辑：** 按原逻辑时刻记录输入与状态，涵盖位置、速度、碰撞、运动状态、动画、受击停顿、
Actor 创建销毁顺序和随机数。快照采用稳定标识，不比较宿主指针或整个对象内存。
先确认原版时间推进方式，不先假设固定 60 Hz，不直接改成每帧乘 delta time。

**数值：** 保留精度、运算顺序、舍入与已确认的整数行为。x87/SSE/ARM、FMA、编译器和数学库差异
需要单独定位；/fp:strict 不是跨架构完全一致的保证，全部改 double 也不是修复。

**呈现／交互：** 核对采样时机、渲染排队、失焦恢复、控制器阈值、音频触发／循环。
D3D9 像素中心、alpha、纹理过滤和目标切换需对照。高分辨率不应改变逻辑尺寸、碰撞或可见范围；
高刷新率不应加速逻辑或多排一帧。状态一致与用户体验一致都要验证，不能互相替代。

## M0–M6 分级目标

| 阶段 | 交付 | 验收关口 |
|---|---|---|
| M0 兼容基线 | 固定源码、DAT 指纹、关键输入／状态记录 | 能判定改变了什么；未知核心行为有清单 |
| M1 可移植核心 | 分离逻辑、资源模型、文件系统与平台设施 | 普通业务不依赖 Windows/D3D 类型或整数指针槽 |
| M2 同架构换后端 | Windows 分批换输入、音频输出和渲染 | 原架构中新旧后端代表场景／延迟对照通过 |
| M3 Windows x64 | 宿主布局、脚本数值与旧字节码适配 | 同资源／输入在约定范围内行为一致 |
| M4 跨平台 | 先 Linux x64，随后按目标覆盖 macOS/ARM64 | 平台补丁不向核心扩散，有平台运行证据 |
| M5 mod 平台 | 稳定内容／脚本 API、存档 profile、管理工具和示例 | 不依赖内部 C++ 布局，兼容范围可说明 |
| M6 现代运行时 1.0 | 接口冻结、发行和回归流程 | 原版模式稳定，承诺的平台／扩展能力有验证记录 |

资源型 mod 和 x64 高风险小实验可以在 M1 并行：尤其 CV4 解码与关键数学函数。
提前实验不是整游戏已可用。原版恢复线可逐模块收尾，主线共享核心，不永久维护两套独立游戏规则。
第一份分析的 30–60 人日不包括现代化、mod 和编辑器。

## 老 DAT 的兼容设计

建议原 DAT 只读，作为统一资源入口的一个来源。直接读取与编辑器导出可以并存，不强制全盘转换。
兼容分为容器格式、查找／覆盖顺序、内容加载后语义三层；成功解包不是成功兼容。

当前基线实际按 a → b → c 挂载，见 [host 入口](../../src/reconstructed/runtime_host.cpp)。
查找层仍有包含 NUL 的 CRC、Windows 大小写规则、碰撞链和单条目快捷行为。
[reader](../../src/reconstructed/file_io.cpp) 还有 payload XOR 与两种不同流协议。
本次发现工具 DatArchive 会 ASCII 小写、反复去 ./，且 read 返回未做 runtime XOR 的原始条目字节；
这不是游戏 reader 的可直接替代品。详见 [路径说明](source-map.md)。

未来新资源入口可以显式规定“启用的 mod 覆盖 → 内容包 → 原 DAT”，这是新设计，不能冒充原版规则。
无 mod 的原版模式应沿现有查找路径；不得无意添加当前工作目录或同名散文件回退。
当前 EXE 同目录 DAT 约定继续遵守，新安装位置／导入模式应另行定义。
每次查找最好能够报告来源包、条目、解码类型和冲突原因，而不是只返回一个字节流。

磁盘字段固定宽度与宿主指针大小必须分离：显式解码 LE 字段，再构造现代对象；
不能把文件缓冲或旧 32 位对象内存直接 reinterpret 成 x64 类。
旧布局视图继续留在格式／兼容边界，未知字段在无损导出时保留原始信息，不杜撰名字或静默丢弃。
缓存可删除重建，应键入源哈希、转换器、平台/VM 配置，不能成为唯一资源来源。

## CV4 与 Squirrel 必须单列里程碑

[script_file](../../src/squirrel/script_file.cpp) 在 packed 模式替换最后四字节为 .cv4，并按 0xFAFA
分派；[sqobject](../../third_party/squirrel-2.2.2/squirrel/sqobject.cpp) 以 sizeof(SQInteger)
读取整数、字符串长度等，因此原 32 位 CV4 不可当作宿主无关的字节码。

分别解决“旧表示的解码”和“执行时数值语义”。能加载转换后的闭包不意味着计算完全一致。
可以评估维护范围明确的 2.2.2 兼容配置，将原数值语义与指针／hash 宽度分开；
不能简单关闭 _SQ64 或将所有字段改 intptr_t，SQHash 必须容纳指针。
也不应要求先把全部旧字节码反编译为 nut 才能迁移。

独立字节码、文本脚本与 ACT 内嵌脚本有不同的 VM 槽、栈、环境与错误返回约定；
不要顺手合并 VM、加统一栈清理或“修复”已开字节码文件的历史成功返回。
原版脚本环境继续兼容，mod 环境另外设计。

## mod 能力分层

资源替换最早开放；数据型扩展在内容模型稳定后开放；脚本行为在生命周期和 API 稳定后开放；
原生动态库最后考虑并明确高权限。高清纹理需要区分像素分辨率与逻辑尺寸、锚点、碰撞和帧时长。

建议开发用目录、分发用 ZIP（可命名 .kmod），清单含唯一 id、version、api_version、content_schema、
dependencies、entry 和 capabilities。该格式尚未实现，不能当作现有协议。
引擎版本、API、内容 schema 和存档版本分开；权限清单必须有真正的实现约束。

依赖拓扑和顺序必须确定，拒绝循环与不满足版本；新增和覆盖显式区分；
同资源／字段冲突可解释，不使用文件系统遍历顺序。数据补丁针对具名对象字段，不打旧 DAT 数字偏移。
初期宁可报告冲突，也不构造不透明的自动合并器。

资源解析应先决定逻辑脚本的来源与内容类型：原 DAT 走旧 CV4，mod 可提供 nut 源码。
不能让旧后缀替换抢先使新 nut 永远找不到；源码编译结果可以缓存，不作为通用分发字节码。

API 暴露创建实体、状态查询、效果和事件，返回有代次校验的句柄，不暴露指针、STL 或内部虚表。
回调是在碰撞前后还是更新结束必须写入协议；可用受控命令在确定阶段应用，避免遍历中任意销毁。
mod 命名空间与生命周期独立于原版根表。

脚本语言优先评估 Squirrel，复用现有知识，不同步再迁移原版语言；但独立 VM/根表不是操作系统沙箱。
若未来运行不可信复杂代码，另评估 Wasmtime/WebAssembly 的隔离与资源预算；这不是第一版 mod 前置工作。
原生扩展默认不自动加载，使用版本化 C ABI／句柄，不导出内部 C++ 类。

存档必须隔离：原版模式保留旧存档，mod 模式独立 profile，记录 mod 版本／顺序／schema。
依赖移除或升级后明确迁移、备份或拒绝加载，不忽略未知数据并覆盖原存档。
引擎、自制内容和用户本地原 DAT 分开分发。ZIP 路径越界、解压体积、畸形资源、脚本失控都要限制；
脚本出错后禁用 mod 不保证已半修改的世界仍有效，必要时重载场景。

## 下一批最有价值的工作

可移植核心边界；保持原查找语义的 DAT 资源系统与最小资源 mod；
CV4、数值和绑定层的 x64 小实验。三者服务同一条恢复路线，不推倒既有成果。
当前 PR 仅做这些工作的前置整理，没有实现 VFS、mod、兼容 VM 或现代后端。

## 官方技术资料（2026-09-24 检索／分析参考）

- [SDL3](https://wiki.libsdl.org/SDL3/FrontPage)；[SDL GPU](https://wiki.libsdl.org/SDL3/CategoryGPU)
- [bgfx 定位与后端](https://bkaradzic.github.io/bgfx/overview.html)；[顺序/API](https://bkaradzic.github.io/bgfx/bgfx.html)
- [Godot GDExtension](https://docs.godotengine.org/en/stable/engine_details/engine_api/gdextension/index.html)；[PCK/mod](https://docs.godotengine.org/en/stable/tutorials/export/exporting_pcks.html)
- [Unity AssetBundle](https://docs.unity3d.com/6000.0/Documentation/Manual/AssetBundlesIntro.html)
- [MSVC 浮点规则](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170)
- [D3D9 像素中心](https://learn.microsoft.com/en-us/windows/win32/direct3d9/directly-mapping-texels-to-pixels)
- [Wasmtime 安全](https://docs.wasmtime.dev/security.html)

技术文档说明能力，不证明本游戏已兼容；具体版本和后端选型需要新仓库的可复算验证。
