# Build info:
# We set locale to UTF-8 to avoid any problems related to the locale.
# We install curl to get rustup and cargo-binstall.
# We install fixed, 1.88 Rust toolchain.
# We install `make` for `Unix Makefiles` CMake generator - that's bare minimum. I'm not sure whether to switch to Ninja.
# We install pkg-config because it's needed for builds on Linux in general.
# We install libssl-dev because some Rust dependencies want it.
# libunwind-dev is required on Alpine because Rust wants it.

# Debian
FROM debian:trixie AS debian-build

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    git cmake make g++ clang pkg-config ca-certificates locales curl \
    libssl-dev libkrb5-dev \
    qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-wayland-dev \
    qt6-l10n-tools qt6-svg-dev \
    libarchive-dev libgit2-dev libnuspell-dev \
    libavutil-dev libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev libswscale-dev \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.88
ENV PATH="/root/.cargo/bin:${PATH}" \
    RUSTFLAGS="-C target-feature=+aes,+sse2"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x configure
RUN ./configure
RUN cmake -B build -S . && cmake --build build -j

# OpenSUSE
FROM opensuse/tumbleweed AS opensuse-build

ARG REPO_URL

RUN zypper refresh && zypper install -y \
    git cmake make gcc-c++ clang curl glibc-locale pkg-config \
    libopenssl-devel libssh2-devel \
    qt6-base-devel qt6-linguist-devel qt6-tools-devel qt6-svg-devel qt6-wayland-devel \
    libarchive-devel libgit2-devel nuspell nuspell-devel ffmpeg ffmpeg-devel libavcodec-devel libavformat-devel libavutil-devel libswscale-devel libswresample-devel libavfilter-devel

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.88
ENV PATH="/root/.cargo/bin:${PATH}" \
    RUSTFLAGS="-C target-feature=+aes,+sse2"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x configure
RUN ./configure
RUN cmake -B build -S . && cmake --build build -j

# Arch
FROM archlinux:latest AS arch-build

RUN pacman -Syu --noconfirm --needed \
    git cmake make gcc clang pkgconf openssl glibc curl \
    qt6-base qt6-tools qt6-svg qt6-wayland \
    libarchive libgit2 nuspell ffmpeg \
    && sed -i '/en_US.UTF-8/s/^#//g' /etc/locale.gen && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.88
ENV PATH="/root/.cargo/bin:${PATH}" \
    RUSTFLAGS="-C target-feature=+aes,+sse2"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x configure
RUN ./configure
RUN cmake -B build -S . && cmake --build build -j

# Alpine
FROM alpine:latest AS alpine-build

RUN apk update && apk add \
    git cmake make g++ clang bash musl-locales musl-locales-lang curl pkgconf \
    libunwind-dev openssl-dev \
    qt6-qtbase-dev qt6-qttools-dev qt6-qtsvg-dev qt6-qtwayland-dev \
    libarchive-dev libgit2-dev nuspell-dev ffmpeg-dev

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.88
ENV PATH="/root/.cargo/bin:${PATH}" \
    RUSTFLAGS="-C target-feature=+aes,+sse2"

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cbindgen

COPY . /app
WORKDIR /app

RUN chmod +x configure
RUN ./configure
RUN cmake -B build -S . && cmake --build build -j