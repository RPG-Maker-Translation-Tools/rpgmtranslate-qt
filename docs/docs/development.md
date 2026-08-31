# Development

## Prerequisites

- Git
- Rust (>= 1.91)
  - cbindgen
- CMake (>= 3.25)
- C++23-compatible compiler (`clang 18+`, `gcc 14+`, `MSVC 2022+`)
- C++23-complete STL (`>libstdc++14`, `>libc++23`, `>MSVC 2022`).

## Required libraries

### Required

- Qt6
- libarchive
  - liblzma
- quickjs-ng
- glaze

#### Qt6 (≥ 6.8.2)

Required modules:

- Widgets
- Gui
- Network
- Concurrent
- Svg
- LinguistTools

Platform-specific:

- Wayland (Linux only)

Optional image format support (PNG, JPEG, etc.) can be omitted if intentionally configured.

### Optional

These are conditionally required based on build flags.

#### Spellchecking (`ENABLE_NUSPELL`)

- Nuspell
  - ICU (`uc`, `data` components only)

#### Media / Asset Playback (`ENABLE_ASSET_PLAYBACK`)

- FFmpeg (≥ 7.1.1)
  - Libraries:
    - avutil
    - avcodec
    - avformat
    - swresample
    - swscale

  - Required decoders:
    - AAC, Vorbis, Opus, Theora
    - H.264, VP9
    - pcm_s16le, pcm_s24le, pcm_s32le, pcm_f32le

  - Required demuxers:
    - OGG, MP4, WAV

  - Required parsers:
    - AAC, H.264, VP9

#### Git Integration (`ENABLE_LIBGIT2`)

- libgit2

#### Ruby Formatting (`ENABLE_RUBY_FORMATTING`)

- libclang (library + headers only, **not** the full clang compiler toolchain)

`rubyfmt`'s `prism-sys` build script uses `bindgen` against `libclang` to generate FFI bindings, so this flag needs the clang library and headers to be present even when the project itself is compiled entirely with GCC/MSVC. Install the distro's "clang library/headers" package (e.g. `libclang-common-*-dev` on Debian/Ubuntu, `clang-devel` on OpenSUSE, `clang21-libclang`/`clang21-headers` on Alpine) rather than the full `clang` compiler package where the distro distinguishes the two.

### Feature flags without extra libraries

These CMake options gate individual Rust cargo features and don't require any additional system libraries - toggle them freely to trim the build:

- Machine translation backends: `ENABLE_GOOGLE_TRANSLATE`, `ENABLE_YANDEX_TRANSLATE`, `ENABLE_DEEPL`, `ENABLE_LLM_CONNECTOR` (OpenAI, Anthropic, DeepSeek, Gemini, Mistral, Ollama, and other OpenAI-compatible/local providers).
- Export/Import serde formats: `ENABLE_SERDE_CSV`, `ENABLE_SERDE_XML`, `ENABLE_SERDE_XLSX`, `ENABLE_SERDE_YAML`.
- Glossary matching word segmentation/stemming backends: `ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_CHINESE_LINDERA`, `ENABLE_TOKENIZER_JAPANESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_LINDERA`, `ENABLE_TOKENIZER_KOREAN_LINDERA`, `ENABLE_TOKENIZER_SNOWBALL`, `ENABLE_TOKENIZER_SOUTHEAST_ASIAN`.

All of these default to `ON`, except the ICU-based tokenizers (`ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_ICU`), which default to `OFF` since the Lindera-based equivalents are enabled by default.

### Getting development headers on Linux

#### Debian

Project's baseline is Debian 13 (Trixie). Check Ubuntu 24.04 build below, it's very different.

`libqjs-dev` (quickjs-ng) and `libglaze-dev` only exist in `trixie-backports`, not plain trixie - enable it first:

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

Tumbleweed's repos only carry the original `quickjs` (`quickjs-devel`), not `quickjs-ng` . Build and install it from source instead:

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

Alpine has some musl problem with `ENABLE_RUBY_FORMATTING` where it's unable to link libclang requires by the underlying prism-sys' build script, so I guess you have to work around this if you want a working build. Or just set `ENABLE_RUBY_FORMATTING` to `OFF`.

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

#### Ubuntu (AppImage)

The AppImage build targets Ubuntu 24.04 specifically for the widest compatibility. Its dependency list is more involved than the other distros: Qt 6.8.2 doesn't ship in the 24.04 repos so it's installed via `aqtinstall`, and `quickjs-ng`/`glaze` aren't packaged either so they're built from source. Rather than duplicating all of that here, follow the `appimage-build` stage in the Dockerfile step by step - it's kept as the single source of truth for this target.

At a minimum, expect to need:

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

Qt itself, `quickjs-ng`, `glaze`, and the `linuxdeploy`/`linuxdeploy-plugin-qt` tooling still need to be installed/built separately - see the Dockerfile for the exact steps.

#### Dockerfile

You can find a Dockerfile in the root of repository that includes builds on aforementioned platforms plus the Ubuntu AppImage build, they're always guaranteed to succeed.

## Configuring

`configure.lua` wraps CMake for better UX:

```bash
./lua configure.lua --fresh -G=Ninja CMAKE_BUILD_TYPE=Release
```

Available CMake options can be fetched through ./lua configure.lua --help.

Plain CMake works just as well if you prefer it.

## Building

```bash
cmake --build build
```

Output lands in `build/target/bin/`.

## Per-platform notes

### Windows

For the details of how the libraries for the official static Windows build are compiled - check <https://github.com/savannstm/personal>, specifically - dev/build-dev-libraries scripts.

### Linux

The `Dockerfile` builds reproducibly on five distributions/targets, each as its own stage:

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target appimage-build .
docker build --target arch-build .
docker build --target alpine-build .
```

Each stage accepts a `TARGET_ARCH` build argument (default `x86-64-v3`) controlling the `-march`/`-Ctarget-cpu` value the C/C++/Rust code is compiled for:

```bash
docker build --target debian-build --build-arg TARGET_ARCH=x86-64-v2 .
```
