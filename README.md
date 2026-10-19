# rpgmtranslate-qt

[README на русском](./README-ru.md)
[简体中文](./README.zh-CN.md)

<div style="display: flex; align-items: center; justify-content: center;">
    <img src="./icons/rpgmtranslate-logo.png" width="128"/>
</div>

A complete rewrite of the [original RPGMTranslate application](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate) in C++23/Qt6.

![Interface](./screenshots/gui.png)

## Features

- [x] Cross-platform, portable, fast and powerful.
- [x] Exceptional performance, powered by low-level Rust/C++ code.
- [x] Support for RPG Maker 2000/2003/XP/VX/VXAce/MV/MZ, with automated archive decryption and file parsing.
- [x] Easy plain text format that's manually editable.
- [x] Translations can be exported into other formats - XLSX, CSV, XML, YAML and JSON.
- [x] Built-in [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) features for more than 35 languages.
- [x] Convenience-oriented user experience with bookmarks, shortcuts, and highly customizable features.
- [x] Batch processing of files using an advanced scripting system: you can even spawn external processes!
- [x] Integrated asset inspector, which allows to inspect images, videos, fonts, scripts and more.
- [x] Integrated spell-checking and advanced linting, including linting of tags from different plugins (like Yanfly Message Core) and information about them.
- [x] **partially** Integrated Git client.
- [x] **i guess** LanguageTool integration.

**Feature requests:** Almost anything is possible in Rust and C++. Request features that you and others would benefit from, and we will implement them.

Under the hood, this application uses:

- [rvpacker-txt-rs-lib](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib) to parse text from RPG Maker files and apply the translation.
- [marshal-rs](https://github.com/RPG-Maker-Translation-Tools/marshal-rs) to parse RPG Maker XP/VX/VX Ace files to JSON.
- [rpgm-archive-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-archive-decrypter-lib) to decrypt `.rgss` RPG Maker XP/VX/VX Ace archives.
- [rpgm-asset-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-asset-decrypter-lib) to decrypt MV/MZ assets.
- [rm2k-lib](https://github.com/RPG-Maker-Translation-Tools/rm2k-lib) to decrypt/encrypt RPG Maker 2000/2003 files.

Using these tools, the program parses the text to `.txt` files, allows you to edit them, and then write them back to the original form with translation applied.

Stuck? Check the `Help > Usage Documentation` top menu option.

## Releases

Builds are published in the [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases) page.

Windows releases require Windows 10 or 11. They are linked statically, so there is nothing to install alongside them.

Every release needs a CPU with **x86-64-v3** support: Intel from Haswell (2013) onwards, AMD from Excavator (2015) onwards. Older hardware has to build from source with a lower `-march`.

Executables are compressed with: `upx --best --lzma --brute --ultra-brute --compress-exports=0`.

On Linux you will need [these dependencies](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/development#getting-development-headers-on-linux) to run the application.

## Documentation

Full documentation lives at <https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/>.

### Installation

See the [installation documentation](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/installation).

### Development

See the [development documentation](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/development).

## Support

[Me](https://github.com/savannstm), the maintainer of this project, is a poor college student from Eastern Europe.

If you could, please consider supporting us through:

- [Boosty](https://boosty.to/mcdeimos) (has lower fees)
- [Ko-fi](https://ko-fi.com/savannstm)

We will keep working on the project either way.

## License

Licensed under [WTFPL](https://www.wtfpl.net/).

The repository contains third-party software, that is licensed under other conditions:

- `icons` - [Google Material Symbols](https://fonts.google.com/icons), [Apache License v2.0](licenses/material-design-icons).
- `src/3rdparty` - contains 3rd-party libraries:
  - [fast_float](https://github.com/fastfloat/fast_float), [MIT](licenses/fast_float)
  - [enchantum](https://github.com/ZXShady/enchantum), [MIT](licenses/enchantum)
  - [zmij](https://github.com/vitaut/zmij), [MIT](licenses/zmij)
  - [miniaudio](https://github.com/mackron/miniaudio), [Unlicense](licenses/miniaudio)
  - [TinySoundFont](https://github.com/schellingb/TinySoundFont), [MIT](licenses/TinySoundFont) (`tml.h`: Zlib)
