# Development

## Prerequisites

- Git
- Rust (>= 1.91)
    - cbindgen
- CMake (>= 3.25)
- C++23-compatible compiler (`clang`, `gcc`, `msvc`).

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
        - Vorbis, Opus, Theora
        - AAC, H.264, VP9

    - Required demuxers:
        - OGG, MP4

    - Required parsers:
        - AAC, H.264

#### Git Integration (`ENABLE_LIBGIT2`)

- libgit2

### Getting development headers on Linux

#### Debian

Project's baseline is Debian 13 (Trixie). Ubuntu has slightly older packages, so compilation is not guaranteed to succeed.

`libqjs-dev` (quickjs-ng) and `libglaze-dev` only exist in `trixie-backports`, not plain trixie - enable it first:

```bash
echo 'deb http://deb.debian.org/debian trixie-backports main' | sudo tee /etc/apt/sources.list.d/backports.list
```

```bash
sudo apt update && sudo apt install -y \
  git cmake g++ clang \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-wayland-dev \
  qt6-l10n-tools qt6-svg-dev \
  libarchive-dev liblzma-dev \
  libgit2-dev libnuspell-dev libicu-dev \
  libqjs-dev libglaze-dev \
  pkg-config libssl-dev
```

#### OpenSUSE

```bash
sudo zypper refresh && sudo zypper install -y \
  git cmake gcc-c++ clang \
  qt6-base-devel qt6-linguist-devel qt6-tools-devel qt6-svg-devel qt6-wayland-devel \
  libarchive-devel xz-devel \
  libgit2-devel nuspell libicu-devel \
  glaze-devel quickjs-ng-devel \
  pkg-config libopenssl-devel
```

#### Arch

```bash
sudo pacman -Syu --needed \
  git cmake gcc clang glibc pkgconf \
  qt6-base qt6-tools qt6-svg qt6-wayland \
  openssl libarchive xz libgit2 nuspell icu ffmpeg \
  quickjs-ng glaze
```

#### Alpine

```bash
sudo apk update && sudo apk add \
  git cmake g++ clang \
  qt6-qtbase-dev qt6-qttools-dev qt6-qtsvg-dev qt6-qtwayland-dev \
  libarchive-dev xz-dev \
  libgit2-dev nuspell-dev icu-dev \
  quickjs-ng-dev glaze \
  pkgconf openssl-dev \
  libunwind-dev
```

#### Dockerfile

You can find a Dockerfile in the root of repository that includes builds on aforementioned platforms, they're always guaranteed to succeed.

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

The `Dockerfile` builds reproducibly on four distributions, each as its own stage:

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target arch-build .
docker build --target alpine-build .
```
