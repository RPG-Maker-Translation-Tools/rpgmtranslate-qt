# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog v2.0.0](https://keepachangelog.com/en/2.0.0/).
This project uses (or at least tries to use) [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Dates are in DD.MM.YYYY format.
Versions may carry a codename alongside the semver number.

## [Unreleased]

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
- Qt is now compiled with proper `-ssl -schannel` configure settings and does not fail trying to access `https` links, which would lead to 301 update error.
- Qt is updated to 6.11.2.
- Quoted all path expansions in the CMake script, which will fix the problems with the paths that contain spaces.
- Added `LUA_EXECUTABLE` CMake option to override the default vendored Lua with user one.
- Added `RUST_PROFILE` CMake option to specify a Rust profile to build Rust part.
- Windows builds are now compiled using LLVM 23.

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
