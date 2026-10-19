---
title: "Installation"
---

Builds are published on the [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases) page as portable archives. Linux builds (archive and AppImage) are built by GitHub workflows when a version is tagged; the Windows build is uploaded by hand.

## Requirements

**CPU.** Every published release targets **x86-64-v3**, which needs AVX, AVX2, BMI1, BMI2, F16C, FMA,
LZCNT and MOVBE. In practice that means Intel from Haswell (2013) or AMD from Excavator (2015)
onwards. Older hardware has to [build from source](development.md) with a lower `-march`.

**Operating system.** Windows releases need Windows 10 or 11. Linux will run as long as you have libstdc++14/libc++23 or higher on your system.

## Updates

RPGMTranslate checks for a new release on startup (toggle this in [Settings > Core](settings.md#updates)), or on demand via Help > Check for Updates. If a newer version exists, you're asked to confirm before it downloads and installs anything - nothing happens automatically in the background. The update replaces the executable (or the AppImage) and the license files that come with it.

Builds made with the self-updater turned off (`ENABLE_UPDATER=OFF`, e.g. by a distribution package) have the setting disabled, and Help > Check for Updates only tells you that the build can't update itself.

### Releases

Executables are compressed with:

```bash
upx --best --lzma --brute --ultra-brute --compress-exports=0
```

## Runtime data files

RPGMTranslate is portable. By default it keeps its data in the directory of the executable:

| File / directory | Purpose |
| ----------------------- | --------------------------------------------------------------------------------------------- |
| `settings.json` | The [settings file](settings.md) |
| `rpgmtranslate.log` | Log file for the current session |
| `scripts/` | Your custom [batch scripts](batch-processing.md#custom-scripts) |
| `dictionaries/` | Spell check dictionaries you provide, see [Spell check](settings.md#spell-check) |
| `lindera-dictionaries/` | CJK matching dictionaries you provide, see [CJK text matching](settings.md#cjk-text-matching) |

If you want to play MIDI files in the [asset inspector](asset-inspector.md#midi), you also need to provide a `soundfont.sf2` file in the same place. The application does not create or ship it.

The single-instance lock, `rpgmtranslate.lock`, is kept in the system's temporary directory instead. While one instance is running, starting another one does nothing.

Set the **`RPGMTRANSLATE_DATA_DIR`** environment variable to move the data somewhere else:

```bash
RPGMTRANSLATE_DATA_DIR=/home/chuck-norris/.config/rpgmtranslate rpgmtranslate
```

This matters when the executable lives somewhere unwritable - under `/usr/local/bin`, or in `Program Files`. If the variable is not set and the executable's directory is not writable, the application falls back to the system's per-user application data directory (`%LOCALAPPDATA%\rpg-maker-translation-tools\rpgmtranslate` on Windows, `~/.local/share/rpg-maker-translation-tools/rpgmtranslate` on Linux).

If you are packaging RPGMTranslate for a distribution, set `RPGMTRANSLATE_DATA_DIR` globally, or patch the default in `main()` in `src/main.cpp`.

## Licenses

Windows static releases bundle the licenses of all statically linked libraries, including transitive Rust dependencies, in a `licenses` directory alongside the release.

`THIRD-PARTY-NOTICE.md` helps you find a given license.

On other platforms, only licenses of the in-tree header-only libraries are supplied.
