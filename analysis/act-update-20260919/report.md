# ACT 图层访问链迁移

基线：6432b44（master）。本批迁移 452020、452040，以及已修复接收者的
455DC0/455F50 helper；后两者改为具名 C ABI，原来的未绑定旧入口不变。
主 C 从 95,605 行降至 95,426 行，减少 179 行。
计入新增 C++（111 行）、接口头（15 行）、布局头（25 行），运行时源/头文件
净减少 28 行；测试新增 61 行，构建列表新增 1 行。不是把主 C 降幅冒充项目总降幅。

## 实现与证据

- `act_layer_access.cpp` 用 DocumentLayers、LayerKeys、KeyNode、LayoutKey
  的 RecordView 读取已确认字段，头文件逐项校验偏移。描述的是前缀，不作为分配大小。
- 临时 holder 用 Allocation 管理；只释放一个指针大小的包装，不释放 ACT 持有的
  layer/key/layout。保留原重建版本的空值保护、分配路径和 trace 标签/顺序。
- 455DC0 仍从 ACT +208/+212 的向量按索引取 layer；455F50 从 layer +180
  的链表按 +0 遍历，取节点 +8 的 key。452040 仅接受 +196 为零、+184 非零的
  layer，452020 读取 key +4 的 layout。IDA 伪代码及 452040 汇编已保存。
- E-imports：ida-imports*.json 保存原版 137 项导入（分页）；包含文件访问、
  临界区、线程、窗口、D3D/输入/声音等接口。survey 保存 EXE SHA256 和架构。
- 辅助核对提供的 `../squirrel-2.2.2/SQUIRREL2/squirrel/sqvm.cpp` CallNative
  及 `analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm`。本链是原生 ACT
  容器访问，不是 Squirrel VM 实现；没有重写 VM 或移除 VM trace 调用。

## 新增契约

stage_contract 使用独立的原始 ABI fixture，不复用生产布局定义。覆盖停用状态、
负索引/越界、空 layer、extra tracks、空 key、链表第二项、断链、反向向量、
空输出指针，以及 1,000 轮查询后借用的栈上记录仍完整。

## 后续已确认差异（本批未改）

451640 原版 4516C4 使用无符号 jnb 比较 wake_time/timeGetTime；当前重建仍是
有符号比较。451746–45175C 每轮从 source holder 重新读取图层数量，当前重建
缓存初始数量。后续应先补计时边界、回调增删图层测试，再单独恢复这两处行为。
不能把目前游玩正常视为这两处差异无影响的证明。

## 验证

源提交：`12b96e70a645e2c7c3180c5fb621a4f26b56806a`。Win32 Release：

| 模式 | 独立目录名 | CTest |
|---|---|---|
| quiet | layers-12b96e7-quiet-20260919-213334 | 45/45 |
| diagnostic | layers-12b96e7-diagnostic-20260919-213334 | 45/45 |

build-runs 下保留 configure/build/CTest/link map/stage-dat 日志和 source-commit；
runtime-builds 下保留 EXE、source-commit 和三个经过大小/SHA256 校验的 DAT。
边界扫描：220 个源/头文件通过，零手写 inline asm/naked，原始参考未改；
Python 审计解析器回归 3/3。现存反编译警告仍有，本批新文件无编译错误。

现有 f2da106 游戏仍由用户运行；遵守用户“不关闭游戏”的要求，未启动新游戏、
未发送输入或退出当前进程。因此本批未完成 quiet/diagnostic 的图形启动及第一关
冒烟，自动契约通过不等于完成真实游玩验证。测试产物全部保留。
