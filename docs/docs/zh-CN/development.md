# 开发

## 前置要求

- Git
- Rust（>= 1.91）
  - cbindgen
- CMake（>= 3.25）
- C++23 兼容编译器（`clang 18+`、`gcc 14+`、`MSVC 2022+`）
- C++23 完整 STL（`>libstdc++14`、`>libc++23`、`>MSVC 2022`）。

## 必需的库

### 必需

- Qt6
- libarchive
  - liblzma
- quickjs-ng
- glaze

#### Qt6（≥ 6.8.2）

必需模块：

- Widgets
- Gui
- Network
- Concurrent
- Svg
- LinguistTools

平台相关：

- Wayland（仅限 Linux）

如果有意配置，可以省略可选的图像格式支持（PNG、JPEG 等）。

### 可选

以下基于构建标志条件必需。

#### 拼写检查（`ENABLE_NUSPELL`）

- Nuspell
  - ICU（仅 `uc`、`data` 组件）

#### 媒体 / 资源播放（`ENABLE_ASSET_PLAYBACK`）

- FFmpeg（>= 7.1.1）
  - 库：
    - avutil
    - avcodec
    - avformat
    - swresample
    - swscale

  - 必需的解码器：
    - AAC、Vorbis、Opus、Theora
    - H.264、VP9
    - pcm_s16le、pcm_s24le、pcm_s32le、pcm_f32le

  - 必需的解复用器：
    - OGG、MP4、WAV

  - 必需的解析器：
    - AAC、H.264、VP9

#### Git 集成（`ENABLE_LIBGIT2`）

- libgit2

#### Ruby 格式化（`ENABLE_RUBY_FORMATTING`）

- libclang（仅库和头文件，**不**需要完整的 clang 编译器工具链）

`rubyfmt` 的 `prism-sys` 构建脚本使用 `bindgen` 对 `libclang` 生成 FFI 绑定，因此即使项目本身完全使用 GCC/MSVC 编译，此标志也需要 clang 库和头文件。安装发行版的"clang 库/头文件"包（例如 Debian/Ubuntu 上的 `libclang-common-*-dev`、OpenSUSE 上的 `clang-devel`、Alpine 上的 `clang21-libclang`/`clang21-headers`），而非完整的 `clang` 编译器包（当发行版区分两者时）。

### 无额外库的特性标志

这些 CMake 选项控制单独的 Rust cargo 特性，无需任何额外的系统库——可自由切换以精简构建：

- 机器翻译后端：`ENABLE_GOOGLE_TRANSLATE`、`ENABLE_YANDEX_TRANSLATE`、`ENABLE_DEEPL`、`ENABLE_LLM_CONNECTOR`（OpenAI、Anthropic、DeepSeek、 Gemini、Mistral、Ollama 及其他 OpenAI 兼容/本地提供商）。
- 导出/导入 serde 格式：`ENABLE_SERDE_CSV`、`ENABLE_SERDE_XML`、`ENABLE_SERDE_XLSX`、`ENABLE_SERDE_YAML`。
- 术语表匹配词分割/词干提取后端：`ENABLE_TOKENIZER_CHINESE_ICU`、`ENABLE_TOKENIZER_CHINESE_LINDERA`、`ENABLE_TOKENIZER_JAPANESE_ICU`、`ENABLE_TOKENIZER_JAPANESE_LINDERA`、`ENABLE_TOKENIZER_KOREAN_LINDERA`、`ENABLE_TOKENIZER_SNOWBALL`、`ENABLE_TOKENIZER_SOUTHEAST_ASIAN`。

以上全部默认为 `ON`，唯独基于 ICU 的分词器（`ENABLE_TOKENIZER_CHINESE_ICU`、`ENABLE_TOKENIZER_JAPANESE_ICU`）默认为 `OFF`，因为基于 Lindera 的等效项默认已启用。

### 在 Linux 上获取开发头文件

#### Debian

项目基准为 Debian 13（Trixie）。Ubuntu 24.04 构建见下方，差异很大。

`libqjs-dev`（quickjs-ng）和 `libglaze-dev` 仅存在于 `trixie-backports`，而非普通 trixie——请先启用：

```bash
echo 'deb http://deb.debian.org/debian trixie-backports main' | sudo tee /etc/apt/sources.list.d/backports.list
```

```bash
sudo apt update && sudo apt install -y \
  git cmake ninja-build g++ libclang1-19 libclang-common-19-dev \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-wayland-dev \
  qt6-l10n-tools qt6-svg-dev \
  libarchive-dev liblzma-dev \
  libgit2-dev libnuspell-dev libicu-dev \
  libqjs-dev libglaze-dev \
  pkg-config libssl-dev libkrb5-dev
```

#### OpenSUSE

Tumbleweed 的仓库仅提供原始 `quickjs`（`quickjs-devel`），而非 `quickjs-ng`。需从源码构建和安装：

```bash
git clone --depth 1 --branch v0.16.2 https://github.com/quickjs-ng/quickjs.git /tmp/quickjs-ng
cmake -G Ninja -B /tmp/quickjs-ng/build -S /tmp/quickjs-ng -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/quickjs-ng/build -j
sudo cmake --install /tmp/quickjs-ng/build
rm -rf /tmp/quickjs-ng
```

```bash
sudo zypper refresh && sudo zypper install -y \
  git cmake ninja gcc-c++ clang-devel \
  qt6-base-devel qt6-linguist-devel qt6-tools-devel qt6-svg-devel qt6-wayland-devel \
  libarchive-devel xz-devel \
  libgit2-devel nuspell libicu-devel \
  glaze-devel \
  pkg-config libopenssl-devel libssh2-devel
```

#### Arch

```bash
sudo pacman -Syu --needed \
  git cmake ninja gcc clang glibc pkgconf \
  qt6-base qt6-tools qt6-svg qt6-wayland \
  openssl libarchive xz libgit2 nuspell icu ffmpeg \
  quickjs-ng glaze
```

#### Alpine

Alpine 在 `ENABLE_RUBY_FORMATTING` 上有一些 musl 问题，其底层的 prism-sys 构建脚本无法链接 libclang，因此如果你想构建成功需要自行解决。或者直接将 `ENABLE_RUBY_FORMATTING` 设为 `OFF`。

```bash
sudo apk update && sudo apk add \
  git cmake samurai g++ clang21-libclang clang21-headers \
  qt6-qtbase-dev qt6-qttools-dev qt6-qtsvg-dev qt6-qtwayland-dev \
  libarchive-dev xz-dev \
  libgit2-dev nuspell-dev icu-dev ffmpeg-dev \
  quickjs-ng-dev glaze \
  pkgconf openssl-dev \
  libunwind-dev
```

#### Ubuntu（AppImage）

AppImage 构建特针对 Ubuntu 24.04 以获得最广泛的兼容性。其依赖列表比其他发行版更复杂：Qt 6.8.2 不在 24.04 仓库中，因此通过 `aqtinstall` 安装，而 `quickjs-ng`/`glaze` 也没有打包，因此需从源码构建。与其在此重复所有内容，不如按照 Dockerfile 中的 `appimage-build` 步骤一步步执行——它作为此目标的唯一权威来源。

至少需要：

```bash
sudo add-apt-repository universe && sudo apt update && sudo apt install -y \
  git cmake ninja-build libc6-dev dpkg-dev gcc-14 g++-14 libclang1-18 libclang-common-18-dev \
  python3-pip \
  libssl-dev libkrb5-dev \
  libarchive-dev libgit2-dev libnuspell-dev \
  libavutil-dev libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev libswscale-dev \
  liblzma-dev libicu-dev \
  libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev \
  libfontconfig1-dev libfreetype6-dev \
  libx11-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
  libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev \
  libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev \
  libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev \
  libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev \
  libxcb-xinerama0-dev libxcb-xkb-dev \
  libxkbcommon-dev libxkbcommon-x11-dev \
  libwayland-dev libwayland-egl-backend-dev \
  libdbus-1-dev \
  libfuse2 file patchelf
```

Qt 本身、`quickjs-ng`、`glaze` 以及 `linuxdeploy`/`linuxdeploy-plugin-qt` 工具仍需单独安装/构建——详见 Dockerfile。

#### Dockerfile

仓库根目录有一个 Dockerfile，包含了上述平台的构建以及 Ubuntu AppImage 构建，它们始终保证能够成功构建。

## 配置

`configure.lua` 封装了 CMake 以提供更好的用户体验：

```bash
./lua configure.lua --fresh -G=Ninja CMAKE_BUILD_TYPE=Release
```

可用 CMake 选项可通过 `./lua configure.lua --help` 获取。

如果你更习惯，直接使用 CMake 也完全可以。

## 构建

```bash
cmake --build build
```

构建产物位于 `build/target/bin/`。

## 各平台注意事项

### Windows

官方静态 Windows 构建所用库的编译详情——请查看 <https://github.com/savannstm/personal>，特别是 dev/build-dev-libraries 脚本。

### Linux

`Dockerfile` 可在五个发行版/目标上可重现地构建，每个作为独立阶段：

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target appimage-build .
docker build --target arch-build .
docker build --target alpine-build .
```

每个阶段接受一个 `TARGET_ARCH` 构建参数（默认为 `x86-64-v3`），控制 C/C++/Rust 代码编译时使用的 `-march`/`-Ctarget-cpu` 值：

```bash
docker build --target debian-build --build-arg TARGET_ARCH=x86-64-v2 .
```
