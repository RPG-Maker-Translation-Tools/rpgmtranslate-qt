# rpgmtranslate-qt

[简体中文](./README.zh-CN.md)

<div style="display: flex; align-items: center; justify-content: center;">
    <img src="./icons/rpgmtranslate-logo.png" width="128"/>
</div>

Полностью переписанное на C++23/Qt6 [оригинальное приложение RPGMTranslate](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate).

![Интерфейс](./screenshots/gui.png)

## Особенности

- [x] Кросс-платформенное, портабельное, быстрое и мощное.
- [x] Невероятная производительность, поддерживаемая низкоуровневым кодом Rust/C++.
- [x] Поддержка движков RPG Maker 2000/2003/XP/VX/VXAce/MV/MZ, с автоматическим расшифровыванием архивов и парсингом файлов.
- [x] Простой текстовый формат файлов перевода, который можно редактировать вручную.
- [x] Перевод может быть экспортирован в другие форматы - XLSX, CSV, XML, YAML and JSON.
- [x] Встроенные [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation)-функции для более чем тридцати пяти языков.
- [x] Ориентированный на удобство интерфейс со вкладками, горячими клавишами и настраиваемыми функциями.
- [x] Встроенная массовая обработка файлов со своей скриптовой системой: можно даже создавать отдельные процессы!
- [x] Встроенный просмотрщик ассетов, позволяющий изучать изображения, видео, шрифты, скрипты и прочее.
- [x] Встроенная проверка правописания и обширный линтинг, включая проверку тегов из различных плагинов (вроде Yanfly Message Core) и информация о них.
- [x] **частично** Встроенный Git клиент.
- [x] **я надеюсь** Интеграция с LanguageTool.

**Запросы функций:** Возможно сделать практически что угодно, используя Rust и C++. Запрашивайте функции которые необходимы вам и помогут другим, и мы их сделаем.

Под капотом, приложение использует:

- [rvpacker-txt-rs-lib](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib) чтобы парсить текст из файлов RPG Maker и применять перевод.
- [marshal-rs](https://github.com/RPG-Maker-Translation-Tools/marshal-rs) чтобы парсить файлы RPG Maker XP/VX/VX Ace в JSON.
- [rpgm-archive-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-archive-decrypter-lib) чтобы расшифровывать архивы `.rgss` для RPG Maker XP/VX/VX Ace.
- [rpgm-asset-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-asset-decrypter-lib) чтобы расшифровывать ассеты MV/MZ.
- [rm2k-lib](https://github.com/RPG-Maker-Translation-Tools/rm2k-lib) для расшифровки/зашифровки файлов RPG Maker 2000/2003.

Используя эти инструменты, программа парсит текст в `.txt` файлы, позволяя вам их редактировать и записывать обратно в исходную форму с применённым переводом.

Если у вас возникли проблемы при использовании программы, почитайте меню `Помощь > Документация по использованию`. Возможно это поможет.

## Релизы

Билды публикуются на странице [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases).

Релизы для Windows требуют Windows 10 or 11. Они линкуются статически, так что больше ничего особо и не надо устанавливать.

Каждый релиз нуждается в процессоре с поддержкой **x86-64-v3**: Intel от Haswell (2013) и далее, AMD от Excavator (2015) и далее. Старому железу надо билдить из исходников.

Исполняемые файлы сжаты через: `upx --best --lzma --brute --ultra-brute --compress-exports=0`.

На Linux вам будут необходимы [данные зависимости](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/ru/development#getting-development-headers-on-linux) чтобы запустить приложение.

## Документация

Полная документация находится по адресу <https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/ru/>.

### Установка

Изучите [документацию по установке](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/ru/installation).

### Разработка

Изучите [документацию по разработке](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/ru/development).

## Поддержка

[Я](https://github.com/savannstm), мейнтейнер данного проекта, бедный студентик из Восточной Европы.

Если можете, поддержите нас через:

- [Boosty](https://boosty.to/mcdeimos) (меньше комиссия)
- [Ko-fi](https://ko-fi.com/savannstm)

Даже если не поддержите, мы будем продолжать трудиться.

## Лицензия

Проект лицензирован под [WTFPL](https://www.wtfpl.net/).

Репозиторий содержит софт третьей стороны, лицензированный под другими условиями:

- `icons` - содержит [Google Material Symbols](https://fonts.google.com/icons) - лицензировано под `Apache License Version 2.0`.
- `src/3rdparty` - содержит библиотеки третьей стороны:
  - [fast_float](https://github.com/fastfloat/fast_float), [MIT](licenses/fast_float)
  - [jeaiii/itoa](https://github.com/jeaiii/itoa), [MIT](licenses/jeaiii-itoa)
  - [enchantum](https://github.com/ZXShady/enchantum), [MIT](licenses/enchantum)
  - [zmij](https://github.com/vitaut/zmij), [MIT](licenses/zmij)
  - [miniaudio](https://github.com/mackron/miniaudio), [Unlicense](licenses/miniaudio)
  - [TinySoundFont](https://github.com/schellingb/TinySoundFont), [MIT](licenses/TinySoundFont) (`tml.h`: Zlib)
