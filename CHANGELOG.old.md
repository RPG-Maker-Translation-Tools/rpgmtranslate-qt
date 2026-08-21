## v1.0.0-rc.6

This release polishes asset inspector and fixes a couple of crucial bugs. And, as well, brings two major changes: baseline data tracking and RPG Maker plugin tags/sequences linting.

Note that tags and sequences linting requires a lot of work. This release brings only basic support, and the next release candidate will complete this feature.

### Changes

- Updated to `rvpacker-txt-rs-lib` v13.0.0:
    - Made `core::parse_rpgm_file` function public.
    - Remove `core::get_engine_extension` function.
    - Changed how hashing works: Now program operates on 64-bit hash outputs of `gxhash` instead of 128-bit ones, and accepts/returns hashes as iterators over filename-hash pairs.
    - Added `from_filename`, `to_str` and `extension` methods for `EngineType`.
    - Added `Display` impl for `EngineType`.
- Added reasoning effort to translation settings.
- The program now generates `.rvpacker-metadata` file for compatibility with `rvpacker-txt-rs`.
- Translations menu now has fixed size, but each translation label is now scrollable along with the menu view.
- Fixes:
    - The previously selected model is now restored in settings. If this model is no longer in the list of models, provided by the endpoint, user will be notified.
    - Small fixes for media player.
    - Fixed writing wrong settings to project settings on the first read.
    - Closing a project will always trigger a save now.
    - Closing a project won't cause it to load on the second startup now.
- Backups:
    - Backups are now LZMA2-compressed to save space.
    - Numbers in backup file names, like days, months, hours etc. are now padded with leading zero.
    - Added `File > Load Backup` button to restore the project to a specific backup.
- Linting:
    - Support and linting for a lot of Yanfly plugins - still WIP.
    - Support and linting for ATS Message Options plugin - still WIP.
    - Support for custom sequences and tags - still WIP.
    - Separate options for highlighting trailing/leading/sequential whitespace.
    - Option for detecting tag/sequence mismatches between source and translation text - still WIP.
    - Linting is highly customizable through settings - still WIP.
- Stability:
    - When the directory where the program is located not writable, it will try to find other directory for the application data. Users can manually specify `RPGMTRANSLATE_DATA_DIR` variable to store the data in desired place. Otherwise, data will be put in the local data directory - [see paths for `AppLocalDataLocation` here](https://doc.qt.io/qt-6/qstandardpaths.html).
    - Reduced possible errors due to improper memory handling when doing FFI calls.
    - New baseline data tracking system:
        - On the first read, `data`/`Data` directory will be copied to `.rpgmtranslate/baseline-data`.
        - All program operations will use `baseline-data` from this point.
        - On startup, the program will check, whether the files in the original data directory changed.
        - If those files have changed, it will suggest to append the new text to the translation files.
        - You can always trigger the check manually through `File > Check for source file changes`.
    - A lot of micro-optimizations.
    - Changed the standard hasher from `rapidhash` to `gxhash` which proves to be much faster.
- Asset inspector:
    - Asset inspector now processes files with both lowercase and uppercase extensions.
    - Implemented searching text when inspecting code assets.
    - It's now possible to refresh the list of assets for any new assets.
    - Implemented the ability to change sample text and font size when inspecting font assets.
    - Added display of current scaling (in percents) when inspecting image assets.
    - Added image scale slider to image viewer in case you don't have a mouse wheel.
- Development:
    - Removed `rapidhash` dependency. `gxhash` is invoked from Rust instead, and it's much faster.
    - Added `magic_enum.hpp`, `miniaudio.h` and new `jeaiii_to_text.h`, `zmij.h` and `zmij.cc` files directly to the source tree.
    - Changed hashing to use unsigned 64-bit integers instead of 128-bit integers. This improves compatibility.

## v1.0.0-rc.5

This release candidate introduces asset inspector support. Next release candidate will further improve user experience and correct some parts of the program. Git client will be implemented in the final v1.0.0 release.

### Changes

- Updated to `rvpacker-txt-rs-lib` v12.1.0:
    - Fix EOF parsing bug: flush final translation section in initialize_translation by @CreepsoOff
    - Changed `generate_file` function in `json` module to also accept filename argument, and have a special case for `Scripts` files.
    - Added `set_game_title` argument to `Reader` for compatibility with RPG Maker XP/VX/VXAce. This allows user to manually decode game title from Game.ini file and pass it here as UTF-8, since system file not always contains game title.
    - Documentation fixes.
- Fixed wrong extraction of encrypted archives which resulted in an unreadable project.
- When reading RPG Maker XP/VX/VXAce projects, read menu will now have option to use the game title from Game.ini file. Since Game.ini is not necessarily UTF-8 encoded, this allows user to manually find the encoding and use correct game title.
- Fixed possible panics on read.
- Added information about libarchive, libgit2 and FFmpeg to about window.
- Fixed clipping text in tab panel items.
- Massively improved documentation.
- Implemented asset inspector: currently supports browsing through images, audio, video, scripts, and inspecting each of those, along with media player, syntax highligthing and more.

## v1.0.0-rc.4

A couple more fixes.

### Changes

- Fixed outputting none/not all files when writing.
- Fixed possible crash when applying batch translation. Still requires more testing.
- Fixed absolutely idiotic issue, where the application would batch translate all maps, that end with the number of the selected map. For example, if map1 is selected, application would try to batch translate all other maps, that end with 1 (e.g. map11, map21, map31 etc.).
- Fixed possible crash when getting error while opening/reading the project or aborting the read.
- To avoid losing project settings and glossary in result of program abort, each backup will also save project settings and glossary.
- To avoid losing settings in result of program abort, settings will be saved when closing settings window.

## v1.0.0-rc.3

A couple of fixes.

### Changes

- Fixed undefined behavior when processing hashes on read which would lead to unexpected side effects, such as wrong engine being recognized.
- Fixed base URL validation in settings.
- Fixed outputting write results to `.rpgmtranslate/.rpgmtranslate/output` instead of `.rpgmtranslate/output`.
- Fixed possible panic when tinkering with options in settings window.
- Changed some checkboxes in settings window to show "custom" label instead of "enabled".
- Added description for endpoint list.
- Added description of different endpoint types.

## v1.0.0-rc.2

The second release candidate implements some scratches for the future features, like git client and asset inspector, along with a couple of fixes. It's expected to be the last release candidate before the final release.

### Changes

- Updated to `rvpacker-txt-rs-lib` v11.2.0.
- Fixed read menu not showing itself when trying to open an unparsed project, which would effectively lead to a complete unability to open a new project using the program.
- Fixed possible deadlock on program startup if `.rpgmtranslate` directory disappears in the saved project.
- Fixed possible empty translation files when parsing the project.
- Fixed leading slash (`/`) in status bar notifications about backups.
- Implemented dictionaries support.
- Fixed multiple translation lints overwriting each other.
- Fixed not shifting row indices in the bookmark menu, when a new bookmark is added.
- Implemented built-in git client.
- Implemented built-in asset inspector.
- Allowed to move dock widgets around.
- Reimplemented tab panel as a dock widget.
- Improved the look of tab panel.
- Added "Locate project directory" button next to the game title.
- Made API key, Yandex folder ID and base URL inputs' contents hidden by default.
- Translation settings rewrite.
- Translations menu overhaul, not complete though.
- Implemented a little bit more of documentation. Not yet finished.
- Overall polishing.

### Note

Git client and asset inspector are not yet fully implemented and usable.

### Coming next

- Replace bare labels in translations menu to scroll areas.
- Finished implementation for git client and asset inspector.
- LanguageTool support;
- More linting (syntax highlighting for Yanfly Message Core, more than two spaces etc.)

## v1.0.0-rc.1

The first release candidate of the rewrite of the original project in C++.

### Changes

- Overall, improved user experience with the application.
- Added tracking of the currently executing tasks, such as batch actions, search and replace.
- No more temporary files. `maps.txt` is parsed to sections and stored in-memory for the duration of the project, while matches are tightly packed into a memory-efficient way, and stored in-memory as long as they're displayed in the search panel.
- Added stubs for LanguageTool and spell check, but those aren't currently implemented.
