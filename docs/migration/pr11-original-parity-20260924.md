# PR #11 原版行为修正交接

源码提交：`90a66db8a2ddf0bcb9a9ace69fcca6505b4207f4`，基于 PR #11 合并提交
`1ec155b6d390601c3eca33c6f540e1bf74020b74`。本批没有运行游戏或本地自动测试。

## 原版依据与改动

- 原版 `0x41E390` 只初始化图层选定字段，并在构造末尾清零偏移 4..71 的属性区。
  移除 348 字节整块清零，显式写入 children、parent、资源关联、名称、位置、
  list 状态和脚本引用；克隆时仅复制偏移 92 的标志字节，不读取旁边未初始化字节。
- 原版 `0x402D40` 在打包模式用 260 字节 `strcpy_s` 缓冲区，并覆盖路径末四字节。
  移除独立的路径规则 helper 及空文件、64 MiB 的重建版拒绝分支。
  少于四字节的路径在原版会越界写入，重建版仍拒绝该无定义输入；
  `size + 1` 溢出也仍拒绝，均不声称是原版规则。
- 原版 `0x410500` 按磁盘中的 32 位索引长度分配，没有 256 MiB 限制。
  移除重建版上限；索引截断时的 cursor 边界检查仍作为内存安全保护保留。
- Squirrel 原版 `0x48ABE0` 将对象 pair 写成 null type 和零值；
  `squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp` 的 `sq_resetobject` 与此一致。

## 构建与资源

- Win32 Release、`KINOKO_RETDEC_DISABLE_TRACE=ON`：
  `build-runs/pr11-original-parity-90a66db`，构建成功。
- 独立 x64 migration preflight：
  `build-runs/pr11-original-parity-preflight-90a66db`，仅编译成功。
- 游戏 EXE：`runtime-builds/pr11-original-parity-90a66db/kinoko_retdec_rebuild.exe`，
  SHA256 `D94EB6BC0942B674D04FA9BD3B368ED258E5B0F4A14B41CB42F756D0845E8B07`。
- `stage_dat.ps1` 已将原版三个 DAT 复制到该 EXE 同目录并校验大小和 SHA256：
  `6kinoko_a.dat` `DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`；
  `6kinoko_b.dat` `4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`；
  `6kinoko_c.dat` `80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`。

新增和调整过的契约源码已编译，但未执行；以上构建与资源校验不等于游戏运行验证。
