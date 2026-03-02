# rpgmtranslate-qt

[README на русском (в работе)](./README-ru.md)

<p align="center">
    <img src="./icons/rpgmtranslate-logo.png" alt="Description" width="256"/>
</p>

A complete rewrite of the [original RPGMTranslate application](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate) in C++23/Qt6.

![Interface](./screenshots/gui.png)

## Features

- [x] Cross-platform, portable, fast and powerful.
- [x] Exceptional performance, powered by low-level Rust/C++ code.
- [x] Support for RPG Maker XP/VX/VXAce/MV/MZ, with automated archive decryption and file parsing.
- [x] Easy plain text format that's manually editable.
- [x] Built-in [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) features for more than 40 languages.
- [x] Convenience-oriented user experience with bookmarks, shortcuts, and highly customizable features.
- [x] Built-in batch processing of files, including batch translation, batch trim and batch text wrap.
- [x] Integrated APIs for Google Translate, Yandex Translate, DeepL, ChatGPT, Claude, DeepSeek, Gemini, Ollama, and any OpenAI-compatible endpoints (e.g. koboldcpp).
- [] **WIP** Integrated spell-checking and advanced linting, including LanguageTool, custom dictionaries, whitespace highlighting etc.

**Please, request features!** It's possible to implement virtually anything in Rust and C++. Request features that you and others would benefit from, and we will implement them.

Under the hood, this application uses:

- [rvpacker-txt-rs-lib](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib) to parse text from RPG Maker files and apply the translation.
- [marshal-rs](https://github.com/RPG-Maker-Translation-Tools/marshal-rs) to parse RPG Maker XP/VX/VX Ace files to JSON.
- [rpgm-archive-decrypter-lib](https://github.com/RPG-Maker-Translation-Tools/rpgm-archive-decrypter-lib) to decrypt `.rgss` RPG Maker XP/VX/VX Ace archives.

Using these tools, the program parses the text to `.txt` files, allows you to edit them, and then write them back to the original form with translation applied.

If you have troubled figuring out the program, check the `Help > Usage Documentation` top menu option. That will probably help.

## Installation

**Download the latest version from the Releases section.**

## Usage

Documentation is available at <https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/>.

## Development

See [Development Documentation](https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt/development).

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
- `dictionaries` - contains [LibreOffice dictionaries](https://cgit.freedesktop.org/libreoffice/dictionaries) - licensed under a variety of licenses.
