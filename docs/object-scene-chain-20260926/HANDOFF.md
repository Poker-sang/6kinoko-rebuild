# 本轮交接：对象与切关生命周期

源码版本：`2684809a19fa21311dd97ef5518f104b00a5f4e2`。行为修正在 `be172e8`；后一提交仅修正新增回归文件的命名空间引用和测试变量名。

已完成本轮指定的三条链：对象创建与脚本绑定、每帧更新与销毁、切关与重新开始。原版证据、修正逐项说明和保留的 native 边界见 [审查记录](README.md)。按用户要求，完成本轮后停止，不自动扩大到其他模块。

## 交付版本

[无日志 Win32 Release EXE](C:/Users/poker/.codex/worktrees/pr11-original-parity/6kinoko-rebuild/runtime-builds/object-scene-2684809/kinoko_retdec_rebuild.exe)

```text
C:\Users\poker\.codex\worktrees\pr11-original-parity\6kinoko-rebuild\runtime-builds\object-scene-2684809\kinoko_retdec_rebuild.exe
```

EXE 大小：912384 bytes。SHA256：`9BE058E18DA1CB0BE0D3FE32497419A3A0F9177B0AA70F88FD4AB46BA4FE7B04`。

- 游戏和全部 **63 个 contract EXE 编译成功**。
- **没有运行游戏，没有运行任何 contract、ctest 或自动测试。** 新回归仅编译；运行验证交给用户，不要求立即执行。
- 三个原版 DAT 已在 EXE 同目录，大小及 SHA256 均由 staging 脚本校验；未复制 index.dat 或存档，也未通过工作目录绕过资源位置约定。
- 上一版 `7ca77b1` 的正常运行是用户刚刚确认的结果，不能算作本版验证。
- 保留既有编译警告。第一批 `be172e8` 全量构建因新增测试文件的编译问题失败，游戏 EXE 已生成并同样完成 DAT 校验；两批目录、EXE 和全部日志均保留。

## 行为要点

创建参数在覆盖字段前独立持有，实例工厂异常按原版传播；回调和参数在异常时有明确释放边界。析构恢复基类虚表，并补齐脚本释放回调之后的成员引用释放。

每帧刷新恢复原版可见计数、输出槽、缓冲调整与空树 dirty 行为。清场在析构之后才取后继，回调新增对象按原版遍历规则处理；对象 Reset 重放 Init 后使用新的优先级。

切关与重开保持原版 DAT 脚本控制：LoadStage 同名图复用地图，InitStage 可强制重新加载；LoadStage 清对象→清渲染→清碰撞，而世界地图首帧清渲染→清对象→释放地图。没有引入统一重启逻辑或额外全局清理。

源码及交接继续推送 `codex/pr11-original-parity` / PR #12，不合并。机器可读版本、产物和日志索引见 [artifacts.json](artifacts.json)。
