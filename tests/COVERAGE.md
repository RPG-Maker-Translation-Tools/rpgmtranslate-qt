# Test coverage: status, what is left, known gaps

Measured on 2026-11-09 (Linux, Clang, Debug, libgit2 ON, offscreen Qt). How to run the tests and the `coverage` target is in `docs/src/content/docs/development.md`.

| | Lines | Branches | Functions |
|---|---|---|---|
| C++ (files compiled into a test) | 82.07% | 76.69% | 84.07% |
| Rust (`api.rs`, `ffi.rs`, `count.rs`; `librubyfmt` excluded) | 98.30% (`api.rs` 97.30%, `ffi.rs` 98.65%, `count.rs` 100%) | not reported by `llvm-cov` for Rust | 96.70% (regions 96.39%) |

- 40 C++ test programs and 91 Rust unit tests pass. The C++ figure above is from before the tests of the last session (LanguageTool and spell-check lints in `TaskWorker`, the GMock credential tests, the `SettingsWindow` rows and code points, a refused branch switch, a failed commit of a save), so it is a little higher now. `ezgit2.hpp` (14.8%) is in the C++ total and is out of scope, as it is moving to another repository.
- Coverage says a line ran, not that an assertion would catch it breaking. The numbers are a floor for "nothing is untouched", not a measure of how well it is checked.
- The test suite has not had a quality pass (duplicates, vacuous or brittle tests). That is separate from coverage.

## What is covered well

- **Not losing the user's work:** saving, writing, replace, batch, script and purge, including an open tab that cannot be saved (the file stays unchanged, the tab stays open, the edit is kept), backups, and what a replace does to the last newline of a file.
- **Settings and endpoints:** the settings window, endpoints, the key check against a local server, script approval, lints, lint tooltips and the search, replace and glossary logic.
- **Dialogs and menus:** nearly all `.ui` dialogs, the main window's connections and flows, the task worker and the script host.
- **The Rust core through its FFI:** archives, assets, saves, reading, writing, purging, matching, serde import and export, and the Yandex and LanguageTool endpoints against a local server.

## Before "everything important is covered" can be said

In order of importance. Each item says what would close it.

1. ~~**Measure the Rust core.**~~ Done on 2026-11-09 under Linux (WSL), as `cargo llvm-cov` writes no profile data on Windows (gnullvm or MSVC). The figures include the lines of the `mod tests` blocks. 91 Rust tests pass with `snowball`, the serde, highlighting and formatting backends, `languagetool`, `yandex-translate`, `google-translate` and `deepl` on (the last two have no tests of their own, as their libraries hardcode HTTPS). The Japanese, Chinese and Korean tokenizers are not in the run. Without the backend features the tests of `languagetool` and `yandex-translate` are compiled out, and `rpgm_translate_single` and the LanguageTool FFI look uncovered. The remaining uncovered application lines are listed under "Rust" in the table below.
2. **Real game data per engine.** The Rust and C++ tests use hand-built fixtures. By references in the tests, MV/MZ and VX Ace are exercised far more than VX and XP, and RPG Maker 2000/2003 has hand-built database, map, tree map and save fixtures only. Closed when each engine has a small real or realistic project that goes through read, write and purge, and the result is checked against known output.
3. **The updater.** It downloads and installs a program. With an earlier version of `AutoUpdater`, five `tst_mainwindow_updates` tests crashed in `QCoreApplicationPrivate::lockThreadPostEventList`. The explanation is a message box opened from the `updateFailed` slot running a nested event loop that deletes the updater and the reply under the slot. That was never checked in the real application, and the tests now work around it with `inEventLoop`. The install step now has tests for a corrupted and a truncated archive (the old executable stays), and a download cut short by the server fails without delivering data (`tst_autoupdater` passes 18/18 on Windows, 7z archives; the Linux `tar.xz` path was not re-run). Still open: the crash has to be reproduced or ruled out in the running app.
4. **Run on macOS.** macOS was never run, so the macOS branches of `AutoUpdater::install` and the tests that call programs (`sh`, `cat`, `sleep`) are unverified there. On 2026-11-09 the whole suite passed on Debian 13 (WSL, GCC, Debug, 40 C++ programs and the Rust test, with GMock found) after the last changes; on Windows (MSYS2 clang) the programs that were touched passed, not all 40. The tests that make a file read-only fail when run as root, as root ignores permissions: run them as an ordinary user.
5. **Timing.** Several tests wait on fixed times (`QTest::qWait(200)`, `QTRY` with 10 to 15 seconds). They passed four runs in a row on Linux and may flake on a slow or loaded machine. Closed when the suite is run repeatedly under load in CI, and the waits that fail are replaced with conditions.
6. **The failure branches of saving and loading.** An archive that cannot be written (the backup folder is a file) is now tested. The credential store refusing a write, read or delete is tested on the Rust side, through the failure that the mock store can be told to return. What is left is in the table below: Glaze cannot fail on these structs, and the C++ side of a refused credential has no seam.

## Known gaps

### Not covered on purpose

| Area | Why |
|---|---|
| `utilities/ezgit2.hpp` (14.8%) | Moving to another repository. |
| Google and DeepL translation success paths | Their libraries hardcode HTTPS. Covering them needs a TLS stub and a custom client. The Yandex and LanguageTool paths, and the failure paths of all of them, are covered. |
| `AssetMenu/MediaPlayer.cpp`, `GraphicsAssetViewer.cpp`, `AssetPreviewWidget.cpp` | Playback and graphics. Low value for the effort. Only selection and filter logic of the asset menu is tested. |
| `SettingsWindow` theme change | The offscreen platform has no colour scheme. |
| `SettingsWindow` LanguageTool HTTPS timeout hint | Needs a 20 second wait. |
| Lints `.inl` tables | Static regular expressions and tooltip strings. `tst_lints` checks that every part has a category, that tags are found by their own part, and the number of duplicate parts. |

### Not reachable, or not worth it, so far

| Where | What | Why |
|---|---|---|
| `MainWindowSaveLoad.cpp` | Serialization failure, the C++ reaction to a credential-store refusal, a `.part` file that cannot be renamed over the archive | Glaze cannot fail on these structs, and the C++ tests have no way to make the mock store refuse. The rename failure needs the archive path to be a non-empty folder with the timestamp of that second. |
| `MainWindowCallbacks.cpp` 295-302, 552-564 | The guards that stop a replace when the open tab cannot be closed | An unsavable open tab is refused earlier, when the tab is saved before the search (`aReplaceIsRefusedWhenTheOpenTabCannotBeSaved`). These fire only if that save works and the close fails afterwards. |
| `MainWindowCallbacks.cpp`, `MainWindowFunctions.cpp` | The batch wrap notice for a line length hint of 0, the baseline move failure, the `.rvpacker-metadata` open failure, the "Write failed" and "Script failed" notices, early returns of `handleSingleLint` | Small notices and OS failures. The batch wrap notice needs a selected cell in an open tab. |
| `MainWindow.cpp` 159-175 | The retranslate failure box | Needs a missing compiled-in `.qm` resource. |
| `ScriptHost.cpp` (6 lines) | The default working directory of `rpgm.exec`, a process that stops between polls, a null string from the engine | Trivial. |
| `TabPanel.cpp` | `currentChanged` with the same row | Not reachable through the public API. |
| `SourceControlDock/GitFileList.cpp` | One line | No test compiles it. |
| `utilities/Types.hpp` | Four Glaze lambdas of `MatchModeInfo` that are never called, and a metatype id | No logic. |
| Rust `api.rs` 32-44 | Keyring entry creation failure | Needs a credential store that fails. |
| Rust `api.rs` 297, 310, 618, 733 | `BackendNotCompiled` for Yandex and DeepL, and `unreachable!` arms | The first two exist only when those backends are off in a different build, and the rest cannot be reached. |
| Rust `api.rs` 365, 467, 498, 697 | A closing brace, the `M4a` asset type, one `generate_file` error and an `ini` read error | Trivial or rare OS failures. |
| Rust `ffi.rs` 626-647, 668-670, 713, 884 | Logger `enabled` and `flush`, the `lindera` dictionary lookup, an empty import buffer | Trivial. The dictionary lookup needs the `lindera` features. |
| Rust `ffi.rs` 813 | The `set_error` branch of `rpgm_highlight_code` | A highlight failure cannot be triggered. |

## Unverified claims

- **"Copy translation" ended the program when the translation folder was missing** (CHANGELOG, v1.2.1). Confirmed on 2026-11-09: with the throwing `fs::copy` call put back, `aTranslationThatCannotBeCopiedIsReportedInsteadOfEndingTheProgram` fails with an unhandled exception and the test program crashes; with the fix it passes.
- **The duplicate regex parts in the lint tables** (`TextCodes` 4, `Notetags` 6, `PluginCommands` 2, `CommentTags` 0) are called intentional because they belong to plugins that share a tag, and the first copy takes the text. That is the stated reason in `tst_lints`, not something checked against each plugin.

## Production code that exists for the tests

- `AutoUpdater::setReleasesUrl` and `AutoUpdater::install` are seams for the updater tests. `setReleasesUrl` writes a static global without locking, so it is only safe in tests.
- `EndpointSettings::yandexBaseUrl` (C++ and `api.rs`) points the Yandex endpoint at a local server. It is part of the settings file. The settings window keeps it when saving, and the key check uses the value of the selected endpoint.
- `RPGMTRANSLATE_MOCK_KEYRING` swaps the system credential store for an in-memory one.
- `CredentialStore` (`utilities/CredentialStore.hpp`) wraps the Rust credential calls on the settings save path, and `MainWindow::setCredentialStore` replaces it. The GMock tests in `tst_mainwindow_saveload` use it to make the store refuse. They are built only where CMake finds `GTest::gmock` (`mingw-w64-clang-x86_64-gtest` on MSYS2, `libgmock-dev` on Debian and Ubuntu) and are left out otherwise.
- `credential_entry` in `api.rs` is `pub(crate)` so that the FFI tests can make the mock store fail the next call.

## Test traps

- The in-memory credential store lives as long as the test process. A test that saves API keys must delete them again, or the next test loads them (`cleanup()` of `tst_mainwindow_saveload` does it for the accounts it uses).
- The test programs are built with `-mwindows` on Windows: they print nothing to a console, so run them with `-o file.txt,txt` and wait for the process.
- Do not declare nested structs in a `private slots:` section: moc cannot parse it.
- Run instrumented binaries through `ctest` or the `coverage` target, not from the repository root: they write `default.profraw` into the current directory.
- `QTest::mouseMove` without a button does not reach widgets offscreen. Send a synthetic `QMouseEvent` to the viewport.
- A `Status` notice becomes a modal box unless the widget lives in a `QMainWindow` with a status bar, so tests that expect one need a `ModalCatcher`.
- A test that leaves a file read-only must restore it with a scope guard, not at the end of the body. Closing the window saves again, and a failed assertion would leave a box that nobody answers.
- Default auto-replacements in `Settings` are all disabled. Enable them in the test.
