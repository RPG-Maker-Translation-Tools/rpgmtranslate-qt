# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog v2.0.0](https://keepachangelog.com/en/2.0.0/).
This project uses (or at least tries to use) [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Dates are in DD.MM.YYYY format.
Versions may carry a codename alongside the semver number.

## [Unreleased]

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

### Planned

- Finishing all features is planned for the next two releases: 1.1 and 1.2.
- Remote support for built-in git client.
- More supported plugins linting (including VisuStella someday)!
- Built-in database like in RPG Maker to search for specific entries and quickly find anything by searching.
