# 当前函数分类与 Squirrel 源码核验（2026-09-17）

统计基线为 d807bd0（运行代码 7a76ee4，用户已反馈上一批没有问题）。沿用以前的 4,890 个原反编译地址入口和相同文本分类算法。它不是完成率、行为等价率或全项目 C++ 占比；不包括外部 CRT/汇编辅助函数。

| 类别 | 数量 | 占比 |
|---|---:|---:|
| 保留原反编译函数体（较长） | 2431 | 49.71% |
| 已修改／替换的非空实现 | 835 | 17.08% |
| 原反编译即为短返回／空函数或包装入口 | 975 | 19.94% |
| 原同名入口已不存在（删除／替代，需看映射） | 649 | 13.27% |
| 简化返回／占位候选（此扫描规则） | 0 | 0.00% |
| 合计 | 4890 | 100.00% |

649 个没有同名入口不等于缺失 649 项功能。其中 612 个与 compiler-removal.json、destructor-removal.json、final-destructor-reachability.json 的已审计删除列表匹配；其余 37 个包括 C++/显式接收者替代、其他已记录删除，以及这次消除的三个旧占位。完整原地址清单在 functions.csv。未对全项目新增函数、第三方函数、宏展开后的函数做 C++ 百分比估计。

## Squirrel 的实际引入方式

当前 quiet/diag 构建使用 third_party/squirrel-2.2.2；KINOKO_ENABLE_SQUIRREL_CPP_VM=OFF。CMake 直接编译 squirrel/ 下全部 12 个核心 .cpp 成静态库，另编译 src/squirrel/ 下 9 个本项目桥接/适配 .cpp。第三方文件是完整核心源码目录的副本，不是仅复制几个函数；stdlib、命令行解释器、示例并未作为完整发行版全部引入。

实际采用的是混合迁移：ACT 的源码到字节码编译使用独立编译 VM；运行中的 sq_compile/sq_compilebuffer/compilestring 则调用当前 VM 上的原始 C++ Compile。对象引用操作、部分容器、栈、线程等操作经桥接使用源码；游戏主解释执行循环仍默认采用重建实现。静态库编译了整个核心，不等于最终 EXE 调用了所有源码函数；链接器也会裁剪未使用代码。完整 C++ Execute 仍是未启用的实验选项。

## 原源码是否修改

此次逐字节校验 39 个引入文件（include/、squirrel/、COPYRIGHT、HISTORY）：

- 与 ../squirrel-2.2.2/SQUIRREL2 对应文件：39/39 相同。
- 与提供的 squirrel_2.2.2_stable.tar.gz 内对应文件：39/39 相同。
- 差异文件：0。

每个文件 SHA256、压缩包 SHA256 和两组比对结果见 squirrel-source-verification.json。这证明当前副本与提供的源码及压缩包一致，不冒充额外的上游签名验证。本项目自己的 README.kinoko.md 不属于上游源码；适配修改放在 src/squirrel/、src/reconstructed/ 和 C 包装入口，而没有直接修改第三方源码。

本批只更新统计与说明，没有改运行逻辑，没有重新构建或启动游戏。修正了 README 和 CMake 注释中“只有独立编译 VM”的过时说明。
