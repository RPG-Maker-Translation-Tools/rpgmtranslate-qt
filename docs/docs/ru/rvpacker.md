# Rvpacker features

`rvpacker-txt-rs` is responsible for reading the text from RPG Maker games and writing it back. It has four parts.

## Read

Reading decrypts the source archive if needed, parses the game's data files, and produces the plain-text files you edit.

The first time you open a new project, a Read options dialog pops up automatically before anything is parsed - you have to configure it and click Apply to proceed. Later on, you can trigger another read manually from the `rv` button (choose "Read" from its menu) to re-parse the project, for example after the source files changed. This opens the same dialog:

| Option | What it does |
| --- | --- |
| Read Mode | **Default** does nothing if the source files are unchanged since the last read - use **Force** to re-read anyway. **Append**/**Force Append** add newly-found text without discarding your existing translation. |
| Duplicate Mode | **Remove Duplicates** (recommended) collapses repeated lines across maps and events. **Allow Duplicates** keeps every occurrence separately. System, scripts, and plugin files always behave as if Allow is set, regardless of this option. |
| Trim | Strips leading/trailing whitespace from extracted strings. Only use this if you're sure it won't change behavior in-game. |
| Disable Custom Processing | Turns off special-cased handling built in for specific games - currently *LISA: The Painful* (and derivatives) and *Fear & Hunger 2: Termina*. |
| Ignore | Skips entries listed in a `.rvpacker-ignore` file (see [Purge](#purge)). |
| Skip Obsolete | Drops entries from the previous read that no longer exist in the source, instead of keeping them around. |
| Parse Map Events | Also extracts map event metadata - event ID, name, and X/Y position - alongside the dialogue text. |
| Use title from Game.ini | For RPG Maker XP/VX/VX Ace, the game title isn't always in the system file. Enable this to pull it from `Game.ini` instead, and pick the right text encoding for it. |

A file-select button in the dialog lets you scope a manual re-read to specific files instead of the whole project.

## Write

Write is accessible through the ![](./assets/manufacturing.svg) button. It writes your current translation back into game files, ready to run.

## Purge

Purge is accessible through the `rv` button (choose "Purge" from its menu). It gets rid of untranslated lines, and can optionally create a `.rvpacker-ignore` file so those lines don't reappear the next time the project is read.

## Export/Import

Accessible through the `rv` button (choose "Export/Import" from its menu). This converts your translation files to and from CSV, XLSX, XML, JSON, or YAML - useful for editing in a spreadsheet, sending to someone who doesn't want to run the application, or archiving a snapshot outside the app's own format.

Pick a format, pick a folder (destination for Export, source for Import), select which files to include with the file-select button, then Export or Import. Each translation file becomes one file of the chosen format in that folder, named the same way with a different extension (e.g. `actors.txt` &rarr; `actors.csv`) - Import expects the same naming back.

**Import overwrites the corresponding translation file's content**, so make sure the folder you're importing from actually has what you want before running it.

