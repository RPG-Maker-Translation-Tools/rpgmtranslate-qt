---
title: "Development"
---

## Prerequisites

- Git
- Rust (≥ 1.95)
  - cbindgen
- CMake (≥ 3.25)
- C++23-compatible compiler (`≥clang 18`, `≥gcc 14`, `≥MSVC 2022`)
- C++23-complete STL (`≥libstdc++14`, `≥libc++23`, `≥MSVC 2022`).

## Required libraries

### Required

- Qt6 (≥ 6.8)
- libarchive
  - liblzma
- quickjs-ng
- glaze (≥ 7.5.0)

#### Qt6 (≥ 6.8)

Required modules:

- Widgets
- Gui
- Network
- Concurrent
- Svg
- LinguistTools

Platform-specific:

- WaylandClient (Linux only)

Optional image format support (PNG, JPEG, etc.) can be omitted if intentionally configured.

On Linux, OpenSSL is required as well.

### Optional

These are conditionally required based on build flags.

#### Spellchecking (`ENABLE_NUSPELL`)

- Nuspell
  - ICU (`uc`, `data` components only)

#### Media / Asset Playback (`ENABLE_ASSET_PLAYBACK`)

- FFmpeg (≥ 6.1, i.e. libavutil ≥ 58.29)
  - Libraries:
    - avutil
    - avcodec
    - avformat
    - avfilter
    - swresample
    - swscale

  The application plays whatever formats the FFmpeg it is linked against can decode, so there is no fixed list of required decoders. Check <https://github.com/savannstm/personal> (`dev/build-dev-libraries`) for the enabled decoders, demuxers and parsers.

  MIDI playback does not use FFmpeg - it is done by TinySoundFont, which is bundled in the sources.

#### Git Integration (`ENABLE_LIBGIT2`)

- libgit2

#### Self-updater (`ENABLE_UPDATER`)

Checks GitHub for a newer release and installs it over the running executable (or the AppImage). On by default; needs nothing beyond Qt Network. Turn it off for package-manager builds: _Check for updates_ then only says the build can't update itself. libarchive stays required, since backups use it.

#### Ruby Formatting (`ENABLE_RUBY_FORMATTING`)

- libclang (library + headers only, **not** the full clang compiler toolchain)

`rubyfmt`'s `prism-sys` build script runs `bindgen` against `libclang`, so this flag needs the clang library and headers even when the project is built with GCC/MSVC. Install the distro's clang library/headers package (e.g. `libclang-common-*-dev`, `clang-devel`, `clang21-libclang`/`clang21-headers`), not the full `clang` compiler where the two are split.

#### mimalloc (`ENABLE_MIMALLOC`)

- mimalloc

Uses mimalloc instead of the standard memory allocator. Off by default.

### Feature flags without extra libraries

These CMake options gate individual Rust cargo features and don't require any additional system libraries - toggle them freely to trim the build:

- LanguageTool linting: `ENABLE_LANGUAGETOOL`.
- Machine translation backends: `ENABLE_GOOGLE_TRANSLATE`, `ENABLE_YANDEX_TRANSLATE`, `ENABLE_DEEPL`.
- Asset inspector code viewer: `ENABLE_JSON_HIGHLIGHTING`, `ENABLE_JS_HIGHLIGHTING`, `ENABLE_RUBY_HIGHLIGHTING` (syntax highlighting) and `ENABLE_JS_FORMATTING` (the Beautify button for JavaScript; `ENABLE_RUBY_FORMATTING` is described above).
- Export/Import serde formats: `ENABLE_SERDE_CSV`, `ENABLE_SERDE_XML`, `ENABLE_SERDE_XLSX`, `ENABLE_SERDE_YAML`.
- Glossary matching word segmentation/stemming backends: `ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_CHINESE_LINDERA`, `ENABLE_TOKENIZER_JAPANESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_LINDERA`, `ENABLE_TOKENIZER_KOREAN_LINDERA`, `ENABLE_TOKENIZER_SNOWBALL`, `ENABLE_TOKENIZER_SOUTHEAST_ASIAN`.
- Word counting (the words/characters counts in the status bar and project statistics): `ENABLE_COUNTING_DICTIONARY` (dictionary segmentation for Chinese, Japanese, Thai, Lao, Khmer and Burmese; without it a run of such text counts as one word) and `ENABLE_COUNTING_LSTM` (LSTM models for Thai, Lao, Khmer and Burmese). Both default to `ON`. Counting uses the `text-stats` crate, which has to be checked out next to this repository (`../text-stats`) until it is published.

All of these default to `ON`, except the ICU-based tokenizers (`ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_ICU`), which default to `OFF` since the Lindera-based equivalents are enabled by default.

### Other options

- `ENABLE_PIC` (`ON`) - position-independent code.
- `MSVC_RUNTIME` (`MT`) - the MSVC runtime library: `MT`, `MD`, `MTd` or `MDd`.
- `RUST_PROFILE` - the Cargo profile the Rust half is built with.
- `LUA_EXECUTABLE` - the Lua used by the pre-build scripts. A Lua binary for Linux and Windows is shipped in the repository; set this to use your own.

### Getting development headers on Linux

#### Debian

The baseline is Debian 13 (Trixie). The Ubuntu 24.04 build below differs a lot.

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
  libavutil-dev libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev libswscale-dev \
  libqjs-dev libglaze-dev \
  pkg-config libssl-dev libkrb5-dev
```

#### OpenSUSE

Tumbleweed only ships the original `quickjs` (`quickjs-devel`), not `quickjs-ng`. Build it from source:

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
  libgit2-devel nuspell nuspell-devel libicu-devel \
  ffmpeg-devel libavcodec-devel libavformat-devel libavutil-devel libavfilter-devel libswscale-devel libswresample-devel \
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

On Alpine, `ENABLE_RUBY_FORMATTING` can't link the libclang that prism-sys's build script needs (a musl problem), so a working build needs a workaround. Or set `ENABLE_RUBY_FORMATTING` to `OFF`.

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

The AppImage targets Ubuntu 24.04 for the widest compatibility. Qt 6.8 is not in its repos (installed via `aqtinstall`), and `quickjs-ng`/`glaze` are built from source. The `appimage-build` stage of the Dockerfile is the single source of truth. At minimum you need:

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

The Dockerfile in the repository root builds on all the platforms above plus the Ubuntu AppImage, and those builds are guaranteed to succeed.

## Configuring

`configure.lua` wraps CMake for better UX:

```bash
./lua configure.lua --fresh -G=Ninja CMAKE_BUILD_TYPE=Release
```

Available CMake options can be fetched through `./lua configure.lua --help`.

Plain CMake works just as well if you prefer it.

## Building

```bash
cmake --build build
```

Output lands in `build/target/bin/`.

## Updating translations

The translation catalogs are `translations/*.ts`. The simplest way to update them is the CMake target, which passes everything below by itself:

```bash
cmake --build build --target update_translations
```

To run `lupdate` by hand, it has to be given the aliases that let it read the `NOTICE` macros, which it does not expand:

```
-tr-function-alias QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N
```

```bash
lupdate src \
  -tr-function-alias "QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N" \
  -ts translations/rpgmtranslate.en.ts translations/rpgmtranslate.ru.ts
```

Call `QCoreApplication::translate("Context", "text")` with literal arguments, never `qApp->translate(...)`: it does not parse `->translate` and silently drops the strings. A `translate` call with a variable (`translate("Lint", message)`) is not extracted either; mark such strings with `QT_TRANSLATE_NOOP("Lint", "text")` where they are written.

## Per-platform notes

### Windows

How the official static Windows libraries are built: see the `dev/build-dev-libraries` scripts in <https://github.com/savannstm/personal>.

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
