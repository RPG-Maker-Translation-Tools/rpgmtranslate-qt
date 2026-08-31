# Разработка

## Требования

- Git
- Rust (>= 1.91)
  - cbindgen
- CMake (>= 3.25)
- C++23-совместимый компилятор (`clang 18+`, `gcc 14+`, `MSVC 2022+`)
- C++23-готовый STL (`>libstdc++14`, `>libc++23`, `>MSVC 2022`).

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
    - AAC, Vorbis, Opus, Theora
    - H.264, VP9
    - pcm_s16le, pcm_s24le, pcm_s32le, pcm_f32le

  - Нужные демуксеры:
    - OGG, MOV, WAV

  - Нужные парсеры:
    - AAC, H.264, VP9

#### Git интеграция (`ENABLE_LIBGIT2`)

- libgit2

#### Форматирование Ruby (`ENABLE_RUBY_FORMATTING`)

- libclang (библиотека + хедеры, **не** полный тулчейн компилятора)

Билд скрипт `prism-sys` для `rubyfmt` использует `bindgen` с `libclang` для генерации FFI биндов, так что этот флаг нуждается в библиотеке clang даже при компиляции с GCC. Установите пакеты с библиотекой и хедерами clang (например `libclang-common-*-dev` на Debian/Ubuntu, `clang-devel` на OpenSUSE, `clang21-libclang`/`clang21-headers` на Alpine) вместо полного пакета компилятора `clang`.

### Опции не нуждающиеся в доп. библиотеках

Эти CMake опции переключают отдельные опции библиотек Rust и не нуждаются в дополнительных системных библиотеках - переключайте их чтоыб уменьшить билд:

- Точки машинного перевода: `ENABLE_GOOGLE_TRANSLATE`, `ENABLE_YANDEX_TRANSLATE`, `ENABLE_DEEPL`, `ENABLE_LLM_CONNECTOR` (OpenAI, Anthropic, DeepSeek, Gemini, Mistral, Ollama, и другие OpenAI-совместимые/локальные провайдеры).
- Экспорт/импорт форматов: `ENABLE_SERDE_CSV`, `ENABLE_SERDE_XML`, `ENABLE_SERDE_XLSX`, `ENABLE_SERDE_YAML`.
- Бэкэнды сегментации и стемминга для глоссария: `ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_CHINESE_LINDERA`, `ENABLE_TOKENIZER_JAPANESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_LINDERA`, `ENABLE_TOKENIZER_KOREAN_LINDERA`, `ENABLE_TOKENIZER_SNOWBALL`, `ENABLE_TOKENIZER_SOUTHEAST_ASIAN`.

Все они установлены на `ON` по умолчанию, кроме токенизаторов основнных на ICU (`ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_ICU`), которые по умолчанию установлены на `OFF` поскольку вместо них включены эквиваленты основанные на Lindera.

### Получение файлов для разработки на Linux

#### Debian

Основа для проекта - Debian 13 (Trixie). Ubuntu 24.04 можно посмотреть снизу, там всё по-другому.

`libqjs-dev` (quickjs-ng) и `libglaze-dev` существуют только в `trixie-backports`, не в trixie - сначала включите их:

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

Репозитории Tumbleweed имеют только оригинальный `quickjs` (`quickjs-devel`), не `quickjs-ng`. Забилдите и установите из исходников:

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

У Alpine есть какая-то проблема с `ENABLE_RUBY_FORMATTING` где не получается линкануть библиотеку libclang необходимую билд скриптом библиотеки `prism-sys`, так что обходите это сами если хотите рабочий билд. Или просто установите `ENABLE_RUBY_FORMATTING` на `OFF`.

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

AppImage билд нацелен на Ubuntu 24.04 для максимальной совместимости. Список зависимостей куда более сложны чем для других дистрибутивов - Qt 6.8.2 нет в репозиториях 24.04, так что нужно установить его через `aqtinstall`, а `quickjs-ng`/`glaze` вообще не запакованы и надо билдить с исходников. Вместо того чтобы дублировать всё сюда, прочтите Dockerfile и следуйте этапам оттуда.

Как минимум, вам нужны:

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

Сам Qt, `quickjs-ng`, `glaze`, и `linuxdeploy`/`linuxdeploy-plugin-qt` нуждаются в отдельной установке - бегом в Dockerfile за шагами.

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

`Dockerfile` создаёт воспроизводимые билды на пяти дистрибутивах, каждый имеет свою стадию:

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target arch-build .
docker build --target alpine-build .
```

Каждый билд принимает аргумент `TARGET_ARCH` (по умолчанию `x86-64-v3`), который контролирует переменные `-march`/`-Ctarget-cpu` для кода C/C++/Rust:

```bash
docker build --target debian-build --build-arg TARGET_ARCH=x86-64-v2 .
```
