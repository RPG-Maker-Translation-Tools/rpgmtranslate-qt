# 安装

构建版本以便携式压缩包的形式手动发布在 [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases) 页面。

## 系统要求

**CPU。** 每个发布的版本都针对 **x86-64-v3** 架构，需要 AVX、AVX2、BMI1、BMI2、F16C、FMA、LZCNT 和 MOVBE 指令集。实际上，这意味着 Intel 从 Haswell（2013 年）或 AMD 从 Excavator（2015 年）开始的处理器。更老的硬件需要[从源码构建](development.md) 并设置更低的 `-march`。

**操作系统。** Windows 版本需要 Windows 10 或 11。Linux 版本只要系统上有 libstdc++14/libc++23 或更高版本即可运行。

## 更新

RPGMTranslate 会在启动时检查是否有新版本（可在 [设置 > 核心](settings.md#updates) 中切换），也可以通过"帮助 > 检查更新"手动检查。如果存在更新版本，系统会询问您确认后才下载并安装——不会在后台自动进行任何操作。

### 发布版本

可执行文件使用以下命令压缩：

```bash
upx --best --lzma --brute --ultra-brute --compress-exports=0
```

## 发布版布局

### Windows

#### 运行时数据文件

RPGMTranslate 是便携式的。默认情况下，它会在可执行文件旁边写入三个文件：

| 文件                   | 用途                         |
| -------------------- | ---------------------------- |
| `settings.json`      | [设置文件](settings.md)       |
| `rpgmtranslate.lock` | 单实例锁                     |
| `rpgmtranslate.log`  | 当前会话的日志文件           |

设置 **`RPGMTRANSLATE_DATA_DIR`** 环境变量可将这三个文件放到其他位置：

```bash
RPGMTRANSLATE_DATA_DIR=/home/chuck-norris/.config/rpgmtranslate rpgmtranslate-qt
```

当可执行文件位于不可写入的位置时（例如 `/usr/local/bin` 或 `Program Files`），这就很关键。如果您在为发行版打包 RPGMTranslate，请全局设置 `RPGMTRANSLATE_DATA_DIR`，或者在 `src/utilities/Utils.cpp` 的 `resolveDataDir()` 中修补默认值。

### 许可证

Windows 静态发布版将所有静态链接的库（包括 Rust 依赖项及其传递依赖项）的许可证打包在发布版旁边的 `licenses` 目录中。

发布版旁边的 `THIRD-PARTY-NOTICE.md` 用于帮助您找到所需的许可证（如果您想查看的话）。

在其他平台上，只提供树内头文件库的许可证。
