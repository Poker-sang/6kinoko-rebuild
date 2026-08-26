---
name: reverse-skill
description: 6kinoko简单的逆向skill
---

6kinoko是一个将近20年前的老游戏，我想加点新的内容：即用自己编写的exe，读取原版的dat。但第一版确实只需要还原windows上的运行效果即可。

逆向6kinoko项目以提供的C反编译源码为基础，使用IDA MCP的反汇编结果作为辅助，也可参考 Squirrel 2.2.2 源码。生成后必须复制 6kinoko_*.dat 文件到 exe 所在目录，不能直接指定工作目录。

每次有大量或重要修改告一段落后提交备份，可以清理无用的构建目录。
