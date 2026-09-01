# rpgmtranslate-qt

[README на русском](./README-ru.md)
[简体中文](./README.zh-CN.md)

<p align="center">
    <img src="./icons/rpgmtranslate-logo.png" alt="Description" width="256"/>
</p>

A complete rewrite of the [original RPGMTranslate application](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate) in C++23/Qt6.

![Interface](./screenshots/gui.png)

## Features

- [x] Cross-platform, portable, fast and powerful.
- [x] Exceptional performance, powered by low-level Rust/C++ code.
- [x] Support for RPG Maker 2000/2003/XP/VX/VXAce/MV/MZ, with automated archive decryption and file parsing.
- [x] Easy plain text format that's manually editable.
- [x] Translations can be exported into other formats - XLSX, CSV, XML, YAML and JSON.
- [x] Built-in [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) features for more than 40 languages.
- [x] Convenience-oriented user experience with bookmarks, shortcuts, and highly customizable features.
- [x] Built-in batch processing of files, including batch translation, batch trim and batch text wrap.
- [x] Integrated APIs for machine translation - Google Translate, DeepL, AI endpoints, local endpoints, OpenRouter.
- [x] Integrated asset inspector, which allows to inspect images, videos, fonts, scripts and more.
- [x] Integrated spell-checking and advanced linting, including linting of tags from different plugins (like Yanfly Message Core) and information about them.
- [x] **partially** Integrated Git client.
- [x] **i guess** LanguageTool integration.

**Feature requests:** It's possible to implement virtually anything in Rust and C++. Request features that you and others would benefit from, and we will implement them.

Under the hood, this application uses:

- [rvpacker-txt-rs-lib](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib) to parse text from RPG Maker files and apply the translation.
- [marshal-rs](https://github.com/RPG-Maker-Translation-Tools/marshal-rs) to parse RPG Maker XP/VX/VX Ace files to JSON.
- [rpgm-archive-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-archive-decrypter-lib) to decrypt `.rgss` RPG Maker XP/VX/VX Ace archives.
- [rpgm-asset-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-asset-decrypter-lib) to decrypt MV/MZ assets.

Using these tools, the program parses the text to `.txt` files, allows you to edit them, and then write them back to the original form with translation applied.

If you have troubles figuring out the program, check the `Help > Usage Documentation` top menu option. That will probably help.

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

- [Ko-fi](https://ko-fi.com/savannstm)
- [Patreon](https://www.patreon.com/cw/savannstm)
- [Boosty](https://boosty.to/mcdeimos)

Even if you don't, it's fine. We'll continue to do as we right now.

## License

Project is licensed under [WTFPL](https://www.wtfpl.net/).

The repository contains third-party software, that is licensed under other conditions:

- `icons` - contains [Google Material Symbols](https://fonts.google.com/icons) - licensed under `Apache License Version 2.0`.
- `src/3rdparty` - contains 3rd-party libraries:
  - [fast_float](https://github.com/fastfloat/fast_float), [MIT](licenses/fast_float)
  - [jeaiii/itoa](https://github.com/jeaiii/itoa), [MIT](licenses/jeaiii-itoa)
  - [magic_enum](https://github.com/Neargye/magic_enum), [MIT](licenses/magic_enum)
  - [zmij](https://github.com/vitaut/zmij), [MIT](licenses/zmij)
  - [miniaudio](https://github.com/mackron/miniaudio), [Unlicense](licenses/miniaudio)
