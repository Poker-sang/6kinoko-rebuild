# 脚本调用游戏功能的接口层

## 范围

`game_script_api.h` / `src/squirrel/game_script_api.cpp` 提供具名接口：
全局更新回调、Init 按 ID 注册、Actor 创建、地图 Actor 和事件创建、
PAT/ACT/地图加载、Actor 清理和移动、碰撞刷新/清理/创建、渲染层操作。
注册顺序与脚本名称不变。`game_script_entries.cpp` 接管全局回调与 Actor
创建的 VM 参数提取。旧 C 文件的 17 个地址命名函数体及其声明被移除。

对象、文件名和返回对象使用真实指针；脚本传值对象使用已有
`KinokoOwnedObjectWords`，保持 Win32 cdecl 的 12 字节栈布局。
全局回调用 VM、environment、closure 三个具名字段表达，并通过
RecordView 访问原有存储。

## 原版与源码依据

参考 `../6kinoko/6kinoko.exe`，SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`。
同目录 JSON 保存 IDA MCP decompile 结果。

- 469A20 / 471C70：先取得环境，再取得闭包，传参为闭包、环境。
  注册前保留副本；替换时先持有传入值再释放旧值。非闭包参数恢复默认
  callback state；临时闭包先于环境释放，传值参数也是闭包先于环境。
- 469B40 / 471DF0 / 4713B0：脚本适配器传递拥有引用的两个对象，
  创建结果拥有独立引用，先取得结果再释放传入对象。下层原生 C++
  ActorManager 接口已改成借用输入，因此不重复模拟原二进制中的中间
  传值复制链；引用持续到整个初始化结束。失败返回 null 对象并释放输入。
- 469D10 / 469DD0：地图层查询使用活动 layout；下层地图接口借用对象，
  入口负责释放传值环境/回调；缺失地图层也必须释放环境。
- 470FA0：只接受 script closure + table，键格式保持 `Init%04x`。
- 4696B0：返回 464F80 的 PAT 加载结果。旧重建固定返回 0，本批纠正。
- 469840：按加载结果低字节判断并输出布尔值，不把高位残留当成脚本结果。
- 469700 / 469710 / 469740 / 469870 / 469880：保持原管理器对象和调用关系。
- `../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp` 的 sq_addref/sq_release
  操作外部引用表；不能用内部 SQObjectPtr 的生命周期替代。继续使用
  现有 SqPlus 对象操作端口，以保留 trace、VM 接收者及兼容行为。

## 边界与验证限制

未在本批改写底层地图加载/释放、MoveActor 数学、路径分割、渲染层分派
或 VM。这些已有实现通过 `game_script_host.h` 暴露类型化借用端口，
整数地址转换只留在这些兼容边界。其他通用脚本适配器继续保留现有 ABI。

新增 game_script_contract 使用 Squirrel 2.2.2 源码 VM，检查 callback
自替换、非闭包清空、初始化键名、Actor 成功/失败返回和输入释放顺序、
缺失地图层、事件转发、PAT 成功/失败返回、地图低字节，以及真实 VM
经过新入口的参数与结果。既有 stage_contract 改用新接口。
按用户要求只编译链接，不执行这些检查，不启动游戏。

## 本批 R1 交接

- 源码提交：`ffe85ea8361bc95716782e5151dd78e41ab553dc`。
- VS 2026 / Win32 / Release；`KINOKO_RETDEC_DISABLE_TRACE=ON`。
- 全量构建退出码 0，新增 game_script_contract 及既有目标均完成编译链接。
- 构建树和日志：`build-runs/game-script-api-r1-quiet/build.log`。
- EXE：`runtime-builds/game-script-api-r1-quiet/kinoko_retdec_rebuild.exe`。
- EXE SHA256：`B08DD3AA9C3B17EC550EF33F643B8F03FEED3A4BF5AB098CB500DEF335B72A72`。
- 三个 DAT 通过 stage_dat.ps1 复制到 EXE 同目录并校验：
  - a：163424746 字节，`DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`。
  - b：44681244 字节，`4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`。
  - c：11796163 字节，`80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

未执行 CTest、检查程序或游戏。以上记录仅证明编译链接和资源校验成功，
不代表运行检查通过；后续游戏验证由用户执行。
