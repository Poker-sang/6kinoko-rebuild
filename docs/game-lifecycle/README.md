# 游戏入口与场景管理

本批将启动、关闭、每帧更新、绘制及 Scene/SceneManager 迁入
`src/reconstructed/game_runtime.cpp`，通过 `game_host.h` 借用现有管理器。
不改底层 VM、碰撞、运动数学或资源解析。

## 原版证据

参考 `../6kinoko/6kinoko.exe`，SHA256：
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`。
同目录 JSON 保存 IDA 查询结果。部分函数被 IDA 合并为共享尾块，
Hex-Rays 无法单独反编译；这些入口以 `assembly.json` 为准。

| 原地址 | 恢复的行为 |
| --- | --- |
| 469640 | 音频、脚本注册、输入、ActorManager、相机、boot.nut，按此顺序启动 |
| 469680 / 470F30 | 地图、Actor、全局对象、声音，随后依次释放 513C58、514484、513C88、5144A8 四个脚本根，再调用原有 VM 关闭入口 |
| 469900 | 输入和全局回调之后读取更新掩码；同一份快照依次控制相机、Actor、地图、全局对象；Actor 掩码写入真实 ManagerPrefix 字段 |
| 46A140 | 绘制配置前读取渲染掩码，配置后准备地图层、遍历队列，最后按快照绘制全局对象 |
| 45D9A0 | 设备状态正常才更新游戏，始终返回场景编号 0 |
| 45D9C0 | BeginScene 失败返回 0；成功后 clear、draw、EndScene，忽略 EndScene 返回值并返回 1 |
| 45DA50 | 仅场景编号 0 才分配场景；修复旧重建中先分配再检查编号造成的泄漏 |
| 45D970 | 恢复基类虚表，根据 flags 的最低位释放场景 |

六槽 Scene 虚表和五槽 Manager 虚表使用具名接口；x86 thiscall
边界沿用项目 fastcall 适配形式。应用持有 Manager，场景销毁入口管理
Scene；管理器、相机、地图、输入均为借用指针。
脚本 updateMask/renderMask 直接绑定 `KinokoGameMasks`，默认均为 -1。

## 保留的兼容边界

旧重建在脚本注册期间保存并恢复借用 VM 指针的兼容逻辑保持原样，
它不是从原版证据新增的行为。全局回调原有错误清理及 trace 调用保留。
地图、输入、脚本对象释放等尚未迁移入口的整数转换收敛到 C host；
渲染队列现有地址槽转换在调用处明确标注，不转移所有权。
去掉反编译函数中的伪异常栈操作，不为游戏增加新的流程分支。

## 编译检查与交接

新增 `game_lifecycle_contract.cpp`，检查启动/关闭顺序、回调改变掩码、
同帧掩码快照、Actor 字段写入、设备丢失、BeginScene 失败、EndScene
失败后的返回值，以及场景工厂和销毁标记。相关既有测试改用具名接口。
按用户要求，仅构建，不执行测试程序或游戏；编译成功不代表测试通过。

本批 R1：源码提交 `8b083e4fabbcef5de6c4437525c029799c890c20`。
VS 2026 / Win32 / Release，全量构建退出码 0，包括新增检查程序及既有
contract 的编译、链接。没有执行 CTest、contract 或游戏。

- 构建树：`build-runs/game-lifecycle-r1-quiet`，构建日志 `build.log`。
- EXE：`runtime-builds/game-lifecycle-r1-quiet/kinoko_retdec_rebuild.exe`。
- EXE SHA256：`065B82363D154CFB33EFE94559DDD7C18523D9E5D7E63005D63C3B561ADF21AB`。
- `KINOKO_RETDEC_DISABLE_TRACE=ON`；保留 trace 调用，仅静默输出。
- `stage_dat.ps1` 已复制三个 DAT 到 EXE 同目录，并核对大小及 SHA256：
  - a：163424746 字节，`DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`。
  - b：44681244 字节，`4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`。
  - c：11796163 字节，`80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

后续游戏验证交给用户，不将这次构建记录为运行验证。
