---
title: "Text Editing"
---

Opening a ![](./assets/menu.svg) tab shows a translation table, where you edit translation cells like in a regular text editor.

## Files and columns

The ![](./assets/menu.svg) tab panel lists the project's files with their translation progress. Right-click a file to mark it as completed (or unmark it), or to switch the progress between a translated/total count and percents.

A file can have several translation columns. Click the "+" button in the table header to add one, and double-click a translation column's header to rename it. The source column can't be renamed or edited.

Right-click a row for "Remove Row" and "Bookmark Row". "Go to row" (`Ctrl+G` by default) jumps to a row by its number.

## Machine Translation

If you configured [endpoints](./settings.md#endpoints), every one of them is used to translate the source text of a translation cell when you start editing it. This requires the source and translation languages to be set in [Settings > Project](./settings.md#project). Results show up in the ![](./assets/translate.svg) translations menu - click one to insert it into the cell you're editing (you're asked first if the cell isn't empty).

## Bookmarks

Right-click a row and pick "Bookmark Row" to save your place. Bookmarks are listed in the ![](./assets/bookmarks.svg) bookmark menu, grouped by file - click one to jump straight back to that row.

## Linting

Lints highlight the translation cell while it is focused (e.g. text color, background, wavy line), and are listed in the lint menu.

Lint menu is available through ![](./assets/lab_profile.svg) button. It provides:

- "Disable linting" - a global switch that overrides every setting below it.
- Individual checkboxes for whitespace/punctuation/tag-mismatch/spellcheck/LanguageTool/glossary lints.
- "Lints" - opens a menu to toggle individual plugin/tag lints and custom lints on and off.
- A file-select button, to pick the files for the check below. No file is selected by default.
- "Global Check" - runs every enabled lint over the selected files as a background task, not just the cell you're editing. Results (file, line, source, translation, lint info) are listed in the menu's table; the clear button empties it.

Hovering a lint shows a tooltip. For text codes (`\C[x]`, `\I[n]`, `\N[n]`, ...), note tags, plugin commands and comment tags, it resolves the referenced data where possible (actor, item or skill name, switch or variable name, icon, color swatch), and some offer a clickable fix. Codes of every engine are recognized, including plugin codes, so `\N[1]` shows the first actor's name in any engine. Data the application lacks is reported in the tooltip rather than failing; for example, a code addressing an enemy by troop position only resolves in battle.

A number inside a code may be given by a variable code, as the game allows (`\N[\V[3]]`). The whole thing is treated as one code, and its tooltip says which variable the number comes from.

### Active save

Codes that depend on a running game (party names, item counts, playtime, variable values) need a save picked in the "Active save" dropdown of the "Lints" menu. With "None", such tooltips show only what the game data knows.

### LanguageTool

With [LanguageTool](./settings.md#languagetool) configured, it lints the translation text.

### Spell Check

Spell check works out of the box, via the Nuspell library.

For finding, installing and activating dictionaries, see [Spell Check Settings](./settings.md#spell-check).

CJK is not supported yet.

### Glossary

With the [Glossary](./glossary.md) configured, term consistency and term mismatches are checked in the source and translation text.

### Plugin & Misc

RPGMTranslate provides built-in linting for the codes and tags of RPG Maker itself and of popular plugins (such as the ones described on Yanfly's wiki pages), as well as some misc lints not related to the plugins.

The list changes over time, so it is not duplicated here. The recognized codes and tags are in the sources:

- `src/Lints/CommentTags.inl`
- `src/Lints/Notetags.inl`
- `src/Lints/PluginCommands.inl`
- `src/Lints/TextCodes.inl`

Plugin lints are toggled in the menu opened with the "Lints" button.

Misc lints:

- Whitespace
  - Leading whitespace - adds a lint to the lint menu, highlights leading whitespace.
  - Trailing whitespace - adds a lint to the lint menu, highlights trailing whitespace.
  - Contiguous whitespace - adds a lint to the lint menu, highlights contiguous whitespace.
  - Which characters count as whitespace for these three lints is configurable, see [Translation > Lints settings](./settings.md#lints).
- Unclosed punctuation - adds a lint to the lint menu, highlights unclosed punctuation, such as unclosed quotes.
- Source/translation tag mismatch - adds a lint to the lint menu when source and translation tags differ, e.g. the translation lacks a tag present in the source.

## Keyboard Shortcuts

While editing a cell:

- Enter or Esc - Finish editing the cell
- Shift + Enter - Insert a new line
- Ctrl + Enter - Finish editing and move to the cell below
- Backspace right after a [sequence replacement](./settings.md#lints) - Undo that replacement

Moving between cells:

- Shift + ↓ - Move to next row (down)
- Shift + ↑ - Move to previous row (up)
- Shift + ← - Move to the left translation column
- Shift + → - Move to the right translation column
- Ctrl + ↑ - Move to the top-most row
- Ctrl + ↓ - Move to the bottom-most row
- Left-click on source text - Copy the source text

Opening panels, and running the built-in Trim, Wrap and Latinize scripts on the current cell, have their own configurable shortcuts - see [Settings > Controls](./settings.md#controls).

## Batch Select

Copy, cut and paste several translation cells at once: move a whole column, or sync similar translations across files.

You can select multiple cells by clicking some of them with Shift (contiguous) or Ctrl (non-contiguous). Selecting cells from different columns is supported, but not from the same row.

Paste: if only one cell is selected, copied cells are pasted repeatedly until the copied cells are used up. Otherwise they are pasted only into the selected cells.

## Batch Text Processing

Access it through the ![](./assets/construction.svg) button.

[Documentation on batch processing](./batch-processing.md)
