# Asset Inspector

Open the asset inspector with the ![](./assets/stock_media.svg) button. It lets you browse and preview your project's assets without leaving the application.

## Finding an asset

Assets are grouped in a tree by category - Audio, Data, Images, Icons, Fonts, Movies, JS - click any entry to preview it. The filter box above the tree narrows it down by filename as you type, which matters once a project has hundreds of files in one category.

## Image Viewer

Supports both encrypted (`.png_`, `.rpgmvp`) and decrypted (`.png`, `.jpg`) images - no need to decrypt anything yourself first.

**Controls:** scroll to zoom, left-click and drag to pan.

## Code Viewer

For JSON, JavaScript, and Ruby files (the engine's data files and scripts). Syntax highlighting is automatic - just open the file.

For JSON specifically, a **Beautify** button appears in the toolbar to reformat the raw contents into something readable. JavaScript/Ruby beautification isn't implemented yet.

## Media Player

Plays both audio and video, encrypted (`.ogg_`, `.rpgmvo`, `.m4a_`, `.rpgmvm`) or decrypted (`.ogg`, `.m4a`, `.mp4`, `.webm`) - again, no manual decryption needed.

**Controls:** play/pause, and a seekable progress bar - drag it and release to jump to that point.

## Font Viewer

Shows sample text rendered in the selected font, plus a list of the writing systems it covers. Check this before assuming a font supports your target language - a font that looks fine for Latin text may be missing the glyphs your translation actually needs.
