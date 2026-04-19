# Info

RPGMTranslate is an open source and simple translation tool for RPG Maker games. Among its main features are an exceptional performance and plain text file format.

For advanced usage, refer to:

- [Search Features](search.md)
- [Text Editing](text-editing.md)
    - [Batch Processing](batch-processing.md)
- [CAT features and the glossary](glossary.md)
- [Git client](git.md)
- [Settings](settings.md)
- [Asset inspector](asset-inspector.md)
- [Development](development.md)

**Also, help us improve [the docs](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/tree/main/docs/docs/en)!**

## Installation

Download the latest release from [GitHub](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest).

Right now, program is available as:

- Portable version for Windows (rpgmtranslate.exe).
- Portable version for Linux (rpgmtranslate).

### Read-only directory

If you installed the application into a read-only directory (e.g. /usr/bin), it [won't be able to write its data](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/issues/7). Application is designed to be portable-only.

By default, the application will fall back to `RPGMTRANSLATE_DATA_DIR` if set, and to [`AppLocalDataLocation`](https://doc.qt.io/qt-6/qstandardpaths.html) if not set.

## Interface

When you open the program for the first time, you'll have to open a project using the corresponding button.

Once the project is opened, the whole UI will become available to you. Overall, UI contains:

- Utilities bar at the top with buttons, task progress meter, translation progress meter and game title input.
- Main area with the translation table.
- Status bar at the bottom with temporary and permanent information, such as the last executed action (like backup), opened tab name, and translation progress information.

## Opening a Project

1. Click the "Open Folder" button
2. Select your RPG Maker game folder.
3. If the folder already contains an `.rpgmtranslate` directory, project will be loaded right away. Else, you will be prompted with the dialog to read game files.
4. Project will open, from here you can start your translation.

Note that application doesn't use the source `data` or `Data` directory when translating - instead it tracks a baseline at `.rpgmtranslate/baseline-data`. This is made for consistency, to avoid issues when the source data was updated. Program will check changes between source data at startup, and you can manually run a check using `File > Check for source file changes` entry.

In case of crashes or incorrect behavior, the application provides the log file beside the executable.
