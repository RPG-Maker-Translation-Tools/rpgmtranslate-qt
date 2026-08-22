# Installation

Builds are published by hand on the [Releases](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases) page, in form of portable archive bundles.

## Requirements

**CPU.** Every published release targets **x86-64-v3**, which needs AVX, AVX2, BMI1, BMI2, F16C, FMA,
LZCNT and MOVBE. In practice that means Intel from Haswell (2013) or AMD from Excavator (2015)
onwards. Older hardware has to [build from source](development.md) with a lower `-march`.

**Operating system.** Windows releases need Windows 10 or 11. Linux will run as long as you have libstdc++14/libc++23 or higher on your system.

## Updates

RPGMTranslate checks for a new release on startup (toggle this in [Settings > Core](settings.md#updates)), or on demand via Help > Check for Updates. If a newer version exists, you're asked to confirm before it downloads and installs anything - nothing happens automatically in the background.

### Releases

Executables are compressed with:

```bash
upx --best --lzma --brute --ultra-brute --compress-exports=0
```

## Release layout

### Windows

#### Runtime data files

RPGMTranslate is portable. By default it writes three files next to the executable:

| File                 | Purpose                          |
| -------------------- | -------------------------------- |
| `settings.json`      | The [settings file](settings.md) |
| `rpgmtranslate.lock` | Single-instance lock             |
| `rpgmtranslate.log`  | Log file for the current session |

Set the **`RPGMTRANSLATE_DATA_DIR`** environment variable to move all three somewhere else:

```bash
RPGMTRANSLATE_DATA_DIR=/home/chuck-norris/.config/rpgmtranslate rpgmtranslate-qt
```

This matters when the executable lives somewhere unwritable - under `/usr/local/bin`, or in `Program Files`. If you are packaging RPGMTranslate for a distribution, set `RPGMTRANSLATE_DATA_DIR` globally, or patch the default in `resolveDataDir()` in `src/utilities/Utils.cpp`.

### Licenses

Windows static releases bundle licenses of all libraries linked statically (including Rust dependencies, transitive dependencies) in `licenses` directory alongside the release.

`THIRD-PARTY-NOTICE.md` alongside the release is supplied to help you navigate to the desired license (if you ever want to see it).

On other platforms, only licenses of the in-tree header-only libraries are supplied.
