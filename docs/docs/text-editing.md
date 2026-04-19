# Text Editing

## Translation Table

When you open a tab, translation table with the content of the tab will be built.

## Translation Input

By double-clicking or single-clicking and pressing a keyboard button on a cell in a translation column, you can trigger editing of the cell. While you're editing it, depending on the enabled options, such as row length hint, whitespace highlighting, spell-check, corresponding decorations will be shown.

## Auto-translation

If you configured the endpoints for [single-translation](./settings.md#translation), they will be used to translate the source text corresponding to the edited translation cell.

## Linting

### LanguageTool

If you configured the [LanguageTool](./settings.md#languagetool), LanguageTool will be used to lint the translation text.

LanguageTool is currently WIP.

### Spell Check

RPGMTranslate supports spell check out of the box, thanks to Nuspell library.

On information about how to find, install and activate dictionaries, see [Spell Check Settings](./settings.md#spell-check)

Note that Japanese, Chinese etc. are not yet supported, because they require segmentation before spellcheck.

### Plugin & Misc

RPGMTranslate provides built-in linting for a lot of Yanfly plugins tags. As of sixth release candidate, there's a lot of stubs but low amount of actual lints.

Misc lints are all about whitespace, punctuation, tag mismatches and so on. Only whitespace and punctuation ones are implemented as of sixth release candidate. Tag mismatches and similar things require more effort to actually implement them.

### Other options

See [Display Options](./settings.md#display-options).

## Keyboard Shortcuts

- Shift + ArrowDown - Move to next row (down)
- Shift + ArrowUp - Move to previous row (up)
- Shift + ArrowLeft - Move to the left translation column
- Shift + ArrowRight - Move to the right translation column
- Ctrl + ArrowUp - Move to the top-most row
- Ctrl + ArrowDown - Move to the bottom-most row
- Left-click on source text - Copy the source text

## Batch Select

You can select multiple translation cells by clicking one of them and extending the selection by holding Shift and clicking the other one.

Batch copy/cut/paste is supported.

## Batch Text Processing

Access it through the "Tools" button.

[Documentation on batch processing](./batch-processing.md)
