# rpgmtranslate-qt

[README English](./README.md) · [README на русском](./README-ru.md)

<p align="center">
    <img src="./icons/rpgmtranslate-logo.png" alt="Description" width="256"/>
</p>

使用 C++23/Qt6 从头重写了[原版 RPGMTranslate 应用](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate)。

![界面](./screenshots/gui.png)

## 功能特点

- [x] 跨平台、便携、快速、强大。
- [x] 卓越性能，由底层 Rust/C++ 代码驱动。
- [x] 支持 RPG Maker XP/VX/VXAce/MV/MZ，自动解密归档并解析文件。
- [x] 简洁的纯文本格式，可手动编辑。
- [x] 翻译可导出为其他格式——XLSX、CSV、XML、YAML 和 JSON。
- [x] 内置 [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) 辅助翻译功能，支持 40 多种语言。
- [x] 以便利为导向的用户体验，支持书签、快捷键和高度可定制化的功能。
- [x] 内置批量文件处理，包括批量翻译、批量去空格和批量文本换行。
- [x] 集成机器翻译 API——Google Translate、DeepL、AI 端点、本地端点、OpenRouter。
- [x] 集成资源检查器，可查看图像、视频、字体、脚本等内容。
- [x] 集成拼写检查和高级检查功能，包括各类插件（如 Yanfly Message Core）标签的检查及信息提示。
- [x] **部分**集成了 Git 客户端。
- [x] **大概**集成了 LanguageTool。

**功能请求：** 由于可以使用 Rust 和 C++ 实现几乎任何功能，欢迎提出您和其他人需要的功能请求，我们会实现它们。

本程序在底层使用了以下工具：

- [rvpacker-txt-rs-lib](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib) 用于解析 RPG Maker 文件中的文本并应用翻译。
- [marshal-rs](https://github.com/RPG-Maker-Translation-Tools/marshal-rs) 用于将 RPG Maker XP/VX/VX Ace 文件解析为 JSON。
- [rpgm-archive-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-archive-decrypter-lib) 用于解密 `.rgss` RPG Maker XP/VX/VX Ace 归档。
- [rpgm-asset-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-asset-decrypter-lib) 用于解密 MV/MZ 资源。

程序使用这些工具将文本解析为 `.txt` 文件，方便编辑，然后再将翻译后的内容写回原始格式。

如果在使用程序时遇到困难，请查看顶部菜单中的 `帮助 > 使用文档` 选项。这可能会有帮助。

## 发布版本

发布版本发布在 [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases) 页面。

Windows 版本需要 Windows 10 或 11。程序为静态链接，无需额外安装其他依赖。

每个版本都需要 CPU 支持 **x86-64-v3**：Intel 从 Haswell（2013 年）起，AMD 从 Excavator（2015 年）起。更老的硬件需要从源码构建并使用更低的 `-march`。

可执行文件使用 `upx --best --lzma --brute --ultra-brute --compress-exports=0` 压缩。

在 Linux 上运行此程序需要安装[这些依赖项](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/development#getting-development-headers-on-linux)。

## 文档

完整文档位于 <https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/>。

### 安装

请参阅[安装文档](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/installation)。

### 开发

请参阅[开发文档](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/development)。

## 支持

项目的维护者[我](https://github.com/savannstm)是来自东欧的贫困大学生。

如果您愿意，请考虑通过以下方式支持我们：

- [Ko-fi](https://ko-fi.com/savannstm)
- [Patreon](https://www.patreon.com/cw/savannstm)
- [Boosty](https://boosty.to/mcdeimos)

即使您不支持，也没关系。我们会继续做下去。

## 许可证

本项目采用 [WTFPL](https://www.wtfpl.net/) 许可证。

仓库中包含的第三方软件，许可证条件各异：

- `icons` - 包含 [Google Material Symbols](https://fonts.google.com/icons)，采用 `Apache License Version 2.0` 许可证。
- `src/3rdparty` - 包含第三方库：
    - [fast_float](https://github.com/fastfloat/fast_float)，[MIT](licenses/fast_float)
    - [jeaiii/itoa](https://github.com/jeaiii/itoa)，[MIT](licenses/jeaiii-itoa)
    - [magic_enum](https://github.com/Neargye/magic_enum)，[MIT](licenses/magic_enum)
    - [zmij](https://github.com/vitaut/zmij)，[MIT](licenses/zmij)
    - [miniaudio](https://github.com/mackron/miniaudio)，[Unlicense](licenses/miniaudio)
