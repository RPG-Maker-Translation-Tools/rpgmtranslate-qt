# Development

## Prerequisites

- Git
- Rust (>= 1.88)
    - cbindgen
- CMake (>= 3.25)
- C++23-compatible compiler (`clang`, `gcc`, `msvc`).

## Required libraries

### Required

- Qt6
- libarchive (LZMA only)

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
    - ICU (components: `uc`, `data` only)

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

```bash
sudo apt update && sudo apt install -y \
  git cmake g++ clang \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-wayland-dev \
  qt6-l10n-tools qt6-svg-dev \
  libarchive-dev \
  libgit2-dev libnuspell-dev \
  pkg-config libssl-dev
```

#### OpenSUSE

```bash
sudo zypper refresh && sudo zypper install -y \
  git cmake gcc-c++ clang \
  qt6-base-devel qt6-linguist-devel qt6-tools-devel qt6-svg-devel qt6-wayland-devel \
  libarchive-devel \
  libgit2-devel nuspell \
  pkg-config libopenssl-devel
```

#### Arch

```bash
sudo pacman -Syu --needed \
  git cmake gcc clang glibc pkgconf \
  qt6-base qt6-tools qt6-svg qt6-wayland \
  openssl libarchive libgit2 nuspell ffmpeg
```

#### Alpine

```bash
sudo apk update && sudo apk add \
  git cmake g++ clang \
  qt6-qtbase-dev qt6-qttools-dev qt6-qtsvg-dev qt6-qtwayland-dev \
  libarchive-dev \
  libgit2-dev nuspell-dev \
  pkgconf openssl-dev \
  libunwind-dev
```

#### Dockerfile

You can find a Dockerfile in the root of repository that includes builds on aforementioned platforms, they're always guaranteed to succeed.

## Building Process

Clone the repository: `git clone https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt`.

We provide configure scripts - `configure.ps1` for PowerShell and `configure.sh` for sh/bash. You can get the list of available options using `./configure --help` (including custom project CMake variables, etc.).

After running configure with the desired arguments, you can `cd` to the build directory and run `cmake --build . -j`, that will build the application. Build artifacts are output to `build/target` directory.

### Default Windows builds

Default Windows builds are always static, with static runtime. They're compiled using MSVC and LLVM toolchain (clang-cl, lld-link and llvm-lib).

Qt6 is built with the following configuration: `./configure -no-guess-compiler -cmake-use-default-generator -c++std c++23 -release -static -static-runtime -ltcg -reduce-relocations -nomake tests -nomake examples -nomake benchmarks -no-opengl -no-emojisegmenter -no-appstore-compliant -no-sbom -no-sbom-json -no-sbom-verify -no-icu -no-gif -no-dbus -no-schannel -no-system-proxies -no-mimetype-database -qt-harfbuzz -qt-freetype -qt-libpng -qt-libjpeg -qt-webp -qt-tiff -qt-zlib -qt-doubleconversion -qt-pcre -gui -widgets -submodules qtbase,qtimageformats,qttools,qtsvg -qpa "windows"`

ICU is built with the following configuration, on MSYS2: `CC=clang-cl CXX=clang-cl LD=lld-link AR=llvm-lib NM=llvm-nm RANLIB=llvm-ranlib CFLAGS="-std:c17 -O2 -Ob3 -GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" CXXFLAGS="-std:c++latest -O2 -Ob3 -GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" LDFLAGS="-LTCG" CPPFLAGS="-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 -DUCONFIG_NO_TRANSLITERATION=1" ICU_DATA_FILTER_FILE=/c/dev/repos/icu/icu4c/source/filters.json ./runConfigureICU MSYS/MSVC --prefix=/c/dev/release-static --enable-static --disable-shared --disable-tests --disable-samples --enable-tools --disable-extras --disable-icuio --disable-layout --disable-layoutex --disable-dyload --with-data-packaging=static`
with the following filters.json file:

```json
{
  "featureFilters": {
    "curr": { "filterType": "exclude" },
    "lang": { "filterType": "exclude" },
    "rbnf": { "filterType": "exclude" },
    "region": { "filterType": "exclude" },
    "translit": { "filterType": "exclude" },
    "tz": { "filterType": "exclude" },
    "unit": { "filterType": "exclude" },
    "zone": { "filterType": "exclude" },
    "coll_tree": { "filterType": "exclude" }
  }
}
```

Nuspell is built with the following configuration: `cmake . -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_TOOLS=OFF -DBUILD_DOCS=OFF`

Libgit2 is built with the following configuration: `cmake . -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_CLI=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF -DUSE_THREADS=ON -DUSE_NSEC=ON -DUSE_SSH=OFF -DUSE_HTTPS=ON -DSTATIC_CRT=ON`

Libarchive is built with the following configuration: `cmake . -DBUILD_SHARED_LIBS=OFF -DENABLE_OPENSSL=OFF -DENABLE_MBEDTLS=OFF -DENABLE_NETTLE=OFF -DENABLE_LIBB2=OFF -DENABLE_LZ4=OFF -DENABLE_LZO=OFF -DENABLE_ZSTD=OFF -DENABLE_ZLIB=OFF -DENABLE_BZip2=OFF -DENABLE_LIBXML2=OFF -DENABLE_EXPAT=OFF -DENABLE_PCREPOSIX=OFF -DENABLE_ICONV=OFF -DENABLE_ACL=OFF -DENABLE_XATTR=OFF -DENABLE_LZMA=ON -DENABLE_TAR=OFF -DENABLE_CPIO=OFF -DENABLE_CAT=OFF -DENABLE_UNZIP=OFF -DENABLE_TEST=OFF -DENABLE_COVERAGE=OFF -DBUILD_TESTING=OFF`

FFmpeg is built with the following configuration: `./configure --cpu=sandybridge --target-os=win64 --arch=x86_64 --toolchain=msvc --ar=llvm-ar --strip=llvm-strip --cc=clang-cl --stdc=c17 --cxx=clang-cl --stdcxx=c++23 --ld=clang-cl --ranlib=llvm-ranlib --enable-lto --enable-pic --extra-cflags="-O2 -Ob3 -GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" --extra-cxxflags="-O2 -Ob3 -GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" --extra-ldflags="-LTCG" --disable-autodetect --disable-debug --disable-all --disable-programs --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages  --enable-asm --enable-optimizations --enable-stripping  --enable-static --enable-avformat --enable-avfilter --enable-avcodec --enable-swresample --enable-swscale --enable-demuxer=ogg,mp4 --enable-decoder=vorbis,theora,aac,opus,h264,vp9 --enable-parser=aac,h264 --enable-protocol=file --enable-zlib --disable-iconv`

All builds use the following CMake toolchain:

```cmake
set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER       lld-link)
set(CMAKE_AR           llvm-lib)

set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "" FORCE)

set(CMAKE_C_FLAGS_RELEASE   "/std:c17 /O2 /Ob3 /GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "/std:c++latest /O2 /Ob3 /GL -fmerge-all-constants -march=sandybridge -fms-runtime-lib=static -fuse-ld=lld-link" CACHE STRING "" FORCE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "/LTCG" CACHE STRING "" FORCE)

set(CMAKE_PREFIX_PATH "C:/dev/release-static" CACHE STRING "" FORCE)
set(CMAKE_INSTALL_PREFIX "C:/dev/release-static" CACHE PATH "" FORCE)

set(ICU_ROOT "C:/dev/release-static")
set(ICU_UC_LIBRARY     "C:/dev/release-static/lib/sicuuc.lib")
set(ICU_DATA_LIBRARY   "C:/dev/release-static/lib/sicudt.lib")
set(ICU_INCLUDE_DIR    "C:/dev/release-static/include")
```
