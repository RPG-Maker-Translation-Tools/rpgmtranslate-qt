# Rvpacker features

`rvpacker-txt-rs-lib` is responsible for reading the text from RPG Maker games and writing it back. It has four parts.

## Read

Reading decrypts the source archive if needed, parses the game's data files, and produces the plain-text files you edit.

The first time you open a new project, a Read options dialog pops up automatically before anything is parsed - you have to configure it and click Apply to proceed. Later on, you can trigger another read manually from the `rv` button (choose "Read" from its menu) to re-parse the project, for example after the game was updated and the source files changed. This opens the same dialog:

| Option                  | What it does                                                                                                                                                                                                                                                                                                                                                                                                 |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Read Mode               | **Default** does nothing if the source files are unchanged since the last read - use **Force** to re-read anyway. **Append**/**Force Append** add newly-found text without discarding your existing translation.                                                                                                                                                                                             |
| Duplicate Mode          | **Remove Duplicates** (recommended) collapses repeated lines across maps and events. **Allow Duplicates** keeps every occurrence separately. System, scripts, and plugin files always behave as if Allow is set, regardless of this option.                                                                                                                                                                  |
| Text Encoding           | Forces text decoding to the given codepage, instead of guessing it. Needed for RPG Maker 2000/2003, XP and VX - these are legacy engines having no encoding of their own, defaulting to the system's codepage (unlike VX Ace, MV and MZ). Leave blank to guess. If the game's title below has native, non-ASCII characters, it can help pinpoint the right encoding.                                         |
| Ignore                  | Skips entries listed in a `.rvpacker-ignore` file (see [Purge](#purge)).                                                                                                                                                                                                                                                                                                                                     |
| Skip Obsolete           | Drops entries from the previous read that no longer exist in the source, instead of keeping them around.                                                                                                                                                                                                                                                                                                     |
| Parse Map Events        | Also extracts map event metadata - event ID, name, and X/Y position - alongside the dialogue text.                                                                                                                                                                                                                                                                                                           |
| Use title from ini file | For RPG Maker 2000/2003/XP/VX/VX Ace, the game title isn't always in the system file. Enable this to pull it from `Game.ini` (XP/VX/VX Ace) or `RPG_RT.ini` (2000/2003), whichever is present. The title is decoded with the Text Encoding field above - if it comes out garbled, that's a hint the encoding is wrong, and if it has native, non-ASCII characters, that can help you pin down the right one. |

A file-select button in the dialog lets you scope a manual re-read to specific files instead of the whole project.

## Write

Write is accessible through the ![](./assets/manufacturing.svg) button. It writes your current translation back into game files, ready to run.

For 2000/20003/XP/VX engines, there's a very important write setting - text encoding. Read [settings](./settings.md#project) for more info.

## Purge

Purge gets rid of untranslated lines, and can optionally create a `.rvpacker-ignore` file so those lines don't reappear the next time the project is read.

## Export/Import

This converts your translation files to and from CSV, XLSX, XML, JSON, or YAML - useful for editing in a spreadsheet, sending to someone who doesn't want to run the application, or archiving a snapshot outside the app's own format.

Pick a format, pick a folder (destination for Export, source for Import), select which files to include with the file-select button, then Export or Import. Each translation file becomes one file of the chosen format in that folder, named the same way with a different extension (e.g. `actors.txt` -> `actors.csv`) - Import expects the same naming back.

**Import overwrites the corresponding translation file's content**, so make sure the folder you're importing from actually has what you want before running it.
