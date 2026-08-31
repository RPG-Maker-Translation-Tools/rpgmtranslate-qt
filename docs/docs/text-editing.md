# Text Editing

When opening a ![](./assets/menu.svg) tab, you'll be presented with a translation table, which allow to edit translation cells like a usual text editors.

There are some crucial features in play for such editors.

## Machine Translation

If you configured the endpoints for [single-translation](./settings.md#translation), they will be used to translate the source text corresponding to the edited translation cell. Results show up in the ![](./assets/translate.svg) translations menu - click one to insert it into the cell you're editing.

## Bookmarks

Right-click a row and pick "Bookmark Row" to save your place. Bookmarks are listed in the ![](./assets/bookmarks.svg) bookmark menu, grouped by file - click one to jump straight back to that row.

## Linting

Lints are generally have some kind of special highlighting when focusing the translation cell (e.g. text color, background, wavy line) or/and they are displayed in the lint menu.

Lint menu is available through ![](./assets/lab_profile.svg) button. It provides:

- "Disable linting" - a global switch that overrides every setting below it.
- Individual checkboxes for whitespace/punctuation/tag-mismatch/spellcheck/LanguageTool/glossary lints.
- "Lints" - opens a menu to toggle individual plugin/tag lints and custom lints on and off.
- A file-select button, to scope the check below to specific files.
- "Global Check" - runs every enabled lint across the selected files as a background task, instead of only linting the cell you're currently editing.

Hovering a lint highlight shows a tooltip with details about it. For text codes (`\C[x]`, `\I[n]`, `\N[n]`, ...), the tooltip resolves and displays the actual referenced data where possible - the real actor/item/skill name, the variable's current value, a rendered icon, or a color swatch - instead of just the raw code. Some tooltips also include a clickable link to apply a suggested fix directly.

### LanguageTool

If you have configured the [LanguageTool](./settings.md#languagetool), LanguageTool will be used to lint the translation text.

### Spell Check

RPGMTranslate supports spell check out of the box, thanks to Nuspell library.

On information about how to find, install and activate dictionaries, see [Spell Check Settings](./settings.md#spell-check)

Note that CJK is not yet supported.

### Glossary

If you have configured the [Glossary](./glossary.md), the application will check term consistency and term mismatches in the source/translation text.

### Plugin & Misc

RPGMTranslate provides built-in linting for all plugins described on Yanfly's wiki pages, as well as some misc lints not related to the plugins.

Listing them all in here would be a waste of time, and synchronizing the lists between the source code and documentation would also be very bothersome. You could check which plugins are implemented in sources:

- `src/Lints/CommentTags.inl`
- `src/Lints/Notetags.inl`
- `src/Lints/PluginCommands.inl`
- `src/Lints/TextCodes.inl`

All individual plugin lints can be enabled and disabled in a menu opened with "Lints" button.

As for misc lints, there's the list of those:

- Whitespace
  - Leading whitespace - adds a lint to the lint menu, highlights leading whitespace.
  - Trailing whitespace - adds a lint to the lint menu, highlights trailing whitespace.
  - Contiguous whitespace - adds a lint to the lint menu, highlights contiguous whitespace.
  - Which characters count as whitespace for these three lints is configurable, see [Translation > Lints settings](./settings.md#lints).
- Unclosed punctuation - adds a lint to the lint menu, highlights unclosed punctuation, such as unclosed quotes.
- Source/translation tag mismatch - adds a lint to the lint menu when source tags and translation tags differ. This happens when the translation cell don't has the tags present in the source text.

## Keyboard Shortcuts

- Shift + ↓ - Move to next row (down)
- Shift + ↑ - Move to previous row (up)
- Shift + ← - Move to the left translation column
- Shift + → - Move to the right translation column
- Ctrl + ↑ - Move to the top-most row
- Ctrl + ↓ - Move to the bottom-most row
- Left-click on source text - Copy the source text

## Batch Select

You can select multiple translation cells by clicking one of them and extending the selection by holding Shift and clicking the other one.

Batch copy/cut/paste is supported.

## Batch Text Processing

Access it through the ![](./assets/construction.svg) button.

[Documentation on batch processing](./batch-processing.md)
