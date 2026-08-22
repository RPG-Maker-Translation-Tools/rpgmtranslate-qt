# RPGMTranslate

RPGMTranslate is an open source and simple translation tool for RPG Maker games. Among its main features are an exceptional performance and plain text file format.

The application itself is a portable binary. Everything that the application writes tries to stay beside it - settings, lock file, etc. Application data directory is changed by the `RPGMTRANSLATE_DATA_DIR` environment variable, see [Installation](installation.md#runtime-data-files).

## Getting started

To start translating anything, you just need to ![](./assets/folder_open.svg) open a directory that contains RPG Maker game. More specifically, it should contain `data` (or `Data`) directory, or an RGSS archive (`Game.rgssad`, `Game.rgss2a`, `Game.rgss3a`).

The first time you open a new project, a Read options dialog pops up before anything is parsed - pick your options (or just accept the defaults) and click Apply. See [Rvpacker > Read](rvpacker.md#read) for what each option does. After that, the application decrypts the archive if needed, pulls text from the data directory files, creates an `.rpgmtranslate` directory, and opens the project.

From here you can start doing things. Don't know where to start? [Installation](installation.md) and [Text Editing](text-editing.md) are the best places to begin.

## Pages

|                                                  | Page                                | What's there                                                                 |
| ------------------------------------------------ | ------------------------------------ | ----------------------------------------------------------------------------- |
|                                                    | [Installation](installation.md)     | Getting a release, requirements, where RPGMTranslate keeps its files          |
|                                                    | [Text Editing](text-editing.md)     | The translation table, linting, keyboard shortcuts, batch select             |
| ![](./assets/search.svg)                          | [Search Features](search.md)        | Search, regex-powered replace, and put                                       |
| ![](./assets/construction.svg)                    | [Batch Processing](batch-processing.md) | Translating or scripting many files at once, custom JS scripts           |
|                                                    | [Tasks](tasks.md)                   | How the background task system works: what can run in parallel, what can be aborted |
| ![](./assets/settings.svg)                        | [Settings](settings.md)             | Every setting in the settings window, explained                              |
| ![](./assets/book_3.svg)                          | [Glossary](glossary.md)             | CAT term glossary: creating, matching, and linting terms                     |
| `rv`                                               | [Rvpacker](rvpacker.md)             | How reading, writing, and purging translation files actually works          |
| ![](./assets/commit.svg)                          | [Git](git.md)                       | The built-in git client: what it can and can't do yet                        |
| ![](./assets/stock_media.svg)                     | [Asset Inspector](asset-inspector.md) | Browsing images, code, audio/video, and fonts from the project              |
|                                                    | [Development](development.md)       | Building from source: dependencies, CMake options, per-platform notes        |

The icon column shows the toolbar button that opens each feature, where it has one, so you can match a page to a button at a glance.

**Also, help us improve [the docs](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/tree/main/docs/docs)!**
