---
title: "Asset Inspector"
---

Open the asset inspector with the ![](./assets/stock_media.svg) button. It lets you browse and preview your project's assets without leaving the application.

## Finding an asset

Assets are grouped in a tree by category - Audio, Data, Images, Icons, Fonts, Movies, Saves, JS - click any entry to preview it. Subdirectories (such as `img/characters`) are branches of their category. The filter box narrows the tree by filename as you type, which helps once a category holds hundreds of files.

Every engine uses the same categories and recognizes the same kinds of files; the engines differ only in where the files are looked for. RPG Maker 2000/2003 has no dedicated asset directories, so the whole project directory is searched (the application's own `.rpgmtranslate` and `translation` directories are skipped). Everything that is listed can be opened.

Every preview has a **Locate file** button, which opens the file's directory in your file manager, and an **Open in default app** button.

## Image Viewer

PNG, JPEG and BMP images, encrypted (MV/MZ) or plain - no need to decrypt anything yourself first.

## Code Viewer

For JSON and JavaScript files, and for the engines' binary data and save files (`.rxdata`, `.rvdata`, `.rvdata2`, `.lmu`, `.ldb`, `.lsd`...), which are shown as readable JSON. The `Scripts` file of RPG Maker XP/VX/VX Ace is shown as Ruby code. Syntax highlighting is automatic, except for files over 1 MB, which are shown without it.

A **Beautify** button in the toolbar reformats the contents into something readable: JSON always, JavaScript and Ruby in builds with `ENABLE_JS_FORMATTING`/`ENABLE_RUBY_FORMATTING` (both on by default).

The search box finds text in the file, optionally case-sensitively, by whole words or by a regular expression, and steps through the matches with the arrow buttons. A search lists at most 5000 matches.

## Media Player

Plays audio and video, encrypted or plain. Click the progress bar or use the keys to seek, and use the volume slider for audio. Videos larger than the window are scaled down to fit.

Playable formats depend on the FFmpeg the application was built with. Official Windows releases use a suitable FFmpeg build; the scripts that build it, with the exact list of enabled decoders and demuxers, are in [this repository](https://github.com/savannstm/personal) (`dev/build-dev-libraries`). If a listed file cannot be played, its format is probably not in the build.

### MIDI

Older RPG Maker and Wolf RPG games often use MIDI music. A MIDI file holds only notes, so a general MIDI SoundFont is needed to make sound. None is shipped: place one as `soundfont.sf2` in the application data directory (by default, next to the executable; see [Installation](installation.md#runtime-data-files)). Without it, opening a MIDI file shows the exact expected path.

[GeneralUser GS](https://www.schristiancollins.com/generaluser) is a good free general-purpose choice. Older games were made for Windows and use its proprietary default SoundFont, so any other SoundFont **won't** sound exactly like the game. The SoundFont only affects MIDI playback in the inspector, not the game.

## Font Viewer

Shows sample text rendered in the selected font, plus a list of the writing systems it covers. Type your own sample text and change its size in the toolbar. Check this before assuming a font supports your language: one that looks fine for Latin text may lack the glyphs your translation needs.
