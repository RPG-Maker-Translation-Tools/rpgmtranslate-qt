# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog v2.0.0](https://keepachangelog.com/en/2.0.0/).
This project uses (or at least tries to use) [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Dates are in DD.MM.YYYY format.
Versions may carry a codename alongside the semver number.

## [Unreleased]

## [v1.2.0] "Death of RPG Maker forums" - 18.10.2026

### Added

- **Batch scripts**
  - Scripts can run programs and read/write files (`rpgm.exec`, `rpgm.readFile`, `rpgm.writeFile`, `rpgm.log`); a custom script must be approved by you before it can run.
  - `rpgmFileCallback` is called once per file instead of once per line. `options` now carries `sourceLanguage`, `translationLanguage` and the enabled `glossary` terms. An example translating through OpenRouter and LiteLLM is in `examples/openrouter-litellm.js`.
  - A button opens the selected script in a separate editor window with JavaScript highlighting that follows your edits.
- **Asset inspector**
  - A "Saves" category shown as JSON (MV/MZ `.rpgsave`/`.rmmzsave`, older engines' `Save*`, RPG Maker 2000/2003 `.lsd`); subdirectories appear as tree branches.
  - More listed and playable formats: WAV, `.wma`, `.mid`/`.midi`, `.avi`, `.mpg`, `.wmv`, `.mp4`, `.ogv`. The player has a volume slider.
  - MIDI is played through TinySoundFont and needs a General MIDI SoundFont named `soundfont.sf2` next to the executable (or in `RPGMTRANSLATE_DATA_DIR`); without it, the menu says where to put it.
- **Statistics**
  - "Display words/characters count for a file" shows the counts in the status bar; **File > Project Statistics** counts the whole project.
  - Words are segmented properly (also Japanese, Chinese, Thai). RPG Maker codes are not counted.
- **Search and bookmarks**: rows can show the name of the entry (actor, item, map...) they belong to. Enable "Display entry names" in settings.
- **Text codes**
  - RPG Maker 2000/2003 `\S[n]` (text speed) and `\_` (half-width space), linted like the others. `\_` counts as half a space in the line length hint and Wrap.
  - A number can come from a variable code, as the game allows (`\N[\V[3]]`, `\C[\V[1]]`, `\V[\V[\V[2]]]`). Such a code is one tag for every lint, and the line length hint, Wrap and counts treat it as one code.
- **Tooltips**
  - "Active save" selection in the lint selection menu (all engines). Save-dependent tooltips show its values: party and actor names, item/weapon/armor quantity, playtime, `\V[n]`, `\caseSwitch{n?x:y}`, actor HP/MP/TP, `\map[0]`.
  - New tooltips for `\map[n]`, the Real Time System codes (`\Year`, `\Month`, `\Hour`...), and codes addressing an enemy by troop position (they say it is only known in a battle). A code that reads a variable names it.
  - Note tags, plugin commands and comment tags that point to the database show names (and icons) of actors, classes, skills, items, weapons, armors, states, animations, common events, switches, variables and icons.
  - RPG Maker 2000/2003: `\V[n]`, `\N[n]` and `\C[n]` work (names from `RPG_RT.ldb` in the project's read encoding, colors from the system graphic the database selects). Other tooltips say the data is not available there.
- **Translation cells**: "Trim Cell", "Wrap Cell" and "Latinize Cell" shortcuts (`Ctrl+Shift+T/W/L`) run the built-in scripts on the current cell. Typing in a cell can be undone after them.
- **Line length hint**: "Hint font" and "Hint font size" project settings.
- **Read**: an "Archive Encoding" select sets the codepage of the file names inside the `.rgss` archive, for archives whose names aren't in the Text Encoding. Leave it blank to use the Text Encoding.

### Changed

- **Lint**
  - Lint no longer recompiles the custom lint expression per row. Linting files is faster with a big glossary, since it computes no spelling suggestions it doesn't show.
  - Global lint adds all rows to the table at once. The results table no longer freezes the window while results are added or a column is resized.
  - Project-wide lint and statistics use all CPU cores.
  - A lint tooltip with several references to one game file reads the file once.
- **Glossary**: matching converts each row to UTF-8 once.
- **Search and replace**
  - Results appear without re-reading the searched files, so big searches don't freeze the window, and are listed in tab order. Search reports progress in batches.
  - Replacing in all found cells asks for confirmation and says how many cells and files it covers.
  - Replacing is faster on large projects. "Replace" with no matches no longer closes the open tab; it says nothing was found.
  - "Whole word" works with regular expressions containing `|` and with text starting or ending with a non-word character. Results with an empty match are dropped.
- **Translation table**
  - Uses several times less memory on big files, and editing a cell is lighter.
  - The line length hint is drawn in a stronger color beside too-long lines.
- **Batch scripts**
  - The built-in Wrap script was rewritten. It understands RPG Maker codes (zero width, never split, size codes like `\{` and `\fs[n]` counted), breaks CJK text, keeps existing line breaks, and leaves rows with `<WordWrap>` or needing more than 4 lines alone.
  - Scripts no longer slow the window down while they run on big projects.
- **Asset inspector**
  - The asset menu and preview share one case-insensitive table of extensions, so a listed file can always be opened. Every engine lists the same media, data and font extensions per category; engines differ only in searched directories. RPG Maker 2000/2003 assets are searched in the whole project directory (skipping the program's own `.rpgmtranslate` and `translation`). Movies were previously only looked up in `Movie`.
  - Big data files open without hanging the window: files over 1 MB are shown without syntax highlighting, and searching in them is capped at 5000 matches. BMP images are listed and shown. RPG Maker 2000/2003 projects open the asset list faster.
  - MIDI files start faster after the first one, as the SoundFont stays in memory. Videos larger than the window are scaled down to fit. Clicking the progress bar or using the keys seeks.
- **Read, Purge and Import**
  - Force read, Purge (without the ignore file) and Import ask for confirmation before they rewrite or remove translation files.
  - Read: the title from the ini file has its own encoding select again. "Text Encoding" is shown only for RPG Maker 2000/2003, XP and VX.
  - Read: the width is fixed and the height grows with the descriptions, so long (or translated) text is no longer clipped.
  - Read: "Allow duplicates" is the default and the recommended mode.
- **Tasks**: Lint, Export and Import show real progress in the task list instead of an endless bar.
- **Backups**
  - Backups are compressed in the background, so the window no longer freezes while one is made. The oldest backup is removed only after the new one is written.
  - The backup list is sorted with the newest backup first, and a backup is loaded when the button is released.
- **Projects**
  - Opening a project with a big glossary or many bookmarks is faster.
  - The recent projects list keeps the most recently opened project last.
- **Translation endpoints**
  - "Check key" works for all remaining built-in endpoints.
  - Translation suggestions are requested a moment after you stop on a cell, not for every cell you pass through, and an answer for a cell you already left is dropped. Suggestions from several endpoints are requested at once, so a slow endpoint doesn't delay the others.
- **Settings**: Settings has Save and Cancel buttons. Cancel, Esc and the close button discard the changes, including style, theme and removed endpoints. Before, closing the window always saved them.
- **Hotkeys**
  - The Translations menu hotkey defaults to `Alt+T`. `Ctrl+S` is also Save's, so neither worked, and settings still holding it are reset. Settings rejects hotkeys that are fixed ones (Save, Write, Search).
  - The hotkeys of Copy, Cut, Paste, Undo and Redo can't be assigned to other actions.
- **Updater**: the updater updates an AppImage in place, installs the new executable atomically on Linux, and updates the license files shipped with a release. Without the updater, the setting is disabled and _Check for updates_ says the build can't update itself.
- **Documentation**: the site moved from MkDocs to Astro Starlight (same languages and URLs) and was checked against the program and corrected.

### Removed

- LLM endpoints integration and batch translation.

### Fixed

- **Core**
  - Undefined behavior and hangs: 16-byte file names in the hashes, names longer than 16 bytes, unparsable translation settings or an unknown endpoint number, decrypting an asset without an extension, and entity data requests with an empty or unknown path.
  - Binary RPG Maker 2000/2003 and Marshal game files were handed to the core as text, and non-UTF-8 translation files were exported as such.
  - Every core log message leaked memory, a read printed stray debug output, and the Rust log level was shifted by one (a malformed message crashed task name parsing).
  - Extracting an RGSS archive wrote outside the output folder for entries with absolute or `..` paths. Such entries are now skipped with a warning.
  - Out-of-bounds writes when selecting map files with a map number or count above 2048.
  - An uninitialized read in the script runtime.
  - Archive entries with Japanese names were split into folders in the wrong place when a character has a backslash as its second byte (such as `ソ` or `表` in Shift_JIS). The names are now decoded with the Read menu's encoding before they are split.
  - The "Source files have been updated" dialog lists changed, new and skipped files in a scrollable box, so it no longer overflows the screen when many files changed.
  - Fixed a lot of places where a single button click could be registered twice.
- **Files and saving**
  - Translation files, `maps.txt`, `settings.json`, the glossary, project settings, project metadata, batch scripts, serialization exports, exported custom lints and files written by `rpgm.writeFile` are written to a temporary file that replaces the old one only on success. A failed or interrupted save no longer empties or half-writes them. Serialization errors of settings, glossary and project settings are reported, and a failed commit is shown.
  - Settings, project settings and the glossary keep a `.bak` copy on every save. An unreadable file falls back to it and is kept as `.corrupt`. They are no longer reset when a newer version adds or removes a field (unknown fields are ignored).
  - A failed `.bak` copy no longer deletes the previous backup.
  - Translation files are rewritten (atomically) only if they changed or contain carriage returns, not on every project open or batch operation that changed nothing.
  - Saving no longer drops map sections without text from `maps.txt`.
  - `system.txt`: opening and saving its tab no longer removes the first line (and duplicates the game title). The table leaves out the last line, which holds the title. A title ending with the title marker no longer crashes. The title is written back only to the file it was read from (also `terms`), and no junk line is added when there is no title.
  - A broken spellcheck dictionary and a failed metadata save are reported instead of ignored.
  - Cancelling the folder picker of "Save to custom location" after a failed save no longer writes to the root of the drive. The same error is shown again.
- **Backups**
  - Restoring a damaged backup could destroy the current translation. Entries are now read completely before the file is replaced, and a damaged backup says so instead of silently restoring only part of it. Entries with `../` in their name are skipped, and archive entries that aren't plain files are skipped too.
  - Old backups were never removed (with maximum 1, the wrong directory was deleted). Backups now include unsaved map edits and get proper file names. Loading a backup is no longer undone by the next save.
  - Leftover half-written backup files are removed when the next backup is made.
  - Backup settings changes (period, on/off) apply without reopening the project.
- **Projects**
  - Reusing a translation folder from the project root (made by `rvpacker-txt-rs`): the engine is detected (it was always MV/MZ), source hashes above the signed 64-bit range (written negative by `rvpacker-txt-rs`) are read, and `.rvpacker-metadata` is written with signed hashes so it reads back. An unopenable or unparsable metadata file, an unknown duplicate mode, or a custom line separator, line break or comment prefix is reported instead of ignored.
  - A project with a missing or unusable `project-settings.json` was opened as MV/MZ with no hashes. The engine is now detected from its files, and hashes, read encoding and duplicate mode are restored from `.rvpacker-metadata`. If the engine can't be detected, the project isn't opened and the reason is shown. Other settings (columns, languages...) are still lost.
  - A `project-settings.json` with no data directory recorded is detected again (an unknown engine type or duplicate mode is ignored) instead of crashing. A project with only an `.rgssad`/`.rgss2a`/`.rgss3a` archive and no data directory no longer crashes. File names longer than 16 bytes in the data directory no longer crash either.
  - A `project-settings.json` with a hash name shorter than 4 or longer than 15 characters is refused as unreadable. Such a name could overwrite memory.
  - A data file whose name (without its extension) has 16 or more characters could corrupt memory when the project settings were saved. Its hash is no longer kept, so the file is read again on the next read.
  - Reading a project that already has `project-settings.json` kept its old source hashes. Opening a project packed in `Game.rgssad` lost the `Game.ini` title and showed a "Cannot start Read" warning.
  - The source-files check on open considered non-translatable files. Wrong source/translation languages were shown in settings. "IN-GAME DISPLAYED NAME" rows weren't counted in progress. The completed state of all maps but the last was forgotten on reopen.
  - A baseline directory that can't be created crashed, and failing to move the baseline files of an older project crashed too. "Check source updates" copied files to the baseline even when its Read was refused. All three are fixed.
  - A file with more columns than the project knows no longer corrupts the project settings when read.
  - A project, tab or file in the translation folder with a name that doesn't fit the file name limits is skipped instead of pointing at another file.
  - Closing a project left the previous project's columns in the Batch and Search menus, and left the Read, Purge, Write, Export/Import, settings, statistics and backup-selection windows open. The settings window kept writing to the closed project's settings and crashed (unsaved edits were dropped).
  - Closing a project, or finishing a Read, no longer saves the open tab twice.
  - Reopening after a cancelled or failed first read of a project no longer tries to open it again (and fails); the Read menu's "Ignore" and "Skip obsolete" stay disabled when a project has no translation.
  - `plugins` (MV/MZ) and `terms` (RPG Maker 2000/2003) files are usable in the file selection of search, lint, batch, read and purge.
  - Importing maps (**Export/Import**) is no longer lost when the project reloads.
- **Read, Purge, Import and Export**
  - After cancelling or failing the first read of a new project, the Read menu no longer silently stops working until restart. Closing it during that first read also ends the wait.
  - Purge's "Create .rvpacker-ignore file" option did nothing.
  - "Use title from Game.ini" works in the Read menu of an open project.
  - Export/Import processed unchecked files instead of checked ones. It now asks to select files when none are checked, like Search and Batch. Exporting map files produced empty files.
  - Export to a folder that doesn't exist yet creates it, instead of skipping every file.
- **Tasks**
  - Closing a project, opening another, or quitting during a task could crash or save over the task's work. Abortable tasks (search, replace, scripts, lint, statistics, export, import) are now aborted and waited for. Read, Write, Purge and extraction can't be aborted, so closing or opening is refused with a message and quitting asks first.
  - What a running task would refuse to start is greyed out instead of answered with a message: Write, Search, Batch, Lint, Statistics, cell scripts, Save, Load backup, Check source updates, and the entries of the Read/Purge/Export menu. Their panels are closed, and the source control panel is greyed out.
  - Tabs can't be opened during Read or Purge: the tab panel is disabled, and search results and bookmarks say why. Tabs also can't be opened while a script, replace or import is rewriting the files, so the tab can't be saved over their result. Automatic backups follow the same rule and don't archive mid-task.
  - Settings can't be opened during a task. They are modal, and a translation request works on a copy.
  - A Search, Replace or Script that couldn't start (or whose open tab couldn't be saved) closed the open tab anyway. Replace and batch scripts no longer start if the open tab can't be saved first, so the editor can't overwrite their changes.
  - Edits were lost or overwritten by reloads: Read, Purge and Import results overwrote the editor's older text in the open tab and in maps, and Write, Export and backups didn't see unsaved edits. Edits are now written to disk before a task starts, and the reload doesn't save them again.
  - A running Read, Purge or Write no longer interferes with closing the project.
  - A task that fails with an internal error is reported and no longer blocks other tasks or closing.
  - A possible crash when saving or opening a map tab while a search, lint or script ran on maps.
- **Settings and hotkeys**
  - Settings turned an unset style into the first style of the list, and an unset hint font into the default font (it means "the editor font"), when saved.
  - The endpoint form didn't match its type until the type was changed. It is disabled while no endpoint is selected.
  - A language in the project settings that isn't in the list is kept, instead of being erased when Settings is closed.
  - Quitting with no project open didn't save the settings, so a closed project was reopened on the next start. This is fixed.
- **Translation endpoints and API keys**
  - API keys were lost when the system credential store refused them. They now stay in `settings.json` with a warning.
  - An API key cleared in Settings is removed from the credential store and no longer comes back after a restart. Keys of endpoints that are no longer supported are removed from the store too.
  - A failed translation endpoint shows its error greyed out. Clicking it no longer inserts the error text into the cell.
  - A Script endpoint without a script file is refused in Settings, instead of being removed at the next start.
  - Suggestions with characters like `*` or `]` are shown as they are in the tooltip.
  - A Yandex translation that comes back without text is reported as an error, instead of an empty suggestion.
  - A key that the system credential store refuses to delete is reported with a warning, instead of quietly staying in the store.
- **LanguageTool and spellcheck**
  - The LanguageTool check mixed up replies when started twice, and failed on a URL ending with `/`.
  - A LanguageTool lint underline or row outside the text crashed; both are fixed.
  - Every file ending in "aff" was listed as a spellcheck dictionary.
- **Translation table and cells**
  - Memory corruption when a translation line has more parts than the table has columns (a game text containing `<#>`). Such a line is kept as is, read-only, and adding columns stops at the limit. A line that can't be split into parts is no longer deleted when its tab is saved.
  - A malformed translation-table column rename crashed; fixed.
  - Editor rows don't grow with wrapped lines, and an editor no longer loses its place when its row moves while open (removed row, inserted bookmark).
  - "Insert translation" overwrote the source column and didn't update the view or the translated counter. Inserting a suggestion into a cell you are typing in warns about the typed text, keeps it in the undo history, and no longer asks about the old text.
  - Copy and cut skipped empty cells, which shifted pasted lines onto other rows; cut also cleared read-only empty cells. Copying now keeps empty cells, so pasting them back no longer shifts the text.
  - Pasted Windows line endings are handled. Pasting text that ends with a line break no longer clears the cell below, and pasting into several rows resizes them.
  - Added columns were editable on comment rows, while other cells of rows with fewer columns weren't. The kept column of a selection moved to another row after an insert or remove.
  - A file with more columns than the project knows read past the column list.
  - "Remove Row" did nothing. Progress counters of the tab, status bar and global bar were wrong after a row was removed.
  - Deleting text no longer brings an automatic replacement back (e.g. `...` left after erasing a fourth dot).
  - Numpad Enter finishes editing a cell like the main Enter, instead of adding a line break.
  - "Go to row": the input was placed off the window, crashed on Enter, and could open twice. All fixed. Typing a key the input refuses (a letter, or an eleventh digit) no longer closes it.
  - The table font is correct when the application font has no point size.
  - Whitespace-only cells no longer count as translated when a project is opened.
- **Tab list**
  - The context menu leaked on every open. Progress text was clipped when the translated count grew a digit, and percent mode divided by zero for a tab without text.
  - The right-click menu opens at the cursor. The progress and the "completed" colour update at once instead of when the mouse passes over them.
  - The tab list is re-measured after the application font or style changes.
- **Bookmarks**
  - Bookmarks were saved with name and file swapped: new ones got the tab name as description and no file. Those in maps were filed under `maps` with the row counted over the whole file, so clicking did nothing. They are now filed under their map tab and counted from its start.
  - Clicking in a filtered list opened another bookmark. Editing or removing touched bookmarks of other files with the same row number (and skipped some when removing). A new bookmark wasn't editable until reopening and ignored the active file filter; it now has the same editable cells as a loaded one.
- **Search and replace**
  - Replacing in a row with matches in several translation columns duplicated the row. Replacing in cells with line breaks (and with regex groups, one result at a time) cut text at the wrong places. A replacement ran on text that had changed since the search.
  - Replacing and putting text in a cell didn't update the translated counters, and could put text into a comment row (`<!>...`).
  - Search results freed their matches while in use, which crashed after a search or a single replacement. Crashes on groups that didn't take part in the match, on a column the row doesn't have, and on matches outside the text are fixed.
  - Clicking a result acted on another one when filtered by file. A single replacement crashed the panel or kept the old text and height. Clear freed a result while it was being replaced. "Replace" kept an item marked replaced after failing. The panel highlighted regex groups over the whole match.
  - An invalid regular expression did nothing without a message, and one matching empty text found a match between every two characters.
  - A new search resets the "Filter by file" box, which used to keep a filter that no longer applied to the results. Choosing an action for a result while the results refresh no longer crashes.
  - The file, row and column line of a result with a long unbroken text is no longer cut off.
  - Replace: `\1`, `\2`... point to the right group when a regex group did not take part in the match.
  - A single replacement refuses to overwrite text if the cell was edited after the search, and can't change a wrong result after the panel was refilled.
  - A result stays replaceable when its replacement couldn't start because another task was running. Putting text from a source result into a translation column no longer changes the text the result shows.
  - Replacing the active translation of a result in a row with two or more translation columns could write into another translation column. Results name and replace the column they matched.
- **Lint**
  - Apostrophes (`it's`, `dogs'`) were reported as unclosed or unmatched punctuation.
  - Whitespace lints (leading, trailing, contiguous) found nothing unless the setting listed the space. Space and tab always count now, and the setting adds other characters.
  - Global lint checked comment rows (`<!>ID`, `<!>NAME`, bookmarks) as text. The map display name is linted without its prefix.
  - Custom lints: a pattern with its own `( )` group no longer breaks the other lints or crashes. Settings refuses invalid patterns, and import skips them, instead of silently turning all custom lints off.
  - Linting a cell that was just emptied clears its old results. A lint result that arrives after you typed more is dropped, so underlines and suggestions no longer land on the wrong characters.
  - Typing quickly no longer fills the lint list with duplicate or outdated rows, and closing a project no longer races with a running lint.
  - A refused global lint no longer clears the previous results. A missing `Window` image no longer slows linting or floods the log with one message per color code.
  - Lint panel buttons stopped or ran several times after a reload (file selection wouldn't open, Check ran several lints, files were listed again per project). The lint selection menu kept pointers into the closed project and settings, which crashed it after changing the language or editing custom lints.
  - After the first lint run, the lint menu forgot its file list and the "Lints" popup was empty. Both keep working until the project closes.
  - Clicking a suggestion in a lint tooltip applies it and closes the tooltip. The tooltip's suggestion link no longer applies at a stale position after the text changed.
  - Lint tooltips, misc lint messages and the About text never reached the translation catalog. They are now in the `Lint` and `AboutWindow` contexts (run `lupdate`).
  - During global lint, untranslated lines are skipped instead of reported as tag mismatches.
- **Glossary**
  - Terms in Fuzzy or Both mode were saved as Exact, losing the mode and threshold. The fuzzy threshold and match score are shown with decimals. A threshold typed with `.` became unreadable in `,` locales, and an unreadable one fell to 0 (matching anything) instead of the default. The threshold is kept between 0 and 1.
  - The edit toggle read the wrong row (or crashed) after another entry was deleted. Deleting a term while the lint selection menu is open no longer toggles the wrong term; the menu closes instead.
  - Terms with an empty side were matched by lint. Match highlights were misplaced after characters outside the Basic Multilingual Plane.
  - The glossary lint text no longer runs the match status into the term line.
  - Glossary search ignores spaces around the text. The delete confirmation belongs to the glossary window.
- **Text codes and tooltips**
  - Patterns that never matched or matched only part of their arguments: `\etp%[n]` (was a copy of `\etp[n]`), `\caseSwitch{n?x:y}` and `\caseEval{e?x:y}` (only a literal `s?x:y` matched), `\bust...` codes with an optional duration or delay (`\bustAni[0, 5]`, `\bustMoveTo[1, 400, 300]`, `\bustFadeIn[0]`...), `<Region Triggers: x, y, z>` with any number of regions, and `\lsn<filename>` (matched no file name).
  - `\C[n]` used only the first digit and read past the color table above 31.
  - `\af[n]`, `\msgactor[n]` and `\autoactor[n]` tooltips read a wrong part of the code as the actor number.
  - `\C[n]` color and `\I[n]` icon tooltips never found MV/MZ system graphics in `img/system` (also encrypted, and under `www`).
  - A memory overflow in tooltips for text-code numbers of more than a few digits.
  - A crash on a `lint:` Markdown link in game text. Tooltip links open only web pages now.
  - An icon (`\I[n]`) tooltip crashed the app when the icon sheet (`IconSet`) was a 0-byte or unfinished image. It shows a warning now.
  - Window colors (`\C[n]`) from a `Window` image that can't be read are reported, instead of showing as transparent. They reload when the image is edited.
  - Lint tooltips read names and values from data files with the project's read encoding, so a project in another encoding shows its own text.
- **Batch scripts**
  - A `rpgmLineCallback` returning nothing broke batch scripts.
  - The built-in Trim, Wrap and Latinize scripts wrote to the wrong column (or nowhere). Wrap inserted real newlines, which split the row and corrupted the translation file.
  - Changes to the script you have open are saved with the project. New scripts are saved as `.js` files so they are there at the next start, and renamed scripts no longer leave their old file behind. The first Trim after opening the menu no longer fails with an empty script.
  - The approval of a script that runs programs and touches files can show the script's code ("Show Details").
  - A script that fails or is aborted leaves the file it was working on as it was, and the project is reloaded for the files done. Abort stops a script that is stuck in a loop at once.
  - A script on an unsplittable line or a file with no lines crashed; fixed.
- **Asset inspector and media player**
  - The asset menu tried to decrypt audio that isn't encrypted (anything but MV/MZ's `.ogg_`, `.m4a_`, `.rpgmvo`, `.rpgmvm`), leaked its preview window, and used an unset project pointer before the first project opened.
  - Asset viewer: reading past the end of an encrypted image or of audio shorter than its header; a failed JSON generation left the previous asset on screen under the new name (it is now shown on the page); temp audio files left behind when the next track opened while the player held the previous one; an encrypted `m4a` given an `.ogg` name; fonts viewed earlier stayed registered; the preview window stayed open with its asset after the project closed; a freeze searching with a regex that matches the empty string; search results kept for the next asset.
  - The decrypted copy of an encrypted audio file is removed when the preview window or the project is closed. The font preview no longer shows a search box that searched the previous file.
  - A failed audio stream open crashed; fixed.
  - Formatting a script that contains dprint's ignore comment (`dprint-ignore-file`) crashed the preview. The script is shown unchanged now.
  - Media player: part of the last video frame showed when audio played after a video, and the video wasn't horizontally centered. Picture and sound drifted apart over time; the picture now follows what the audio device has played (or the wall clock for a movie without audio), late frames are dropped, and sound isn't cut off before the end.
  - Media player: the device thread and the GUI decoded the same demuxer at once. The device thread now only reads a lock-free ring that the GUI thread fills half a second ahead, filled before playback starts. Freeing the decoder, packet and file while the device thread still read them is fixed.
  - Media player: a file whose video never decodes froze the window; a tick now reads a bounded number of packets. Seeking a paused clip started the device while the ring and decoder were rewound; the slider now stops the device for the seek. The ring's fill level went wrong after about 4 GiB of audio, and a clip of unknown length gave the progress bar a garbage range.
- **Updater**
  - Aborting the download after it finished no longer crashes.
  - The updated program no longer sometimes fails to start because the previous instance still held the single-instance lock.
  - A failed self-update no longer leaves Windows without an executable. The old one is put back, and a leftover `rpgmtranslate-old.exe` no longer breaks the next update.
  - Installing an update saves the open tab, the project and the settings first, and asks about running tasks like closing the window does.
  - The automatic check for updates at startup no longer shows an error when the network is down. Update errors show their text instead of a number.
  - The updater refuses an archive entry that is a link in place of the program or a notice. The version check no longer downloads the whole releases page.
  - Failed or cancelled downloads close their progress dialog.
- **Localization**
  - Untranslated or reset texts after changing the language: main window actions, the task button and status bar (and the game title replaced with "Game Title" while a project is open), the task panel (task names, abort buttons), the Translations menu title, the bookmark and search panel file filters, the asset menu (search field, categories), the asset preview's buttons and player, the glossary's term cells, the statistics dialog, the lint table headers, the "Custom" checkbox, the Read menu descriptions, the About window, and the settings window's section names, font size label, endpoint description and hotkey names in messages.
  - An unsupported language (the default is the system's) failed to load a translation on every start. English is used then.

### Development

- **Build**
  - `ENABLE_GXHASH` CMake option (on by default). Turn it off to hash with the portable rapidhash instead of gxhash, which needs AES-NI / Armv8 crypto.
  - `ENABLE_UPDATER` CMake option (on by default) controls the self-updater. Off, its code isn't built.
  - The build checks minimum versions of Qt (6.8), FFmpeg (6.1, Ubuntu 24.04's 6.1.1; libavutil 58.29) and Glaze (7.5.0). Fixed the build against FFmpeg 6.1 and Qt older than 6.9.
  - The CMake configure step prints a build summary (build type, MSVC runtime, PIC, options, dependency versions). Source lists use relative paths.
  - `MSVC_RUNTIME` is now applied to `CMAKE_MSVC_RUNTIME_LIBRARY` (it was validated but never passed).
  - The AppImage is built with asset playback, linked against Ubuntu 24.04's FFmpeg 6.1.1.
- **Dependencies**
  - [Windows] Updated `mimalloc` to v3.5.4, downgraded to FFmpeg v8.1.3, upgraded to Qt 6.12.0.
  - Updated `zmij` to commit `538a186` and `fast_float` to v8.3.2. Integers are formatted by `zmij` (built with `zmij-int.cc`) instead of `jeaiii/itoa`, which is no longer vendored.
- **Licensing**
  - The project's LICENSE.md is included in releases.
  - Third-party notice fixes: Qt's Unicode data, smooth image scaling, the Direct3D 12 mipmap shader and Qt Svg's XSVG-derived arc code are attributed; the FreeType credit line is added; FFmpeg is `LGPL-2.1-or-later`, nuspell `LGPL-3.0-or-later`, libgit2 `GPL-2.0-only WITH GCC-exception-2.0` with its current COPYING, libarchive `BSD-2-Clause AND BSD-3-Clause`, TinySoundFont `MIT AND Zlib`, and md4c's current license text. lua (build-time only) is left out.
  - Releases include the missing license texts for md4c, bzip2 and the code bundled inside the static Qt (tinycbor, easing, sha3, rfc6234, D3D12MemoryAllocator).

### From dependencies

- **rvpacker-txt-rs-lib**
  - (v17.0.1) Fixed translated maps being written into the wrong files: an untranslated map section in `maps.txt` was not skipped properly, so the map after it was parsed as part of it, and the translation of map N went to the first untranslated map before it, while map N itself was skipped ([#20](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/issues/20)).
  - (v18.0.0) Corrupt game files no longer cause crashes or undefined behavior when read:
    - Scripts with missing parts or invalid zlib data are reported as invalid.
    - `plugins.js` that isn't `plugins = [...]`, or has a nameless plugin, is reported as invalid.
    - Database objects without an `id` or name are skipped.
    - Malformed translation lines are reported with the line number.
  - (v19.0.0)
    - Source hashes changed, so the first Append after upgrading re-reads every file once.
    - With no Text Encoding set, text is read as UTF-8 instead of being guessed (RPG Maker 2000/2003, XP, VX); text that doesn't decode is reported as an error. Text that can't be encoded in the write encoding fails the file instead of being written as `&#NNNN;`.
    - Trailing `if(...)` on XP/VX/VX Ace text and VX Ace `\et[n]` speaker codes now stay in the source text, so existing translations of such lines no longer match. Append, then translate them again.
    - `Scripts` and `plugins.js` entries are matched to the game's scripts and plugins by name instead of position, so inserting or reordering one no longer shifts the translations after it. `plugins.js` and MV/MZ data files are written back with their original formatting, changing only the translated strings.
    - Append keeps what the game no longer has (translations of removed entries and maps, comments) instead of dropping it, unless "Skip Obsolete" is on.
    - Translation files and game data files are written through a temporary file, so an interrupted write keeps the previous file. A translation file with a byte order mark no longer loses its first section, and one with two sections of the same id is reported.
    - Fixed untranslated event text being erased on write, a map's display name being blanked when it has no translation, the last message box of every XP event list being skipped, and a map or entry whose only text is shared with another one (duplicates removed) being left untranslated.
    - The game title in `system.txt` is written back for engines that keep it in the system file.
    - RPG Maker 2000/2003: hero names and titles set by events are extracted; message boxes holding only symbols are not; skipped maps/events and the ignore file work; append no longer drops translations of unchanged maps or merges two consecutive message boxes.
    - Ignore file: `.rvpacker-ignore` is no longer rewritten outside a purge, `?` in a pattern matches one character, and RM2K terms entries match when duplicates are removed.
    - Export/Import: CSV export prefixes cells a spreadsheet would read as a formula with `'` (import removes it), a trailing carriage return survives, and malformed CSV/XLSX/XML rows are reported. Fixed an XML export of a comment containing `]]>`.
    - Files are processed in parallel, and writing skips maps without a translation. Malformed event commands in game files are skipped instead of crashing.
- **rpgm-asset-decrypter-lib**
  - (v4.0.0) Truncated or malformed OGG files report an error instead of crashing. M4A key detection no longer underflows when a box name is in the first chunk. M4A and OGG key detection no longer leak state between files, so decrypting one asset can't affect the next.
  - (v5.0.0) M4A files with no box after the type box report an error instead of being decrypted with an assumed 28-byte header.
- **rpgm-archive-decrypter-lib**
  - (v6.0.0) Archives are fully validated on extraction: empty or truncated archives, and archives with out-of-range sizes or offsets, report an error instead of crashing or reading out of bounds. VX Ace archives whose entry contents overlap each other or the entry table, or aren't in table order, are rejected as corrupt.
  - (v7.0.0) Extracting archives is faster. Archives with more than 1,048,576 entries are rejected with an error.
- **language-tokenizer**
  - (v0.4.2) Fixed swapped dash/quote punctuation normalization.

### Planned for 1.3

- Remote support for built-in git client.
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
- Wolf RPG support.
- VisuStella support (if needed).
- Translator++ project porting.
- Finally add extensive GIFs and illustrations to the documentation.

## [v1.1.4] "Death of RPG Maker forums" - 16.09.2026

v1.1.3 was faulty after updating to mimalloc v3.5.2, so this release is identical to v1.1.3, but with changed deps.

### Development

- Downgraded `mimalloc` to v3.5.1.
- Updated `glaze` to v8.4.0.
- Updated `fast_float` to v8.3.0.
- Fixed CI failing with enchantum header.

## [v1.1.3] "Death of RPG Maker forums" - 16.09.2026

### Added

- When LMB is pressed on a source cell, its text is now copied.

### Changes

- Several changes to translation input (cells) behavior:
  - Enter now does not insert a new line. Shift+Enter does, and Enter closes the input.
  - Shift+Enter now inserts a new line.
  - Ctrl+Enter jumps to the next row.
  - When a new line is inserted to the translation input, it is not scrolled to the bottom, which would lead to the first line in the cell being not visible.
- Batch copy/cut/paste behavior changes:
  - Non-contiguous cells now can be selected. Text can be copied from non-adjacent cells, and inserted in non-adjacent cells in the same pattern. Read more in the [documentation](https://rpg-maker-translation-tools.github.io/rpgmtranslate-qt/text-editing/#batch-select).
  - Comment cells are now ignored when copying/cutting/pasting.
  - Cells from different columns now can be selected simultaneously, but not from the same rows.
- Slightly optimized copying text from translation cells and translation table performance overall.
- Improved visibility for selected cells in the translation table.

### Fixes

- Fixed crash introduced in v1.1.2 that would result in crash when right-clicking a translation table's row (and possibly in other parts of the application).
- Rows are now properly resized when something is cut/pasted from/into them.
- Fixed inconsistent usage of incorrect application data directory throughout the code.
- Really fixed auto-updater, I guess.
- Translation table header is now properly updated when the header of the column is changed.

### Development

- Application now uses `enchantum` instead of `magic_enum`.

### Planned for 1.2

- Total source/translation words/characters information.
- Display name of the entries where text is located when, for example, searching.
- Check which plugins need to be finished/improved in linting, check whether everything is compatible with VisuStella plugins.
- WAV playback for RM2K.
- Save file asset inspection.
- RPG Maker 2000/2003 also seems to have its own text tags, which are currently not supported.

### Planned for 1.3

- Remote support for built-in git client.
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
- Wolf RPG support.
- VisuStella support (if needed).
- Translator++ project porting.

## [v1.1.2] "Death of RPG Maker forums" - 11.09.2026

### Fixes

- Fixed auto-update failing with "Failed to extract archive" on Windows.
- Fixed wrong parsing of icon sets which would result in fails to display icons corresponding to `\i[x]` tags.
- Fixed linting being entirely disabled by default.
- Translation inputs (cells) are now not scrolled to the very bottom when opened, which previously led to the first line in the cell being not visible when the input is focused.
- Fixed possible fail of copying data as a baseline in some cases.
- Project metadata (`.rvpacker-metadata`) will now be automatically deduced when using existing translation in the root of the project.
- Small micro-optimizations.

### Development

- Rust version that was used during compilation is now displayed in about window.
- Fixed incorrect mimalloc version in about window.

### Planned for 1.2

- Total source/translation words/characters information.
- Display name of the entries where text is located when, for example, searching.
- Check which plugins need to be finished/improved in linting, check whether everything is compatible with VisuStella plugins.
- WAV playback for RM2K.
- Save file asset inspection.
- RPG Maker 2000/2003 also seems to have its own text tags, which are currently not supported.

### Planned for 1.3

- Remote support for built-in git client.
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
- Wolf RPG support.
- VisuStella support (if needed).

## [v1.1.1] "Death of RPG Maker forums" - 04.09.2026

### Added

- `mimalloc` library version is now displayed in about window.
- Added a database button. It is disabled now and does nothing. It will be implemented when the database system is implemented.

### Changed

- Published an updated documentation.
- `js/plugins.js` file is now copied as a baseline alongside plain data files.
- Status bar now explicitly says after write that the write output can be found at `.rpgmtranslate/output`.
- Entries in the assets menu are now sorted by filename.
- `baseline-data` is now NOT a plain storage of RPG Maker data files. It now contains `data` subdirectory (for data files) and `js` directory (MV/MZ-only, stores `plugins.js`). Backward compatibility is preserved.

### Fixes

- For MV/MZ games, fixed the error about `js/plugins.js` file on write.
- Fixed path for copying updated sources as a new baseline that would not account for RPG Maker 2000/2003 files.
- Fixed using the baseline data files for re-reads from the rvpacker menu, which would effectively result in no effect.

### Development

- Fixed CI not adding release artifacts to the GitHub releases automatically.
- Added `.gitattributes` file to keep everything LF.

### Planned for 1.2

- Total source/translation words/characters information.
- Display name of the entries where text is located when, for example, searching.
- Check which plugins need to be finished/improved in linting, check whether everything is compatible with VisuStella plugins.
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
- WAV playback for RM2K.
- Save file asset inspection.

### Planned for 1.3

- Remote support for built-in git client.
- Wolf RPG support.
- VisuStella support (if needed).

## [v1.1.0] "Death of RPG Maker forums" - 01.09.2026

Most of the work for this release was done outside the `rpgmtranslate-qt`.

### Added

- RPG Maker 2000/2003 support.
- Since RPG Maker 2000/2003/XP/VX do not support Unicode, the following was added:
  - Added a text encoding select when reading.
  - Added a "Write Encoding" option in settings.
- Simplified Chinese translation [#17](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/pull/17), thanks @grill-glitch

### Changed

- Updated to `rvpacker-txt-rs-lib` v16.2.0:
  - **RPG Maker 2000/2003 support.** Detected from `.ldb`/`.lmt`/`.lmu` extensions. New `core::Base` methods for RM2K: `begin_rm2k_maps`/`process_rm2k_map`/`finish_rm2k_maps`, and one `process_rm2k_*` per entity kind (`actors`, `skills`, `items`, `enemies`, `troops`, `classes`, `commonevents`, `states`, `switches`, `variables`, `terms`).
  - **Text encoding is now a deliberate, two-sided setting.** `Base::set_read_encoding`/`Processor::read_encoding` forces decoding of the game's own source text to a specific codepage instead of guessing - needed for RM2K, XP and VX, none of which carry any encoding indicator at all (previously this always fell back to guessing across UTF-8/Windows-1252/Windows-1251/Shift-JIS/GB18030). VX Ace doesn't need this in practice: its Marshal format tags most strings with their real declared encoding already, and that's trusted over any guess or override.
    `Base::set_write_encoding`/`Processor::write_encoding` is the independent write-side counterpart, and defaults to always writing UTF-8 rather than whatever the source declared since a translation generally isn't representable in the source game's own codepage (Shift_JIS has no Cyrillic to translate a Russian translation into), and forcing it doesn't fail loudly: an unmappable character is silently replaced with a literal `&#1055;`-style numeric reference. Override `write_encoding` only when the target engine has no Unicode-aware renderer (true of RM2K, XP and VX) and the translation's own script actually fits the codepage you choose. See the README's "Text encoding" section for the full mechanics, including how a player needs to match their system locale (or use a locale emulator) to see non-UTF-8 output correctly.
    `get_ini_title` also got a documented use as a heuristic for finding an XP/VX project's read encoding in the first place: `Game.ini`'s title is the one field in that file that can carry non-ASCII bytes, and when it does, RPG Maker's editor wrote them in the original developer's own codepage.
  - **New `core::entity` module.** `get_entity_values` resolves a dotted key/index path inside a raw, unprocessed RPG Maker data file (any engine) to the scalar value(s) at the end of it, for callers that just want one field (e.g. a tooltip) without running extraction.
  - **`marshal-rs` updated to v3.** Internals migrated from an owned `marshal_rs::Value` tree to v3's arena/cursor API. JSON layouts that the `json` module produces and consumes are different now - this is the one breaking change in this release.
  - Examples are actually useful now.
  - Added `get_ini_title_rm2k` for extracting game's title from `RPG_RT.ini` files.
  - `Processor` now properly hashes RM2K files.
  - Map events option now works for parsing map event metadata for RM2K files.
  - Removed processing of RM2K switches and variables which have no meaningful text for translation.
  - Added RM2K methods to `json` module.
  - Game title is now written to `terms` files for RM2K if supplied.
  - Improved performance by copying less in the code.
- Changed default glossary hotkey to `Alt+L`.
- Removed language selector from settings, it's already available from the menu bar.

### Fixes

- Fixed bogus entries in the about window.
- Fixed incorrect RPGMTranslate version in the about window.
- A single hotkey now cannot be used in multiple places, settings will explicitly reject it.
- VX's "Areas" file is not counted towards source file check against baseline and now won't trigger a "Source files have been updated" popup.
- Fixed 301 update error on Windows, check development section.
- Fixed "add new row" button's "+" icon missing in the translation table.
- Mimalloc now properly overrides everything in Windows builds.

### Development

- Rust side now calls into C++-provided allocator, so the allocation logic is unified and if there's an allocator overriding the default allocator, Rust can use it without changes.
- Changed CI to automatically add artifacts to the published releases.
- Fixed Russian docs.
- Quoted all path expansions in the CMake script, which will fix the problems with the paths that contain spaces.
- Added `LUA_EXECUTABLE` CMake option to override the default vendored Lua with user one.
- Added `RUST_PROFILE` CMake option to specify a Rust profile to build Rust part.
- [Windows] Qt is now compiled with proper `-ssl -schannel` configure settings and does not fail trying to access `https` links, which would lead to 301 update error.
- [Windows] Qt is updated to 6.11.2.
- [Windows] Builds are now compiled using LLVM 23.

### Planned for 1.2

- Total source/translation words/characters information.
- Display name of the entries where text is located when, for example, searching.
- Remote support for built-in git client.
- Check which plugins need to be finished/improved in linting, check whether everything is compatible with VisuStella plugins.
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
- WAV playback for RM2K.
- Save file asset inspection.

### Planned for the future

- Wolf RPG support.
- VisuStella support (if needed).

## [v1.0.1] "Death of RPG Maker forums" - 24.08.2026

### Changed

- Updated to `rvpacker-txt-rs-lib` v15: See [its release notes](https://github.com/RPG-Maker-Translation-Tools/rvpacker-txt-rs-lib/releases/tag/v15.0.0).

### Development

- Fixed the unability to compile the applications with some features disabled because the code was unguarded.
- Added a lot more features to CMakeLists so that the builds are more modular.
- Linux builds in Docker are now compiled with `x86-64-v3` baseline.
- Adjusted documentation.
- Default Linux/AppImage builds are now provided through GitHub Actions CI.

## [v1.0.0] "Death of RPG Maker forums" - 21.08.2026

### Added

- Support and linting for Yanfly and co plugins. Information: <https://rpg-maker-translation-tools.github.io/rpgmtranslate-qt/text-editing/#plugin-misc>.
- Implemented lint tooltips that fire when the lints are hovered. Tooltips contain useful information about the tag.
- A couple of new lints, like tag mismatch, togglable spellcheck/glossary check.
- Implemented special coloring for tags such as \C[x] or \hc[rrggbb].
- Implemented local features to Git client, no remote interactions yet.
- Implemented LanguageTool support. Using LanguageTool paid API is not tested, only the local server.
- About window now provides the information about release's build environment (compiler, STL, SDK, date), as well as versions of key bundled libraries (e.g. Nuspell).
- The running task (like search) now can be aborted using a button near the task text.
- The application now warns you if a translation file contains lines that couldn't be parsed, instead of silently skipping them.
- Export/Import: translation files can now be converted to and from CSV, XLSX, XML, JSON, and YAML, from a new "Export/Import" entry in the `rv` button's menu.
- The asset inspector's "Beautify" button now also formats JS and Ruby scripts (via `dprint-plugin-typescript` and `librubyfmt`), not just JSON.

### Changed

- Updated to `rvpacker-txt-rs-lib` v14.0.0:
  - Files are now sorted before reading, this means that they will always go in right order (e.g. actors is first, weapons is last, map001-map999 sequentially).
  - Removed romanize feature since it carried 0 real importance - "romanizing" (or more correctly, latinizing) the text should be performed by the downstream users of the library, and should only affect the translation - not the source text.
  - Implemented `serde` module for exporting text to different structured formats and importing them back. Such formats include CSV, XLSX, YAML, JSON, XML.
- Default color for the tooltips is now gold.
- Enabled lints, their appearance and behavior are customizable through settings.
- Text on-the-fly replacements (e.g. turning `<<` into `«`) are now customizable through settings.
- Batch menu now operates on JavaScript code to perform batch actions. To implement this, quickjs-ng was used. It aims to support the latest ECMAScript specifications so you're not constrained when writing scripts.
  - You can create your own scripts! Documentation on how it works in rpgmtranslate-qt can be found here: <https://rpg-maker-translation-tools.github.io/rpgmtranslate-qt/batch-processing/>.
- Micro-optimized different parts of the program.
- Tracking the current task in the top panel is now more descriptive.
- Batch menu now has two selectable actions, "Translate" and "Script". "Script" has three built-in scripts, including trim and wrap from before, and a new one latinize script.
- Source/translation language selects were reworked to now display the whole spectre of human languages. Previously, these inputs only included languages supported by the stemming Snowball backend. If the language is not supported by [Snowball](https://snowballstem.org/algorithms/) (or LSTM segmentation), it will not support stemming and will notify the user about it.
- Term match menu was reworked to be a general menu for lints. The table was simplified to show general lints about tag mismatches, punctuation mismatches, misspellings, term mismatches and anything else.
- Enabled lints and other translation checks are now toggled in the new lint menu. It allows you to toggle lints right on the spot, however lints are still configured in the settings.
- Logging is massively improved, it should now cover more parts of the application and be better overall.
- Each task now runs in its own dedicated thread, which allows you to run multiple tasks simultaneously.
- API keys (endpoints, LanguageTool) are no longer stored in `settings.json`. They now live in the OS credential store (Windows Credential Manager / Linux Secret Service) and are only read back into memory when needed.

### Fixes

- Git client/tab panel are now hidden before opening a project.
- Git client/tab panel are now hidden and git client is properly reset when closing a project.
- Fixed problems possibly related to hashing due to hashing garbage past the null terminator.
- Fixed pulling wrong hashes from the project settings, which would cause the application to show a popup stating that files have been updated, when in reality they weren't.
- Fixed showing menus at wrong y positions on Windows.
- Fixed wrong translation column / translation endpoint values in batch menu.
- Fixed broken select file menus, that were broken for some reason. Or maybe that's a regression that never was shipped in release candidates.
- Translation table now scrolls horizontally per-pixel which is more convenient.
- The toolbar "Write" action no longer shows a spurious "written successfully" message after the write actually failed.
- Settings window no longer writes a stale endpoint index after that endpoint was just removed.
- The zoom-percent label in the asset inspector now actually shows the `%` sign.
- Zoom-to-fit in the asset inspector (on opening an image or updating the font sample) no longer computes against a stale viewport size.
- Fixed a copy-paste bug where the Mistral endpoint was silently sent using Moonshot's API preset for single-cell translation.

### Development

- Migrated configure scripts to Lua, and started distributing minimal Lua Windows/Linux builds to run the configure scripts. You can see the modified sources of Lua here: <https://github.com/savannstm/lua>.
- JSON parsing/writing now uses Glaze which is more maintainable.
- Refined flags used to build the project on Windows - it should be more optimized and less error-prone. I also trimmed down the built copy of ICU data, so the raw executable now takes up less space.
  - Something has corrupted binaries after compressing them with UPX which we used for every other release and the application would crash at start - #12. The problem was about compressed exports. After repacking with `--compress-exports=0`, the issue disappeared, so now our compressed distributed binaries are **less than 20MB**!
- We're now shipping licenses for all libraries that we link statically. Licenses can be found in `licenses` directory, and about window now contains information about such libraries. This is our legal obligation, but I also decided to include the libraries that don't require attribution.
- Added `prepare-release.lua`, a release-packaging script that verifies every bundled library has documented license text, generates the third-party notice, UPX-compresses the built binary, and assembles the final release archive.
- Docs (settings, git, batch processing, glossary, text editing) were updated to reflect the current state of the program and its features.
- Official releases now target x86-64-v3 baseline (instead of previous tuning for Sandy Bridge architecture) - starting from Intel Haswell (2013) and AMD Excavator (2015). This allows to use AVX2, FMA and BMI to massively improve performance. If you need a build that supports older processors, you'll have to build from source.
- Default application JS scripts can now be found in `scripts` directory.
- Replaced the hand-rolled tree-sitter highlighting pipeline (manual token buffers, incremental `QSyntaxHighlighter`, per-token color mapping) with [lumis](https://github.com/leandrocp/lumis): the Rust side now hands back HTML directly, and the asset inspector's code viewer just loads it.
- Fixed a long-standing typo in the CMake option for JS syntax highlighting that meant it was never actually being enabled, regardless of its cached value.
- Windows releases now use `mimalloc` as the default allocator.

### Planned

- Finishing all features is planned for the next two releases: 1.1 and 1.2.
- Remote support for built-in git client.
- More supported plugins linting (including VisuStella someday)!
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
