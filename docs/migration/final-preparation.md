# 最终准备轮：源码边界、检查与合并交接

日期：2026-09-24；第一轮交接为 `8c7c199acdaa77699399c00eadea971d9fc07042`。
本轮运行时源码提交：`f2438f6acf95a66b599c768e2b90d2bb371ff575`。
用户已明确授权优化后合并 PR #11；先检查实际最终 head 的 CI，再按该 SHA 合并。
最终合并 SHA／构建链接记录在 PR，本文不预写尚未发生的成功或游戏验证结论。

## 完成的前置整理

| 障碍 | 本轮处理 | 明确边界 |
|---|---|---|
| ACT 图层构造、清理、克隆各写一套数值偏移 | `act_layer_storage.hpp` 统一348字节宿主schema；`act_layer_lifecycle.cpp`、`act_script_lifecycle.cpp` 独立生命周期实现；clone/动态创建复用 | 仍为 x86 宿主，绝不是磁盘格式或可直接用于x64的布局 |
| 资源格式算法与 Windows 存取混在一起 | `compat/archive_index.hpp` 显式 LE 读取、有界条目cursor、共享MT解码；生产mount/wrapper复用 | 不改变Windows大小写/CRC、原挂载顺序、部分挂载失败状态或reader怪癖 |
| 统计器漏掉指针返回签名，ABI分类隐藏遗留 | schema-2、固定Git提交、包含头文件、文件级marker独立扫描、函数外marker单列 | 仍是词法候选，不是AST、调用可达性或完成度认证 |
| 清单与源码移动脱节 | source-map更新真实文件和符号；CI检查相对链接、文件和符号文本 | 符号出现不证明调用边，清单不是链接依赖闭包 |
| 合并后未自动保存迁移基线 | preflight与完整快照覆盖master，保存source revision、tree、bundle与SHA256 | Actions保留期有限，长期基线需另行归档；不包含用户原DAT |

### ACT 行为逐项保留

原版阅读入口在 `src/decompiled/6kinoko.exe.c`：41E390（构造）、41EA50/41ECA0（克隆）、
415B80（脚本构造）。同时对照本轮父提交中的实际实现；本轮不是重新推测字段语义。

图层清理仍依次处理 layout object、script object、script、keys、timelines、name、children；
外部引用的拥有标志、release/retain次序不改。对象赋值只复制VM、裸pair和flag，不复制vtable或padding。
脚本仍逆序释放三组callback，再释放payload和filename。20字节callback中的两个8字节Sqrat pair
不能替换成另一个使用三字SqPlus记录的 callback 类型。

图层保留重建基线已有的348字节zero-fill，**不是声称原版初始化了所有padding**。
脚本独立构造没有新增VM槽／未知字节清零。未顺带修复分配失败、短读、部分初始化等既有行为，
不以“更合理”替代证据。key/list虚调用适配和其他旧槽仍保留在已注明的边界。

### DAT 索引与随机状态

磁盘header仍分两次读取2/4字节，改为显式LE解释，索引条目是offset/size/path-size/path。
cursor借用decoded buffer，检查9字节固定头和路径长度，不负责mount policy或文件长度安全策略。
生产mount仍在解析前发布archive，并逐个插入entry；不新增原版不存在的事务回滚。

索引decode复用原共享MT19937，seed是32位 `index_size + 6`，每字节消费一次随机值，再做原XOR序列。
用局部新随机引擎虽可能得到相同解码结果，却会改变之后的随机数，因此没有这样替换。
payload XOR与index解码仍是不同层，工具DatArchive仍不可直接替代游戏reader。

## 新的统计口径

```sh
python tools/audit_readability.py --source-ref HEAD --scope project --output build-runs/<batch>/readability.json
```

`project`包含普通tracked src/include C/C++文件，排除原版参考、vendor和tests；
`cmake`保留顶层CMake文字列举路径的近似范围，二者都不是CMake实际配置后的编译闭包。
输出路径必须是新文件，不再覆盖 `docs/readability-r138/inventory.json`。
报告固定source commit、每文件Git blob与集合指纹；工具只读源码，不运行游戏或契约。

修复 `T *name`、`T* name`、`noexcept(false)`；ABI候选不能优先盖掉已发现的数值偏移／地址调用。
构造初始化列表、宏、操作符、模板实例与条件编译仍有限制。因此独立保留整个文件的markers及
未落入任何函数候选的marker行号。平台类型、布局断言等是审查线索，不代表每一项都应该删除。
不把 `structured_candidate_unreviewed` 改名为“恢复完成”，不输出完成百分比。

初次静态扫描运行时提交 f2438f6：326个源/头文件、2190个函数候选，其中306个带高风险词法标记、
317个薄ABI候选、1567个其他未验收候选。该数字属于这个提交与schema-2，不与历史1935条直接相减。
CI会按每次实际提交重新生成完整报告；这里只记录首次扫描以说明旧“混合遗留为零”不能作为完成依据。

## 验证与未执行项

新增 `tests/act_layer_storage_contract.cpp` 校验具名字段与原偏移、未对齐storage、guard及选择性callback写入；
顶层Windows构建会编译它，默认不注册CTest。`KINOKO_REGISTER_MIGRATION_TESTS=ON` 是用户的显式执行入口。
独立preflight扩展DAT短索引、路径边界、共享随机引擎状态等契约源码，默认仍不运行。
`tests/test_readability_inventory.py` 保存漏检回归源码，本PR不自动执行；语法检查不等于测试通过。

本地只做静态检查与独立Linux规则目标编译，不运行游戏或自动测试；未取得原DAT，不做staging/原资源校验。
Windows x86是否编译、既有筛选的CTest是否通过，以及两平台preflight结果，都以最终提交对应的CI记录为准。
不能把45项既有筛选成功、布局编译成功或静态报告当作原版体验验收。

## 迁移可以从哪里开始，什么仍未关闭

本轮合并后的master可作为**新仓库的源码／证据导入起点**，不应再重复做这批字段和路径梳理。
按 [source-map](source-map.md) 搬运核心及必要legacy边界、vendor补丁和原版证据，不仅复制reconstructed目录。
保留全部历史或完整bundle；引擎、自制内容与用户的原DAT/存档分开，校验许可证与资源来源。

| 下一关口 | 为什么不在这轮强行清零 |
|---|---|
| CV4的32位序列化、Squirrel数值与宿主指针/hash宽度 | 需要明确的兼容decoder/VM方案和跨架构验证，不能只改typedef |
| x86宿主布局、Windows/D3D/音频/计时边界 | 替换属于真正平台迁移，应逐后端对照，而非删除断言或强转 |
| 游戏输入、碰撞、舍入、渲染/音频延迟和失败链验收 | 缺少本提交原DAT与用户实机证据，不以推断冒充验收 |
| 尚存的legacy字段/调用与完整A/B/C/D/E人工台账 | 词法审计现在可见，但仍需要逐项原版核对，不能把候选数当认证 |
| mod API、profile与不可信内容限制 | 属于明确的新功能，不能混入原版无mod加载路径 |

因此“本轮准备完成并可开始迁移”与“所有障碍都已消失”不同。保留这些可执行关口，
比一次性抹掉旧ABI并宣称全绿更有利于保持手感。最终合并不等于用户游戏验证已完成。
