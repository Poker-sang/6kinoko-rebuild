# Actor 方法与场景操作接口收尾

## 原版依据

参考 `../6kinoko/6kinoko.exe`，SHA256：
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`。
同目录 JSON 保存 IDA MCP 反编译结果；`462600-assembly.json` 保存
MoveActor 原始指令，作为条件和浮点比较顺序的依据。

- 45F7A0 / 4698A0：GetChipID 使用 Actor 坐标转整数、层索引和 Actor 自身
  的分层缓存地址；不添加推测的层数限制。
- 45F7E0 / 469780：先写优先级，再在全局 ActorManager 中重排。
  Release 则标记 Actor 自身的所属管理器，两者不能混淆。
- 45F800/810/820：碰撞回调、地形标记和矩形查询仍访问原全局管理器。
- 462600：迭代数量为 0 时跳过 Actor 和相机移动（46260C）。非空迭代
  按 active、registration_flag20、update_group 筛选；以移动前相机边界
  加减 64 筛选 Actor，再平移坐标、世界包围盒和 previous 字段，最后移动
  相机。循环继续条件重新读取无符号迭代数量。
- 462655..4626AF：x87 在扩展的加减结果上比较，不能先将 camera ±64
  舍入到 float；原 unordered 标志不会排除 Actor。迁移用 double 比较和
  排除条件，float 写回，源文件 `/fp:strict`，不改现有游戏线程数学环境。
- 46A830：actor_back/middle/front/water 分别读取管理器 +20/+24/+28/+32；
  其他名称交给地图层创建。队列允许重复项并保持添加顺序。

## 修改

Actor 方法参数改为 `KinokoActor*`，初始化数据入口返回 Actor 指针；
`ActorRecord::manager` 恢复为借用的 `KinokoActorManager*`，布局不变。
五个全局地址桥接函数及其七个不再使用的整数适配函数移除；方法直接调用
既有类型化管理器、碰撞和查询接口。脚本 thiscall/fastcall ABI 不变。

`scene_operations.cpp` 接管人物与相机联动、四层读取和图层分派，
消除该链路中的数值字段偏移。修复旧重建在空迭代时仍移动相机的问题。
`g618..g621` 四个镜像全局移除；启动诊断也读取真实管理器字段。
新增类型化地图层创建和队列追加接口，保留现有整数入口给未迁移调用者。
地图容器拥有地图层，渲染队列只借用，清空队列不销毁对象。

保留原重建的空管理器/相机、空 Actor 项、空名称/层保护；这些是兼容
边界，不声称原版存在对应检查。没有改动 VM、脚本名称、DAT 加载位置。

## 验证范围

新增 scene_operations_contract：空迭代不动相机；Actor 筛选、64 边界、
扩展精度边界、NaN 比较、previous 写回；非空但全部被排除时仍移动相机；
四层分派、动态替换层、大小写匹配、重复入队、绘制顺序及借用生命周期。
既有 actor_records_contract 额外区分全局管理器与所属管理器。
相关 Actor、记录布局、脚本和 stage 检查同步适配真实指针。

按用户要求只编译链接，不执行测试程序、CTest 或游戏。

## 本批 R1 交接

- 源码提交：`1f7dafaa66e5140a0f7324b24011ad90d0bf6569`。
- VS 2026 / Win32 / Release，`KINOKO_RETDEC_DISABLE_TRACE=ON`。
- 全量构建退出码 0，新增及既有检查目标均编译链接成功。
- 构建日志：`build-runs/actor-scene-r1-quiet/build.log`。
- EXE：`runtime-builds/actor-scene-r1-quiet/kinoko_retdec_rebuild.exe`。
- EXE SHA256：`F3B8C5FE55E3BC5D95458065448059249AAA405422FD0AAA84F90BD56A7C0EFE`。
- stage_dat.ps1 已复制三个 DAT 到 EXE 同目录并校验大小、SHA256：
  - a：163424746 字节，`DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`。
  - b：44681244 字节，`4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`。
  - c：11796163 字节，`80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

以上是编译链接与资源校验记录，不代表测试或游戏运行通过；后续验证由用户执行。
