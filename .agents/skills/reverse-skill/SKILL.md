---
name: reverse-skill
description: 6kinoko简单的逆向skill
---

D:\6kinoko\6kinoko.exe是一个将近20年前的老游戏，我想加点新的内容：即用自己编写的exe，读取原版的6kinoko_*.dat。但第一版确实只需要还原windows上的运行效果即可。

逆向6kinoko项目以提供的C反编译源码为基础，使用IDA MCP，D:/squirrel-2.2.2 源码的反汇编结果作为辅助。但若确定是squirrel相关的函数，你可以选择直接引入源码。生成后必须复制 6kinoko_*.dat 文件到 exe 所在目录，不能直接指定工作目录。

主要逻辑在 .\src\decompiled\6kinoko_rebuilt.c 里，你尽量做到经过的函数完全相同（可以通过x64dbg MCP保证）

x32dbg/x64dbg 在 C:\Users\poker\AppData\Local\Microsoft\WinGet\Packages\x64dbg.x64dbg_Microsoft.Winget.Source_8wekyb3d8bbwe\release\x32\x32dbg.exe 里，使若丢失可以手动拉起进程。

不要直接读取git更改，内容太多，有必要时截取少量读取或定向读取。
每次有大量或重要修改告一段落后提交备份，可以清理无用的构建目录。

现在第一步是在修复直到出现和原版一样的标题界面即可（当前还没到原版的标题界面，commit中说的是错的）
