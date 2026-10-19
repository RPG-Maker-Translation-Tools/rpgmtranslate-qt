---
title: "Settings"
---

![](./assets/settings.svg) Open Settings from the File menu.

The window is split into five sections, listed on the left: **Core**, **Appearance**, **Controls**, **Translation**, **Project**. The window has **Save** and **Cancel** buttons. Save applies the changes; if a field holds an invalid value (e.g. an out-of-range backup period), the window refuses to close and points you at the offending field instead. Cancel, `Esc` and the close button discard all changes, including style, theme and removed endpoints. Settings can't be opened while a [task](./tasks.md) is running.

Core, Appearance, Controls and Translation are stored once, globally, in `settings.json`. Only the Project section is stored **per project** (in `.rpgmtranslate/project-settings.json`), so different projects can use different languages, dictionaries and line length hints, but share endpoints and lint configuration. The Project section only has an effect while a project is open.

## Core

### Backup

| Setting       | Description                                                               |
| ------------- | ------------------------------------------------------------------------- |
| Enabled       | Turns automatic project backups on or off.                                |
| Backup period | How often a backup is created, in seconds (60-3600).                      |
| Max backups   | How many backups to keep per project before the oldest is deleted (1-99). |

To restore one, use File > Load Backup. It lists your project's backups by date and warns you that loading one **overwrites your current, unsaved progress** - make sure that's what you want first.

### Updates

| Setting                       | Description                                                                                                |
| ----------------------------- | ---------------------------------------------------------------------------------------------------------- |
| Check for updates             | Checks for a new application release on startup.                                                           |
| Check for source file updates | Checks whether the game's source files changed since you last read them, and offers to re-read them if so. |

### Git

Identity used when you commit from the built-in [git client](./git.md).

| Setting                            | Description                                                                                                         |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| Prefer the repository's git config | If the repository already has an author identity set in its own `git config`, use that instead of the fields below. |
| Name / Email                       | The author identity to commit as, when not deferring to `git config`.                                               |

## Appearance

| Setting                       | Description                                                                                                                   |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| Translation table font / size | Font and size (8-96pt) used in the translation table.                                                                         |
| Style                         | The Qt widget style - affects the look of buttons, lists, and other controls.                                                 |
| Theme                         | Light, Dark, or follow the system (Default).                                                                                  |
| Display progress as percents  | Shows each file's translation progress in the tab panel as a percentage instead of a translated/total count.                  |
| Display entry names           | Adds the name of the entry a row belongs to (the actor, item, map... from its `<!>NAME` row) to search results and bookmarks. |

The application's own UI language (English, Russian or Simplified Chinese) is picked from the **Language** menu in the menu bar, not here.

## Controls

Keyboard shortcuts for opening panels and menus. Click a shortcut field and press the key combination you want.

| Action            | Default        |
| ----------------- | -------------- |
| Search panel      | `Ctrl+R`       |
| Tab panel         | `Tab`          |
| Go to row         | `Ctrl+G`       |
| Batch menu        | `Ctrl+B`       |
| Bookmark menu     | `Alt+B`        |
| Lint menu         | `Ctrl+M`       |
| Glossary menu     | `Alt+L`        |
| Translations menu | `Alt+T`        |
| Trim cell         | `Ctrl+Shift+T` |
| Wrap cell         | `Ctrl+Shift+W` |
| Latinize cell     | `Ctrl+Shift+L` |

The last three run the built-in [Trim, Wrap and Latinize scripts](./batch-processing.md) on the cell you are editing (or the selected translation cell), so you don't need to open the Batch menu for one cell. Trim removes leading and trailing whitespace, and Wrap uses the project's line length hint (it asks you to set it if it is 0). While a cell is open for editing, the change can be undone with `Ctrl+Z`.

The same shortcut can't be assigned to two of these actions at once - trying to save with a duplicate in place is rejected until you change one of them. The same goes for assigning one of the fixed shortcuts below.

There are special cases for shortcuts, which cannot be changed:

| Action                           | Default                                                                                                         |
| -------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| Save                             | [QKeySequence::Save](https://doc.qt.io/qt-6/qkeysequence.html#standard-shortcuts) (Ctrl+S)                      |
| Write                            | Ctrl+W                                                                                                          |
| Search                           | [QKeySequence::Find](https://doc.qt.io/qt-6/qkeysequence.html#standard-shortcuts) (Ctrl+F)                      |
| Close tab                        | Esc                                                                                                             |
| Copy / Cut / Paste / Undo / Redo | The platform's standard shortcuts ([QKeySequence](https://doc.qt.io/qt-6/qkeysequence.html#standard-shortcuts)) |

## Translation

Configures machine-translation endpoints, LanguageTool, and the lint system.

API keys are kept in your OS's own credential store (Windows Credential Manager, or the Secret Service keyring on Linux) instead of `settings.json`, and only read back in when needed. If the store refuses a key, it stays in `settings.json` and you get a warning.

### Endpoints

Endpoints are listed on the left of this section; add or remove them with the buttons beside the list, and double-click an endpoint to rename it. Every endpoint is queried when you start editing a cell, see [Machine Translation](./text-editing.md#machine-translation). The fields depend on the endpoint type:

- **Google** - free, no configuration needed.
- **Yandex** - requires an API key and a Yandex Cloud folder ID. "Check key" verifies the key.
- **DeepL** - requires an API key. "Check key" verifies the key.
- **Script** - runs a JavaScript file you provide, so any translation service or AI model can be connected. The file defines `rpgmTranslateCallback(text, sourceLanguage, translationLanguage, glossary, filename)` and returns the translated text as a string. `glossary` is an array of the enabled [glossary](./glossary.md) terms as `{ term, translation, note }`, and is empty unless the endpoint's "Use glossary" checkbox is on. When you close Settings, you are asked once per script version whether it may run programs and read or write files; a script you did not approve will not run. See [Batch processing](./batch-processing.md#what-scripts-can-do) for the functions available to scripts and an Ollama example.

A backend left out of the build (see [Development](./development.md#feature-flags-without-extra-libraries)) can still be selected, but its description warns that it was not compiled into the build, and using it fails.

### LanguageTool

Configures the [LanguageTool](https://languagetool.org/) grammar checker used for [linting](./text-editing.md#linting). The local server and the paid hosted API are both supported, but only the local server has been tested.

| Setting                                             | Description                                                                                        |
| --------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| Base URL                                            | Your LanguageTool server, local or hosted. "Check connection" verifies it's reachable.             |
| Username / API key                                  | Needed only for the paid hosted API, to get Premium access.                                        |
| Mother tongue                                       | Your native language, to enable false-friend checks for some language pairs.                       |
| Enabled only                                        | If on, only the categories/rules explicitly listed below are checked - everything else is ignored. |
| Enabled/disabled categories, enabled/disabled rules | Comma-separated rule/category IDs to narrow or trim what gets checked.                             |
| Preferred variants                                  | Comma-separated regional spelling variants LanguageTool should prefer (e.g. `en-US` over `en-GB`). |
| Dicts                                               | Comma-separated extra dictionaries to pull words from. Leave unset to use the default.             |
| Picky mode                                          | Turns on additional rules mostly useful for formal text.                                           |

### Words and characters

"Display words/characters count for a file" shows, in the status bar, how many words and characters the source and the first translation column of the open file have. The counts follow your edits a moment after you leave a cell, and the tooltip also shows the characters without whitespace.

The counts are the ones a translator would bill by, and RPG Maker codes (`\C[2]`, `\N[1]`, `<br>` and so on) are not counted:

- **Words** are the word-like segments of the text: letters, numbers and CJK ideographs/kana, but not whitespace or punctuation. `don't` and `3.14` are one word, `well-known` is two. Japanese, Chinese, Thai, Lao, Khmer and Burmese, which have no spaces between words, are split with a built-in dictionary, so a Japanese source gets a sensible word count too.
- **Characters** are what a reader sees as one character (a letter with its accent, or an emoji, is one), spaces included.

**File > Project Statistics** counts every file of the project at once (the open tab is saved first) and shows the counts per file with the totals at the bottom.

### Lints

Everything here can also be toggled per item from the "Lints" button in the [lint menu](./text-editing.md#linting). Configure and edit them here, not just toggle them.

- **Sequence lints** - four read-only tables (Text Codes, Note Tags, Plugin Commands, Comment Tags) listing every built-in plugin/tag pattern RPGMTranslate recognizes, grouped by category. Each row can be given a custom highlight color.
- **Custom lints** - your own lint patterns, in an editable table: a sequence to match, a tooltip shown on hover, case sensitivity, and an optional highlight color. Double-click a cell to edit it. Custom lints can be exported to and imported from a JSON file, to back them up or share them.
- **Misc**:
  - Sequence replacements - a table of character sequences that get silently substituted as you type (e.g. `<<` -> `«`), useful for non-ASCII punctuation your keyboard doesn't have. A few common ones are pre-populated but disabled by default.
  - Whitespace characters - which Unicode codepoints (`U+XXXX`, comma-separated) count as whitespace for the leading/trailing/contiguous whitespace lints, in addition to the space and the tab, which always do.

## Project

Stored per project.

| Setting                                | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| -------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Line length hint                       | Draws a thin red line in the translation cell at this many characters (0-255), as a wrapping guide.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| Source language / Translation language | The source and target languages, used for many different things.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Spellcheck dictionary                  | The Nuspell dictionary to check against. See below for where to get one.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| Write encoding                         | UTF-8 by default, which is right for XP, VX, VX Ace, MV and MZ (XP and VX are generally UTF-8 because RGSS sets `$KCODE` to UTF-8, though a game _might probably_ override that). Change it only for RPG Maker 2000/2003, which use the OS's legacy ANSI codepage, or for a game that overrides `$KCODE`, and only if the translation fits in that codepage. Text that can't be encoded in that codepage (a Cyrillic translation in Shift-JIS, for instance) fails the whole file with an error, instead of being written as `&#1055;`-style references. The translated game's system must use the same codepage (on Windows 10 1903+, "Beta: Use Unicode UTF-8" under Region settings makes UTF-8 work for RPG Maker 2000/2003). |

### Line length hint

The hint is a rough estimate of how many characters fit on a line, drawn at a fixed position that doesn't move as you type. It uses the average width of printable ASCII and a fixed CJK sample (punctuation, kana, kanji), since translators often keep CJK punctuation.

- **Hint font** - the font your game uses. It does not change how the editor looks; it is only used to decide when a line is too long.
- **Hint font size** - the game's base message font size in pixels (`0` means 28, the MV/MZ default). It lets size tags such as `\{`, `\}` and `\fs[n]` be taken into account.

The hint line is drawn in a stronger red beside any text line that is too long.

### Spell check

RPGMTranslate doesn't ship spellcheck dictionaries - drop `.aff`/`.dic` files into the `dictionaries` directory of the [application data directory](./installation.md#runtime-data-files) (subdirectories are fine) and they'll show up in the dropdown above. Sources:

- [LibreOffice dictionaries](https://github.com/LibreOffice/dictionaries)
- [This dictionaries collection](https://github.com/wooorm/dictionaries)

CJK isn't supported yet.

### CJK text matching

Japanese, Chinese, and Korean [glossary matching](./glossary.md) needs word-boundary dictionaries, since these languages have no whitespace between words. RPGMTranslate doesn't ship them: they are multi-hundred-megabyte files that would bloat every install. Grab the matching archive from [Lindera's releases page](https://github.com/lindera/lindera/releases), unzip it, and move its files (`dict.trie`, `dict.words`, `metadata.json`, etc.) into `lindera-dictionaries/<language>` in the [application data directory](./installation.md#runtime-data-files), so they sit directly inside it. Folders are read at startup, so restart after adding one:

| Language | Folder                          | Archive to download                                                                        |
| -------- | ------------------------------- | ------------------------------------------------------------------------------------------ |
| Japanese | `lindera-dictionaries/japanese` | `lindera-ipadic-neologd-*.zip` (best matching quality) or `lindera-ipadic-*.zip` (smaller) |
| Chinese  | `lindera-dictionaries/chinese`  | `lindera-cc-cedict-*.zip`                                                                  |
| Korean   | `lindera-dictionaries/korean`   | `lindera-ko-dic-*.zip`                                                                     |

If a language's folder is missing, empty, or its files ended up nested inside an extra subfolder, matching for that language fails with an error instead of silently falling back to something else.
