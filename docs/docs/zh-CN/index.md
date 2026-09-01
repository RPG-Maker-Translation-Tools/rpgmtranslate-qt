# RPGMTranslate

RPGMTranslate 是一款开源且简洁的 RPG Maker 游戏翻译工具。其主要特点包括出色的性能表现和纯文本文件格式。

该应用本身是一个便携式可执行文件。应用所写入的所有内容（设置文件、锁文件等）都尽量放在可执行文件旁边。应用数据目录可通过 `RPGMTRANSLATE_DATA_DIR` 环境变量更改，详见 [安装](installation.md#运行时数据文件)。

## 快速入门

要开始翻译任何内容，只需点击 ![](./assets/folder_open.svg) 打开一个包含 RPG Maker 游戏的目录。更具体地说，该目录应包含 `data`（或 `Data`）目录，或者一个 RGSS 存档文件（`Game.rgssad`、`Game.rgss2a`、`Game.rgss3a`）。

首次打开新项目时，在任何内容被解析之前会弹出一个"读取选项"对话框——选择您的选项（或直接接受默认值），然后点击"应用"。各选项的具体含义请参阅 [Rvpacker > 读取](rvpacker.md#read)。之后，应用会在需要时解密存档文件，从数据目录文件中提取文本，创建一个 `.rpgmtranslate` 目录，然后打开项目。

从这里您就可以开始操作了。不知道从哪里开始？[安装](installation.md) 和 [文本编辑](text-editing.md) 是最好的起点。

## 页面

|                                                  | 页面                                 | 内容简介                                                                   |
| ------------------------------------------------ | ------------------------------------ | ----------------------------------------------------------------------------- |
|                                                    | [安装](installation.md)              | 获取发行版、系统要求、RPGMTranslate 文件存放位置                             |
|                                                    | [文本编辑](text-editing.md)          | 翻译表、代码检查、键盘快捷键、批量选择                                       |
| ![](./assets/search.svg)                          | [搜索功能](search.md)                | 搜索、正则替换、放置                                                         |
| ![](./assets/construction.svg)                    | [批量处理](batch-processing.md)      | 批量翻译或执行脚本、自定义 JS 脚本                                            |
|                                                    | [任务](tasks.md)                     | 后台任务系统的工作方式：哪些可以并行运行、哪些可以中止                         |
| ![](./assets/settings.svg)                        | [设置](settings.md)                  | 设置窗口中的每一项设置及其说明                                               |
| ![](./assets/book_3.svg)                          | [术语表](glossary.md)                | CAT 术语表：创建、匹配和检查术语                                             |
| `rv`                                               | [Rvpacker](rvpacker.md)              | 读取、写入和清除翻译文件的具体工作原理                                        |
| ![](./assets/commit.svg)                          | [Git](git.md)                        | 内置 Git 客户端：目前能做什么和还不能做什么                                  |
| ![](./assets/stock_media.svg)                     | [资源检查器](asset-inspector.md)     | 浏览项目中的图片、代码、音频/视频和字体                                       |
|                                                    | [开发](development.md)               | 从源码构建：依赖项、CMake 选项、各平台注意事项                               |

图标列显示了打开各功能区的工具栏按钮（如果有的话），让您一眼就能将页面与按钮对应起来。

**同时，也欢迎您帮助我们改进[文档](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/tree/main/docs/docs)！**
