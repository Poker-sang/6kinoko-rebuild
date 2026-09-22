# MapManager 生命周期与每帧调度

## 原版依据

原版 `C:/WorkSpace/6kinoko/6kinoko.exe` SHA256：
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`。
本目录保存本批通过 IDA MCP 取得的反编译及关键汇编；Squirrel 仍使用仓库
已有的 2.2.2 源码实现和 SqPlus 外部引用接口。

- 46F4C0：构造脚本对象、运行实例及容器，初始化两个源指针；不清空 512 字节。
- 46F620：脚本引用 → event vector → render list → runtime → source holder → source ACT。
  runtime 析构前保存指针，析构后删除同一指针；宽高和碰撞查询结果保留。
- 46F6D0：清旧图 → 加载源 ACT → 资源 → holder/runtime → VM root → BeginStage →
  宽高 → Map 实例 → root.map → 图层名逆序数组 → root.currentMap。
- 46F8xx：通过虚 QueryType 筛选 C2DMapLayout 后才读取 owning layer。
  空图层名不被过滤；循环每轮重读 holder/count/runtime。
- 46F99A/46F9AF：无条件发布 root[ACT.name]，包括 null。旧重建读取
  三元素 current_map 数组的第五项是重建错误，不是原版 bug。
- 46EDC0：camera.center - camera.position 先舍入为 float，再 floor，写入源 ACT。
  不把它改成运行时克隆 ACT；现有绘制链路保持不变。
- 46F0B0：先 IncrementFrame，再从 manager 重读 runtime 并 UpdateFrame。
- 470100：复制源 ACT/holder 指针，auto_ptr 风格转移 runtime；自赋值保留实例。
  此处保留原版不对称所有权行为，不改成 shared_ptr 或深复制。

## 整理范围

统一 84 字节 ManagerRecord 前缀，使用 memcpy RecordView 访问已有字节存储。
源 ACT、holder、runtime 及容器持有者为真实指针；容器内事件也使用布局指针。
原 STL 区域 24..51 的其余字节保留为不透明兼容空间；不声称 84 是完整类大小。

具名接口接管构造、清理、加载、每帧更新、相机准备和赋值；Map 图层查询、容器、
碰撞查询结果和脚本属性统一引用同一字段定义。C 文件仅保留全局对象宿主入口。
移除未使用的 retdec_map_bind_resource 及旧地址函数、裸偏移主体。

保留既有重建的空参数、分配失败、VM 初始化/BeginStage 失败保护；这些不是
原版对应的正常路径行为。ACT 文件加载失败按原版只销毁刚加载的源 ACT，
不会再次释放脚本引用或清空容器。资源加载结果仍按原版忽略。

## 验证边界

新增 map_manager_contract 源码：构造边界、清理顺序、析构回调修改 runtime、
清理后字段/容器容量保留、更新顺序、相机 floor 和自赋值所有权。
既有 stage_contract 的迁移入口同步更新，保留地图切换和 x87 平衡检查。
按用户要求仅构建，不执行测试程序、CTest 或游戏；运行效果由用户验证。

## R1 构建记录

- 源码提交：`fdebb98`。
- 构建目录：`build-runs/map-manager-r1-quiet`，日志 `build.log`。
- 全量构建失败：collision_queries.cpp 中 Actor / Map 的 ManagerView 名称冲突。
- 新增 map_manager_contract 已编译链接，未执行。R1 全部产物保留。
- 修正为 kinoko::map::ManagerView 后在独立 R2 目录重新构建。

## R2 交接

- 源码提交：`c59df5ab1931f9bd88c3c793db902d18fdafc199`。
- VS 2026 / Win32 / Release，`KINOKO_RETDEC_DISABLE_TRACE=ON`。
- 全量构建退出码 0；新增及既有回归目标均编译链接成功，未执行。
- 构建日志：`build-runs/map-manager-r2-quiet/build.log`。
- EXE：`runtime-builds/map-manager-r2-quiet/kinoko_retdec_rebuild.exe`（889856 字节）。
- EXE SHA256：`CF67BE6CC7975D37D21E69FA7845C256917CA746B642083EBCABF1872DE79381`。
- stage_dat.ps1 已将三个 DAT 复制到 EXE 同目录，并校验大小、SHA256：
  - a：163424746 字节，`DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`。
  - b：44681244 字节，`4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`。
  - c：11796163 字节，`80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

未运行游戏、测试程序或 CTest。以上不代表运行验证通过。
