# Settings

![](./assets/settings.svg) Open Settings from the File menu.

The window is split into five sections, listed on the left: **Core**, **Appearance**, **Controls**, **Translation**, **Project**. Settings save when you close the window - if a field holds an invalid value (e.g. an out-of-range backup period), the window will refuse to close and point you at the offending field instead.

Core, Appearance and Controls are stored once, globally. Translation and Project settings are stored **per project**, so different projects can use different endpoints, languages, and lint configuration.

## Core

### Backup

| Setting       | Description                                                        |
| ------------- | ------------------------------------------------------------------ |
| Enabled       | Turns automatic project backups on or off.                         |
| Backup period | How often a backup is created, in seconds (60-3600).               |
| Max backups   | How many backups to keep per project before the oldest is deleted. |

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

| Setting                       | Description                                                                       |
| ----------------------------- | --------------------------------------------------------------------------------- |
| Translation table font / size | Font and size (8-96pt) used in the translation table.                             |
| Style                         | The Qt widget style - affects the look of buttons, lists, and other controls.     |
| Theme                         | Light, Dark, or follow the system.                                                |
| Language                      | The application's own UI language (English or Russian).                           |
| Display percents              | Shows task progress in the tab panel as a percentage instead of a raw item count. |

## Controls

Keyboard shortcuts for opening panels and menus. Click a shortcut field and press the key combination you want.

| Action            | Default  |
| ----------------- | -------- |
| Search panel      | `Ctrl+R` |
| Tab panel         | `Tab`    |
| Go to row         | `Ctrl+G` |
| Batch menu        | `Ctrl+B` |
| Bookmark menu     | `Alt+B`  |
| Lint menu         | `Ctrl+M` |
| Glossary menu     | `Alt+B`  |
| Translations menu | `Ctrl+S` |

The same shortcut can't be assigned to two actions at once - trying to close the window with a duplicate in place is rejected until you change one of them.

## Translation

This is the largest section. It configures machine-translation endpoints, LanguageTool, and the lint system.

API keys never get written to `settings.json` - they're kept in your OS's own credential store (Windows Credential Manager, or the Secret Service keyring on Linux) instead, and only read back in when needed.

### Endpoints

Endpoints are listed on the left of this section; add or remove them with the buttons above the list. Each endpoint has:

| Setting            | Description                                                                                                                                                                                                     |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Single translation | When enabled, editing a translation cell also translates the source text through this endpoint and shows the result in the translations menu. Watch out with priced LLM APIs - this burns tokens on every edit. |
| Use glossary       | Supplies your [glossary](./glossary.md) terms to the endpoint when translating.                                                                                                                                 |

Beyond that, the fields depend on the endpoint type:

- **Google** - free, no configuration needed.
- **Yandex** - requires an API key and a Yandex Cloud folder ID.
- **DeepL** - requires an API key.
- **LLM endpoints** (OpenAI-compatible, Ollama, and similar) share one set of fields:
  - API key / Base URL - credentials and server address. The API key is optional for local providers.
  - Model - fetched via "Check key", which validates your credentials and populates the model list.
  - Token limit / Output token limit - request and response token budgets (1000-65536).
  - Temperature - sampling randomness (0.0-2.0).
  - Thinking / Reasoning effort - enables extended thinking on models that support it, and how much effort to spend on it.
  - System prompt / Single-translation system prompt - the instructions sent to the model; each can be reset to its built-in default.

Providers not in the type list (e.g. [OpenRouter](https://openrouter.ai/)) work through the generic **OpenAI-Compatible** type - just point Base URL at the provider's OpenAI-compatible endpoint (for OpenRouter, `https://openrouter.ai/api/v1`) and use your API key from that provider.

### LanguageTool

Configures the [LanguageTool](https://languagetool.org/) grammar checker used for [linting](./text-editing.md#linting). Both the local server and the paid hosted API are supported by the settings, but only the local server has actually been tested by us.

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

### Lints

Everything here can also be toggled per-item from the "Lints" button in the [lint menu](./text-editing.md#linting) - this is just where you configure and edit them, not just flip them on/off.

- **Sequence lints** - four read-only tables (Text Codes, Note Tags, Plugin Commands, Comment Tags) listing every built-in plugin/tag pattern RPGMTranslate recognizes, grouped by category. Each row can be given a custom highlight color.
- **Custom lints** - your own lint patterns, in an editable table: a sequence to match, a tooltip shown on hover, case sensitivity, and an optional highlight color. Double-click a cell to edit it. Custom lints can be exported to and imported from a JSON file, to back them up or share them.
- **Misc**:
  - Sequence replacements - a table of character sequences that get silently substituted as you type (e.g. `<<` -> `«`), useful for non-ASCII punctuation your keyboard doesn't have. A few common ones are pre-populated but disabled by default.
  - Whitespace characters - which Unicode codepoints (`U+XXXX`, comma-separated) count as whitespace for the leading/trailing/contiguous whitespace lints.

## Project

Stored per project.

| Setting                                | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| -------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Line length hint                       | Draws a thin red line in the translation cell at this many characters (0-255), as a wrapping guide.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| Source language / Translation language | The source and target languages, used for [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) matching. Limited to languages supported by our [text-matching library](https://github.com/savannstm/language-tokenizer/blob/master/src/lib.rs#L139).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| Spellcheck dictionary                  | The Nuspell dictionary to check against. See below for where to get one.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Write encoding                         | A translation is not generally representable in the source game's own codepage (Japanese `Shift-JIS` has no Cyrillic to translate a Russian translation into, for instance), and forcing the wrong one doesn't fail loudly - an unmappable character gets silently replaced with a literal `&#1055;`-style numeric reference spliced into the output. Only set write encoding when the target engine build has no Unicode-aware text renderer to fall back on - true of RPG Maker 2000/2003, XP and VX, which render through the OS's legacy ANSI codepage rather than decoding UTF-8 - _and_ the translation's own script fits inside the codepage you choose. Whoever runs the translated game then also needs their system (or a locale-emulation tool) set to that same codepage; on Windows 10 1903+, checking "Beta: Use Unicode UTF-8 for worldwide language support" under Region settings makes the default UTF-8 output work for these engines too, without any of this. VX Ace is Unicode-native, so its UTF-8 default just works regardless. |

### Spell check

RPGMTranslate doesn't ship spellcheck dictionaries - drop `.aff`/`.dic` files into `APPLICATION_DIR/dictionaries` and they'll show up in the dropdown above. Sources:

- [LibreOffice dictionaries](https://github.com/LibreOffice/dictionaries)
- [This dictionaries collection](https://github.com/wooorm/dictionaries)

CJK isn't supported yet.

### CJK text matching

Japanese, Chinese, and Korean [CAT matching](#project) needs word-boundary dictionaries (there's no whitespace between words in these languages), and RPGMTranslate doesn't ship them either - they're multi-hundred-megabyte files that would otherwise bloat every install. Grab the matching archive from [Lindera's releases page](https://github.com/lindera/lindera/releases) and unzip it, than move the files from the unpacked folder (`dict.trie`, `dict.words`, `metadata.json`, etc.) into `APPLICATION_DIR/lindera-dictionaries/<language>`, so `dict.trie` and friends sit directly inside it. They're picked up automatically the next time that language is used - no rebuild or restart needed beyond that:

| Language | Folder                          | Archive to download                                                                        |
| -------- | ------------------------------- | ------------------------------------------------------------------------------------------ |
| Japanese | `lindera-dictionaries/japanese` | `lindera-ipadic-neologd-*.zip` (best matching quality) or `lindera-ipadic-*.zip` (smaller) |
| Chinese  | `lindera-dictionaries/chinese`  | `lindera-cc-cedict-*.zip`                                                                  |
| Korean   | `lindera-dictionaries/korean`   | `lindera-ko-dic-*.zip`                                                                     |

If a language's folder is missing, empty, or its files ended up nested inside an extra subfolder, matching for that language fails with an error instead of silently falling back to something else.

### Context

Free-form text supplied to LLM endpoints when translating:

- **Project context** - overall setting, tone, terminology, audience - applies to the whole project.
- **File context** - pick a file from the list, then write context specific to it (character names appearing only in that file, tone shifts, constraints, etc.).
