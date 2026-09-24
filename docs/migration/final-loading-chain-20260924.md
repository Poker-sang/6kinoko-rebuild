# 迁移前加载链收尾

源码提交：`cf6c7590603c83f005ec34ae5157cbfa8c28f185`，接续
`90a66db8a2ddf0bcb9a9ace69fcca6505b4207f4`。本批未运行游戏或本地自动测试。

## 原版对照

- 原版 `0x410500` 在 `CreateFileA` 成功后、第一次 `ReadFile` 前将归档路径
  加入容器并更新归档计数。现生产挂载入口也按此顺序发布；截断索引时仍
  使用边界检查返回失败，但不回滚已发布的归档。
- 原版 `0x402D40` 在分配并清零 `size + 1` 字节后调用 reader 虚表偏移 12 的
  `Read`，不检查该调用的返回值。独立脚本入口现调用同一虚 reader 接口；
  短读留下的字节保持清零，随后照常识别字节码 tag 和执行脚本。
- `tests/file_archive_contract.cpp` 增加一个合成截断索引的登记顺序用例。
  源码已编译，尚未执行。此用例检验重建版在不越界读取的前提下保留原版
  先发布的副作用，不声称原版会安全处理损坏的 DAT。

`kinoko_stage_load` 与 `kinoko_map_manager_load` 的返回值及清理规则不同，
已有原版证据记录在 `source-map.md`；本批没有按对称性改写它们。
迁移时仍需分别验收正常关卡、地图和失败路径，构建成功不等于运行验证。

## 构建交接

- Win32 Release，`KINOKO_RETDEC_DISABLE_TRACE=ON`：
  `build-runs/migration-final-cf6c759`。主程序、归档契约和图层布局契约目标
  均编译成功；契约没有执行。
- EXE：`runtime-builds/migration-final-cf6c759/kinoko_retdec_rebuild.exe`，
  SHA256 `8CB2E8CA4B3A94CF80611D4ADA976CAED7F19EE80E93A208B24A2426A7B82A70`。
- `stage_dat.ps1` 已将原版三个 DAT 复制到 EXE 同目录并校验大小及 SHA256：
  `6kinoko_a.dat` `DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`；
  `6kinoko_b.dat` `4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`；
  `6kinoko_c.dat` `80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

后续由用户自行安排运行验证，范围为第一关、跳跃、看到怪物后退出。
届时应把结果与本页的源码提交和 EXE 指纹一起记录。
