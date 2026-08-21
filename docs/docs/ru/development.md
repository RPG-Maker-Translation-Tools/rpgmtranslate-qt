# Разработка

## Требования

- Git
- Rust (>= 1.91)
    - cbindgen
- CMake (>= 3.25)
- C++23-compatible compiler (`clang`, `gcc`, `msvc`).

## Необходимые библиотеки

### Необходимые

- Qt6
- libarchive
    - liblzma
- quickjs-ng
- glaze

#### Qt6 (≥ 6.8.2)

Необходимые модули:

- Widgets
- Gui
- Network
- Concurrent
- Svg
- LinguistTools

Для платформ:

- Wayland (только Linux)

Опциональные форматы изображений (PNG, JPEG, и т.д.) могут быть выключены если не нужны.

### Опциональные

Эти библиотеки нужны исключительно при особых опциях.

#### Проверка правописания (`ENABLE_NUSPELL`)

- Nuspell
    - ICU (только компоненты `uc`, `data`)

#### Проигрывание ассетов (`ENABLE_ASSET_PLAYBACK`)

- FFmpeg (≥ 7.1.1)
    - Библиотеки:
        - avutil
        - avcodec
        - avformat
        - swresample
        - swscale

    - Нужные декодеры:
        - Vorbis, Opus, Theora
        - AAC, H.264, VP9

    - Нужные демуксеры:
        - OGG, MP4

    - Нужные парсеры:
        - AAC, H.264

#### Git интеграция (`ENABLE_LIBGIT2`)

- libgit2

### Получение файлов для разработки на Linux

#### Debian

Основа для проекта - Debian 13 (Trixie). Ubuntu имеет старые пакеты, на нём компиляция не гарантируется.

`libqjs-dev` (quickjs-ng) и `libglaze-dev` существуют только в `trixie-backports`, не в trixie - сначала включите их:

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

Вы можете найти Dockerfile в корне репозитория, который включает билды на вышеуказанных платформах, они всегда удадутся.

## Настройка

`configure.lua` - обёртка вокруг CMake для лучшего опыта:

```bash
./lua configure.lua --fresh -G=Ninja CMAKE_BUILD_TYPE=Release
```

Доступные CMake опции могут быть получены через `./lua configure.lua --help`.

Обычный CMake работает как надо, если вы хотите использовать его.

## Билдинг

```bash
cmake --build build
```

Артефакты помещаются в `build/target/bin/`.

## Заметки для платформ

### Windows

Насчёт деталей того, как библиотеки для официальный билдов скомпилированы - проверьте <https://github.com/savannstm/personal>, конкретнее - скрипты dev/build-dev-libraries.

### Linux

`Dockerfile` создаёт воспроизводимые билды на четырёх дистрибутивах, каждый имеет свою стадию:

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target arch-build .
docker build --target alpine-build .
```
