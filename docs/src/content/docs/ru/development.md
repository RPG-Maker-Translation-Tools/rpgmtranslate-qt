---
title: "Разработка"
---

## Требования

- Git
- Rust (≥ 1.95)
  - cbindgen
- CMake (≥ 3.25)
- C++23-совместимый компилятор (`≥clang 18`, `≥gcc 14`, `≥MSVC 2022`)
- STL с полной поддержкой C++23 (`≥libstdc++14`, `≥libc++23`, `≥MSVC 2022`).

## Необходимые библиотеки

### Обязательные

- Qt6 (≥ 6.8)
- libarchive
  - liblzma
- quickjs-ng
- glaze (≥ 7.5.0)

#### Qt6 (≥ 6.8)

Необходимые модули:

- Widgets
- Gui
- Network
- Concurrent
- Svg
- LinguistTools

Для платформ:

- WaylandClient (только Linux)

Поддержку дополнительных форматов изображений (PNG, JPEG и т.д.) можно не включать, если это сделано намеренно.

На Linux также требуется OpenSSL.

### Опциональные

Эти библиотеки нужны в зависимости от флагов сборки.

#### Проверка правописания (`ENABLE_NUSPELL`)

- Nuspell
  - ICU (только компоненты `uc`, `data`)

#### Проигрывание медиа/ассетов (`ENABLE_ASSET_PLAYBACK`)

- FFmpeg (≥ 6.1, т.е. libavutil ≥ 58.29)
  - Библиотеки:
    - avutil
    - avcodec
    - avformat
    - avfilter
    - swresample
    - swscale

  Приложение проигрывает любые форматы, которые может декодировать FFmpeg, с которым оно слинковано, так что фиксированного списка необходимых декодеров нет. Включённые декодеры, демуксеры и парсеры смотрите в <https://github.com/savannstm/personal> (`dev/build-dev-libraries`).

  Проигрывание MIDI не использует FFmpeg - этим занимается TinySoundFont, который поставляется в исходниках.

#### Git интеграция (`ENABLE_LIBGIT2`)

- libgit2

#### Самообновление (`ENABLE_UPDATER`)

Проверяет GitHub на наличие нового релиза и устанавливает его поверх запущенного исполняемого файла (или AppImage). Включено по умолчанию и не требует собственной библиотеки помимо Qt Network; отключите его для билдов, которые обновляет пакетный менеджер, и тогда _Проверить обновления_ лишь объясняет, что этот билд не может обновить сам себя. libarchive остаётся обязательной в любом случае: её используют резервные копии проекта.

#### Форматирование Ruby (`ENABLE_RUBY_FORMATTING`)

- libclang (только библиотека и хедеры, **не** полный тулчейн компилятора clang)

Билд скрипт `prism-sys` для `rubyfmt` использует `bindgen` с `libclang` для генерации FFI биндингов, так что этому флагу нужны библиотека и хедеры clang, даже когда сам проект целиком компилируется GCC/MSVC. Установите пакет дистрибутива с библиотекой/хедерами clang (например `libclang-common-*-dev` на Debian/Ubuntu, `clang-devel` на OpenSUSE, `clang21-libclang`/`clang21-headers` на Alpine) вместо полного пакета компилятора `clang`, если дистрибутив их разделяет.

#### mimalloc (`ENABLE_MIMALLOC`)

- mimalloc

Использует mimalloc вместо стандартного аллокатора памяти. По умолчанию отключено.

### Опции без дополнительных библиотек

Эти опции CMake переключают отдельные фичи Rust крейта и не требуют дополнительных системных библиотек - переключайте их свободно, чтобы урезать билд:

- Линтинг LanguageTool: `ENABLE_LANGUAGETOOL`.
- Бэкэнды машинного перевода: `ENABLE_GOOGLE_TRANSLATE`, `ENABLE_YANDEX_TRANSLATE`, `ENABLE_DEEPL`.
- Просмотрщик кода в просмотрщике ассетов: `ENABLE_JSON_HIGHLIGHTING`, `ENABLE_JS_HIGHLIGHTING`, `ENABLE_RUBY_HIGHLIGHTING` (подсветка синтаксиса) и `ENABLE_JS_FORMATTING` (кнопка "Отформатировать" для JavaScript; `ENABLE_RUBY_FORMATTING` описан выше).
- Форматы экспорта/импорта: `ENABLE_SERDE_CSV`, `ENABLE_SERDE_XML`, `ENABLE_SERDE_XLSX`, `ENABLE_SERDE_YAML`.
- Бэкэнды сегментации слов и стемминга для сопоставления глоссария: `ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_CHINESE_LINDERA`, `ENABLE_TOKENIZER_JAPANESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_LINDERA`, `ENABLE_TOKENIZER_KOREAN_LINDERA`, `ENABLE_TOKENIZER_SNOWBALL`, `ENABLE_TOKENIZER_SOUTHEAST_ASIAN`.
- Подсчёт слов (количество слов/символов в строке состояния и статистике проекта): `ENABLE_COUNTING_DICTIONARY` (сегментация по словарю для китайского, японского, тайского, лаосского, кхмерского и бирманского; без неё отрезок такого текста считается одним словом) и `ENABLE_COUNTING_LSTM` (LSTM модели для тайского, лаосского, кхмерского и бирманского). Обе по умолчанию включены. Подсчёт использует крейт `text-stats`, который нужно склонировать рядом с этим репозиторием (`../text-stats`), пока он не опубликован.

Все они по умолчанию установлены на `ON`, кроме токенизаторов на основе ICU (`ENABLE_TOKENIZER_CHINESE_ICU`, `ENABLE_TOKENIZER_JAPANESE_ICU`), которые по умолчанию установлены на `OFF`, поскольку вместо них включены эквиваленты на основе Lindera.

### Другие опции

- `ENABLE_PIC` (`ON`) - позиционно-независимый код.
- `MSVC_RUNTIME` (`MT`) - рантайм библиотека MSVC: `MT`, `MD`, `MTd` или `MDd`.
- `RUST_PROFILE` - профиль Cargo, с которым собирается Rust часть.
- `LUA_EXECUTABLE` - Lua, используемый скриптами перед сборкой. Бинарник Lua для Linux и Windows поставляется в репозитории; укажите эту опцию, чтобы использовать свой.

### Получение файлов для разработки на Linux

#### Debian

Основа проекта - Debian 13 (Trixie). Сборку для Ubuntu 24.04 смотрите ниже, там всё сильно по-другому.

`libqjs-dev` (quickjs-ng) и `libglaze-dev` есть только в `trixie-backports`, а не в обычном trixie - сначала включите его:

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

В репозиториях Tumbleweed есть только оригинальный `quickjs` (`quickjs-devel`), а не `quickjs-ng`. Соберите и установите его из исходников:

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

У Alpine есть какая-то проблема с musl и `ENABLE_RUBY_FORMATTING`: не получается слинковать libclang, необходимую билд скрипту `prism-sys`, так что обходите это сами, если хотите рабочий билд. Или просто установите `ENABLE_RUBY_FORMATTING` на `OFF`.

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

Билд AppImage нацелен именно на Ubuntu 24.04 для максимальной совместимости. Его список зависимостей сложнее, чем у других дистрибутивов: Qt 6.8 нет в репозиториях 24.04, так что он устанавливается через `aqtinstall`, а `quickjs-ng`/`glaze` тоже не запакованы, так что они собираются из исходников. Вместо того чтобы дублировать всё это здесь, шаг за шагом следуйте этапу `appimage-build` в Dockerfile - он является единственным источником истины для этой цели.

Как минимум, вам понадобятся:

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

Сам Qt, `quickjs-ng`, `glaze` и инструменты `linuxdeploy`/`linuxdeploy-plugin-qt` всё ещё нужно установить/собрать отдельно - точные шаги смотрите в Dockerfile.

#### Dockerfile

В корне репозитория есть Dockerfile, который включает сборки на вышеуказанных платформах, а также сборку AppImage для Ubuntu, и они всегда гарантированно удаются.

## Настройка

`configure.lua` - обёртка вокруг CMake для большего удобства:

```bash
./lua configure.lua --fresh -G=Ninja CMAKE_BUILD_TYPE=Release
```

Доступные опции CMake можно получить через `./lua configure.lua --help`.

Обычный CMake работает так же хорошо, если вы предпочитаете его.

## Сборка

```bash
cmake --build build
```

Результат сборки помещается в `build/target/bin/`.

## Обновление переводов

Каталоги переводов - это `translations/*.ts`. Проще всего обновить их через цель CMake, которая сама передаёт всё описанное ниже:

```bash
cmake --build build --target update_translations
```

Чтобы запустить `lupdate` вручную, ему нужно передать псевдонимы, позволяющие читать макросы `NOTICE`, которые он не раскрывает:

```
-tr-function-alias QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N
```

```bash
lupdate src \
  -tr-function-alias "QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N" \
  -ts translations/rpgmtranslate.en.ts translations/rpgmtranslate.ru.ts
```

Вызывайте `QCoreApplication::translate("Context", "text")` с литералами в качестве аргументов, а не `qApp->translate(...)`: он не разбирает `->translate` и молча теряет строки. Вызов `translate` с переменной (`translate("Lint", message)`) тоже не извлекается; помечайте такие строки через `QT_TRANSLATE_NOOP("Lint", "text")` там, где они написаны.

## Заметки для платформ

### Windows

Подробности того, как компилируются библиотеки для официального статического билда под Windows, смотрите в <https://github.com/savannstm/personal>, конкретно - в скриптах dev/build-dev-libraries.

### Linux

`Dockerfile` воспроизводимо собирает проект на пяти дистрибутивах/целях, каждая на своём этапе:

```bash
docker build --target debian-build .
docker build --target opensuse-build .
docker build --target appimage-build .
docker build --target arch-build .
docker build --target alpine-build .
```

Каждый этап принимает аргумент сборки `TARGET_ARCH` (по умолчанию `x86-64-v3`), который задаёт значение `-march`/`-Ctarget-cpu`, под которое компилируется код на C/C++/Rust:

```bash
docker build --target debian-build --build-arg TARGET_ARCH=x86-64-v2 .
```
