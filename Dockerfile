# syntax=docker/dockerfile:1

# Build info:
# We set locale to UTF-8 to avoid any problems related to the locale.
# We build with Ninja - it's faster, and every stage now uses it consistently.
# We install curl to get rustup and cargo-binstall.
# We use pkgconf instead of pkg-config where the distro offers a choice - lighter and faster.
# We install libssl-dev because some Rust dependencies want it.
# libunwind-dev is required on Alpine because Rust wants it.
#
# Debian's libgit2-dev also ships a broken libgit2Targets.cmake whose
# INTERFACE_INCLUDE_DIRECTORIES points at .../include/git2 instead of its parent - that
# shadows real system headers with git2's bundled MSVC-only compat shims (e.g. stdint.h,
# a silent no-op under GCC/Clang) for anything included via <stdint.h> etc. afterward.
# cmake/FindLibgit2.cmake strips the stray /git2 suffix before wrapping the target.
#
# Every package manager and the cargo registry download into a BuildKit cache mount, so a rebuild
# after a source change does not refetch hundreds of MiB of packages and the whole crates.io graph.
# The caches live on the build host, not in the image, so a clean machine still gets a clean build.

# Debian
FROM debian:trixie AS debian-build

ARG DEBIAN_FRONTEND=noninteractive

# the image ships a hook that deletes downloaded archives, which would empty the cache mount
RUN rm -f /etc/apt/apt.conf.d/docker-clean \
    && echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > /etc/apt/apt.conf.d/keep-cache

# libqjs-dev (quickjs-ng) and libglaze-dev only exist in trixie-backports
RUN echo 'deb http://deb.debian.org/debian trixie-backports main' > /etc/apt/sources.list.d/backports.list

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt update && apt install -y \
    git make cmake ninja-build g++ libclang1-19 libclang-common-19-dev pkgconf ca-certificates locales curl \
    libssl-dev libkrb5-dev \
    qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-wayland-dev \
    qt6-l10n-tools qt6-svg-dev \
    libarchive-dev libgit2-dev libnuspell-dev \
    libavutil-dev libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev libswscale-dev \
    libqjs-dev libglaze-dev liblzma-dev libicu-dev \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.91
ENV PATH="/root/.cargo/bin:${PATH}"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x lua
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    ./lua configure.lua -B=build -G=Ninja CMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

# OpenSUSE
FROM opensuse/tumbleweed AS opensuse-build

ARG REPO_URL

RUN --mount=type=cache,target=/var/cache/zypp,sharing=locked \
    zypper --non-interactive modifyrepo --keep-packages --all \
    && zypper refresh && zypper install -y \
    git make cmake ninja gcc-c++ clang-devel curl glibc-locale pkgconf \
    libopenssl-devel libssh2-devel \
    qt6-base-devel qt6-linguist-devel qt6-tools-devel qt6-svg-devel qt6-wayland-devel \
    libarchive-devel libgit2-devel nuspell nuspell-devel ffmpeg ffmpeg-devel libavcodec-devel libavformat-devel libavutil-devel libswscale-devel libswresample-devel libavfilter-devel \
    glaze-devel xz-devel libicu-devel

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Tumbleweed's repos only carry the original quickjs (quickjs-devel)
RUN git clone --depth 1 --branch v0.16.2 https://github.com/quickjs-ng/quickjs.git /tmp/quickjs-ng \
    && cmake -G=Ninja -B /tmp/quickjs-ng/build -S /tmp/quickjs-ng -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/quickjs-ng/build -j \
    && cmake --install /tmp/quickjs-ng/build \
    && rm -rf /tmp/quickjs-ng

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.91
ENV PATH="/root/.cargo/bin:${PATH}"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x lua
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    ./lua configure.lua -B=build -G=Ninja CMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

FROM ubuntu:24.04 AS appimage-build

ARG DEBIAN_FRONTEND=noninteractive

RUN rm -f /etc/apt/apt.conf.d/docker-clean \
    && echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > /etc/apt/apt.conf.d/keep-cache

# universe repo needed for some packages (nuspell, etc.)
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y software-properties-common \
    && add-apt-repository universe && apt-get update \
    && apt-get install -y \
    git make cmake ninja-build libc6-dev dpkg-dev gcc-14 g++-14 libclang1-18 libclang-common-18-dev pkgconf ca-certificates locales curl wget \
    python3-pip \
    # same app deps as your Debian stage
    libssl-dev libkrb5-dev \
    libarchive-dev libgit2-dev libnuspell-dev \
    libavutil-dev libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev libswscale-dev \
    liblzma-dev libicu-dev \
    # Qt platform/render deps (Qt itself comes from aqtinstall below)
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
    # AppImage bundling tools
    libfuse2 file patchelf \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen

# GCC 14 for C++23
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Neither quickjs-ng nor glaze is packaged for 24.04 - build and install both from source
RUN git clone --depth 1 --branch v0.16.2 https://github.com/quickjs-ng/quickjs.git /tmp/quickjs-ng \
    && cmake -G=Ninja -B /tmp/quickjs-ng/build -S /tmp/quickjs-ng -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/quickjs-ng/build -j \
    && cmake --install /tmp/quickjs-ng/build \
    && rm -rf /tmp/quickjs-ng \
    && git clone --depth 1 --branch v8.0.0 https://github.com/stephenberry/glaze.git /tmp/glaze \
    && cmake -G=Ninja -B /tmp/glaze/build -S /tmp/glaze -DCMAKE_BUILD_TYPE=Release -Dglaze_BUILD_EXAMPLES=OFF \
    && cmake --install /tmp/glaze/build \
    && rm -rf /tmp/glaze

# Qt 6.8.2 - apt on 24.04 only ships 6.4, so we use aqtinstall
RUN --mount=type=cache,target=/root/.cache/pip,sharing=locked \
    pip3 install --break-system-packages aqtinstall

RUN aqt install-qt linux desktop 6.8.2 linux_gcc_64 \
    --outputdir /opt/Qt \
    --modules qtwaylandcompositor

ENV QT_ROOT=/opt/Qt/6.8.2/gcc_64
ENV PATH="$QT_ROOT/bin:${PATH}"
ENV LD_LIBRARY_PATH="$QT_ROOT/lib:${LD_LIBRARY_PATH:-}"
# CMAKE_PREFIX_PATH lets find_package(Qt6) work without any extra cmake flags
ENV CMAKE_PREFIX_PATH="$QT_ROOT" \
    Qt6_DIR="$QT_ROOT/lib/cmake/Qt6"

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.91
ENV PATH="/root/.cargo/bin:${PATH}"

RUN curl -L --proto '=https' --tlsv1.2 -sSf \
    https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    cargo binstall -y cbindgen

# linuxdeploy + Qt plugin
# FUSE doesn't work in Docker, so we extract both AppImages in place and symlink AppRun
RUN wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    -O /usr/local/bin/linuxdeploy \
    && wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    -O /usr/local/bin/linuxdeploy-plugin-qt \
    && chmod +x /usr/local/bin/linuxdeploy /usr/local/bin/linuxdeploy-plugin-qt \
    && cd /usr/local/bin \
    && APPIMAGE_EXTRACT_AND_RUN=1 linuxdeploy --appimage-extract \
    && mv squashfs-root linuxdeploy.appdir && rm linuxdeploy \
    && ln -s linuxdeploy.appdir/AppRun linuxdeploy \
    && APPIMAGE_EXTRACT_AND_RUN=1 linuxdeploy-plugin-qt --appimage-extract \
    && mv squashfs-root linuxdeploy-plugin-qt.appdir && rm linuxdeploy-plugin-qt \
    && ln -s linuxdeploy-plugin-qt.appdir/AppRun linuxdeploy-plugin-qt

COPY . /app
WORKDIR /app

RUN chmod +x lua
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    CMAKE_PREFIX_PATH=/opt/Qt/6.8.2/gcc_64 \
    Qt6_DIR=/opt/Qt/6.8.2/gcc_64/lib/cmake/Qt6 \
    ./lua configure.lua -G=Ninja CMAKE_BUILD_TYPE=Release ENABLE_ASSET_PLAYBACK=OFF \
    && cmake --build build -j

ARG APP_NAME=rpgmtranslate
ARG APP_BINARY=build/target/bin/rpgmtranslate

RUN QMAKE="$QT_ROOT/bin/qmake" \
    APPIMAGE_EXTRACT_AND_RUN=1 \
    linuxdeploy \
    --appdir AppDir \
    --executable ${APP_BINARY} \
    --desktop-file ${APP_NAME}.desktop \
    --icon-file icons/${APP_NAME}-logo.png \
    --plugin qt \
    --output appimage

# Arch
FROM archlinux:latest AS arch-build

RUN sed -i 's/^#\?ParallelDownloads.*/ParallelDownloads = 10/' /etc/pacman.conf

RUN --mount=type=cache,target=/var/cache/pacman/pkg,sharing=locked \
    pacman -Syu --noconfirm --needed \
    git make cmake ninja gcc clang pkgconf openssl glibc curl \
    qt6-base qt6-tools qt6-svg qt6-wayland \
    libarchive libgit2 nuspell ffmpeg \
    quickjs-ng glaze icu xz \
    && sed -i '/en_US.UTF-8/s/^#//g' /etc/locale.gen && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.91
ENV PATH="/root/.cargo/bin:${PATH}"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x lua
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    ./lua configure.lua -B=build -G=Ninja CMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

# Alpine
FROM alpine:latest AS alpine-build

RUN --mount=type=cache,target=/var/cache/apk,sharing=locked \
    apk update && apk add --cache-dir /var/cache/apk \
    git make cmake samurai g++ clang21-libclang clang21-headers bash musl-locales musl-locales-lang curl pkgconf \
    libunwind-dev openssl-dev \
    qt6-qtbase-dev qt6-qttools-dev qt6-qtsvg-dev qt6-qtwayland-dev \
    libarchive-dev libgit2-dev nuspell-dev ffmpeg-dev \
    quickjs-ng-dev glaze xz-dev icu-dev

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.91
ENV PATH="/root/.cargo/bin:${PATH}"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x lua
RUN --mount=type=cache,target=/root/.cargo/registry,sharing=locked \
    --mount=type=cache,target=/root/.cargo/git,sharing=locked \
    ./lua configure.lua -B=build -G=Ninja CMAKE_BUILD_TYPE=Release \
    && cmake --build build -j
