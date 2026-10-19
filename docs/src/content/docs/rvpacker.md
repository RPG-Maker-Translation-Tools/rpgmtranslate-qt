---
title: "Rvpacker features"
---

`rvpacker-txt-rs-lib` is responsible for reading the text from RPG Maker games and writing it back. It has four parts.

## Read

Reading decrypts the source archive if needed, parses the game's data files, and produces the plain-text files you edit.

On first opening a project, a Read options dialog appears before anything is parsed; configure it and click Apply. Later, re-parse the project from the "Read" item of the `rv` button's menu, for example after the game was updated. It opens the same dialog:

| Option                  | What it does                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Read Mode               | **Default** parses the game text; it is only used for the first read, and can't be picked once the project has translation files. **Force** re-parses everything and **rewrites the existing translation files, discarding your translation**. **Append** (the default for a manual re-read) adds newly-found text without discarding your existing translation, but does nothing if the source files are unchanged since the last read; **Force Append** appends anyway. In the Append modes, Duplicate Mode and Text Encoding are locked to the values of the first read. |
| Duplicate Mode          | **Allow Duplicates** (the default and the recommended mode) keeps every occurrence separately, so each has its own translation. **Remove Duplicates** collapses repeated lines across maps and events. System, scripts, and plugin files always behave as if Allow is set, regardless of this option.                                                                                                                                                                                                                                                                       |
| Text Encoding           | Decodes the game text in the given codepage. UTF-8 by default. RPG Maker 2000/2003 have no encoding of their own and use the system's codepage, so set it for them. XP and VX are generally UTF-8 (RGSS sets `$KCODE` to UTF-8), but a game _might probably_ override that: change this only if the text comes out garbled. Text that doesn't decode is reported as an error. If the game's title below has native, non-ASCII characters, it _may_ help pinpoint the right encoding.                                                                                        |
| Archive Encoding        | Decodes the file names inside the RGSS archive (XP, VX, VX Ace), which is extracted when the project is first opened. UTF-8 by default. Shown only for those engines, and only on the first read of a project that still has the archive (not when its data directory already exists).                                                                                                                                                                                                                                                                                      |
| Ignore                  | Skips entries listed in a `.rvpacker-ignore` file (see [Purge](#purge)).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| Skip Obsolete           | Drops entries from the previous read that no longer exist in the source, instead of keeping them around.                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| Parse Map Events        | Also extracts map event metadata - event ID, name, and X/Y position - alongside the dialogue text.                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| Use title from ini file | For RPG Maker 2000/2003/XP/VX/VX Ace, the game title isn't always in the system file. Enable this to pull it from `Game.ini` (XP/VX/VX Ace) or `RPG_RT.ini` (2000/2003), whichever is present. The title has its own encoding select; if the title shown under it comes out garbled, the encoding is wrong.                                                                                                                                                                                                                                                                 |

Force Read asks for confirmation before it rewrites the translation files. A file-select button in the dialog lets you scope a manual re-read to the files you check. Map files are always re-read, whether they're checked or not.

## Write

Write (the ![](./assets/manufacturing.svg) button, or `Ctrl+W`) writes your current translation of every file back into game files, ready to run. Output goes into `PROJECT_FOLDER/.rpgmtranslate/output`; copy its contents into the game directory.

For RPG Maker 2000/2003, the write text encoding matters; the other engines are UTF-8. See [settings](./settings.md#project).

## Purge

Purge gets rid of untranslated lines, and can optionally create a `.rvpacker-ignore` file so those lines don't reappear the next time the project is read with "Ignore" enabled. It has its own file select menu (as with Read, map files are always purged), asks for confirmation before it removes anything, and reopens the project when it finishes.

## Export/Import

This converts your translation files to and from CSV, XLSX, XML, JSON, or YAML - useful for editing in a spreadsheet, sending to someone who doesn't want to run the application, or archiving a snapshot outside the app's own format.

Pick a format, pick a folder (destination for Export, source for Import), check the files to include in the file-select menu, then Export or Import. Each translation file becomes one file of the chosen format in that folder, named the same way with a different extension (e.g. `actors.txt` -> `actors.csv`) - Import expects the same naming back.

If no files are checked, you are asked to select some. Export creates the folder if it doesn't exist yet. **Import overwrites the corresponding translation file's content** and asks for confirmation first, so make sure the folder you're importing from actually has what you want.
