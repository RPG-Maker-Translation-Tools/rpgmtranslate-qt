#include "AssetMenu.hpp"
#ifdef ENABLE_UPDATER
#include "AutoUpdater.hpp"
#endif
#include "BackupSelectDialog.hpp"
#include "BatchMenu.hpp"
#include "BookmarkMenu.hpp"
#include "Constants.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "PurgeMenu.hpp"
#include "ReadMenu.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "SerdeMenu.hpp"
#include "SettingsWindow.hpp"
#include "ScrollableMessageBox.hpp"
#include "StatisticsDialog.hpp"
#include "TabListModel.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "WriteMenu.hpp"
#include "config.h"
#include "glazemeta.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QVersionNumber>
#include <QtConcurrent>
#include <cstring>
#ifdef ENABLE_UPDATER
#include <archive.h>
#include <archive_entry.h>
#endif

namespace {
constexpr i32 MAX_RECENT_PROJECTS = 10;

// Whether rvpacker_txt_rs_lib reads and hashes this file, i.e. can appear in `hashes`: the engine's data files,
// `MapNNN`, and RPG Maker 2000/2003's `RPG_RT.ldb`. Other files in the data directory are ignored.
[[nodiscard]] auto isTrackedSourceFile(const QStringView filename) -> bool {
    static constexpr array<QStringView, 12> STANDARD_FILES = {
        u"actors",  u"armors", u"classes", u"commonevents", u"enemies", u"items",
        u"scripts", u"skills", u"states",  u"system",       u"troops",  u"weapons",
    };

    if (filename.compare(u"RPG_RT.ldb", Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QStringView stem = filename.sliced(0, max<isize>(filename.lastIndexOf(u'.'), 0));

    // The stem has to fit a FilenameArray with its terminator.
    if (stem.size() >= scast<isize>(sizeof(FilenameArray))) {
        return false;
    }

    if (ranges::any_of(STANDARD_FILES, [stem](const QStringView name) -> bool {
        return stem.compare(name, Qt::CaseInsensitive) == 0;
    })) {
        return true;
    }

    const QStringView digits = stem.sliced(min<isize>(3, stem.size()));

    return stem.startsWith(u"map", Qt::CaseInsensitive) && !digits.isEmpty() &&
           ranges::all_of(digits, [](const QChar chr) -> bool { return chr.isDigit(); });
}

// The tab and worker APIs address a file by its name as a `FilenameArray` of Latin-1 characters, so a longer name or
// one with other characters would point at another file.
[[nodiscard]] auto isUsableTabName(const QStringView basename) -> bool {
    return basename.size() < scast<isize>(sizeof(FilenameArray)) &&
           ranges::all_of(basename, [](const QChar chr) -> bool { return chr.unicode() <= 0x7F; });
}

// Takes the hashes `rpgm_*` returns: a `u32` count, then (`FilenameArray`, `u64`) pairs back to back. Frees the buffer.
// Copied out with `memcpy`, as the pairs are not aligned to their types.
[[nodiscard]] auto takeHashes(const ByteBuffer buffer) -> HashMap<FilenameArray, u64> {
    HashMap<FilenameArray, u64> hashes;

    u32 count;
    memcpy(&count, buffer.ptr, sizeof(count));
    hashes.reserve(count);

    for (u32 cursor = sizeof(count); cursor < buffer.len; cursor += sizeof(FilenameArray) + sizeof(u64)) {
        FilenameArray filename;
        u64 hash;
        memcpy(&filename, buffer.ptr + cursor, sizeof(filename));
        memcpy(&hash, buffer.ptr + cursor + sizeof(filename), sizeof(hash));
        hashes.insert({ filename, hash });
    }

    rpgm_buffer_free(buffer);

    return hashes;
}

// Where the game keeps its data files: at the root for RPG Maker 2000/2003, otherwise in `data` or `Data`. A game that
// has neither (only an RGSS archive, which unpacks into `Data`) gets `Data`, so the result is never `None`.
[[nodiscard]] auto detectSourceDirectory(const QString& folder) -> SourceDirectory {
    if (QFile::exists(folder % u"/RPG_RT.ldb"_qsv)) {
        return SourceDirectory::Root;
    }

    // On a case-insensitive file system `Data` is found as `data` too, as it always was
    return QFile::exists(folder % u"/data"_qsv) ? SourceDirectory::LowercaseData : SourceDirectory::UppercaseData;
}

// A settings file written by another version, or edited by hand, can hold a value that the enums don't have, and the
// code that switches on them takes it as impossible.
[[nodiscard]] auto hasKnownValues(const ProjectSettings& settings) -> bool {
    return settings.engineType <= EngineType::RM2K && settings.duplicateMode <= DuplicateMode::Remove;
}

// Whether the translation uses a separator, line break or comment prefix other than the defaults. The editor hardcodes
// the defaults (`SEPARATOR`, `NEW_LINE`, `COMMENT_PREFIX`), so it can't edit such a translation.
[[nodiscard]] auto hasCustomFormat(const Metadata& metadata) -> bool {
    const auto differs = [](const optional<string>& value, const QAnyStringView standard) -> bool {
        return value.has_value() && !value->empty() && QAnyStringView(*value) != standard;
    };

    return differs(metadata.lineSeparator, SEPARATOR) || differs(metadata.lineBreak, NEW_LINE) ||
           differs(metadata.commentPrefix, COMMENT_PREFIX);
}
}  // namespace

void MainWindow::copyToBaseline(const QString& sourcePath, const QString& baselinePath) {
    const string baselineDataPath = (baselinePath + u"/data"_qsv).toStdString();

    const auto copyLcfFiles = [&](const QString& sourceDir, const QString& destDir) -> void {
        const QStringList filters = { u"*.lmu"_s, u"*.lmt"_s, u"*.ldb"_s };
        const auto files = QDirListing(sourceDir, filters, QDirListing::IteratorFlag::FilesOnly);

        for (const auto fileInfo : files) {
            const QString destPath = QDir(destDir).filePath(fileInfo.fileName());

            fs::copy(
                fileInfo.absoluteFilePath().toStdString(),
                destPath.toStdString(),
                fs::copy_options::overwrite_existing
            );
        }
    };

    try {
        fs::create_directories(baselineDataPath);

        if (projectSettings->engineType == EngineType::RM2K) {
            copyLcfFiles(sourcePath, baselinePath + u"/data"_qsv);
        } else {
            fs::copy(
                sourcePath.toStdString(),
                baselineDataPath,
                fs::copy_options::recursive | fs::copy_options::overwrite_existing
            );

            if (projectSettings->engineType == EngineType::MVMZ) {
                const string baselineJsDir = (baselinePath + u"/js"_qsv).toStdString();
                fs::create_directory(baselineJsDir);

                fs::copy(
                    (projectSettings->projectPath + u"/js/plugins.js"_qsv).toStdString(),
                    (baselinePath + u"/js/plugins.js"_qsv).toStdString(),
                    fs::copy_options::overwrite_existing
                );
            }
        }
    } catch (const fs::filesystem_error& error) {
        present(
            this,
            NOTICE(
                "Failed to copy %1 to %2 as a baseline data: %3. The original source data from the root will be used instead.",
                Warning,
                Modal,
                sourcePath,
                baselinePath,
                QUtf8SV(error.what())
            )
        );
    }
}

void MainWindow::checkForUpdates(const bool manual) {
#ifdef ENABLE_UPDATER
    if (!settings->core.checkForAppUpdates && !manual) {
        return;
    }

    auto* const updater = new AutoUpdater(this);

    connect(updater, &AutoUpdater::updateDownloaded, this, [this, updater](const QByteArray& archiveData) -> void {
        updater->deleteLater();

        // The application quits when the update is installed, so what it would save on exit is saved first
        if (!prepareToQuit()) {
            return;
        }

        const QString appImage = AutoUpdater::appImagePath();
        const auto installTarget = QFileInfo(appImage.isEmpty() ? qApp->applicationFilePath() : appImage);
        const QString appDir = installTarget.absolutePath();

#ifdef Q_OS_WINDOWS
        const QString exePath = installTarget.absoluteFilePath();
        const QString oldExePath = appDir + u"/rpgmtranslate-old.exe";

        // Leftover from a previous update; QFile::rename refuses to overwrite it
        QFile::remove(oldExePath);

        if (!QFile::rename(exePath, oldExePath)) {
            present(this, NOTICE("Failed to prepare the update: could not rename %1.", Warning, Modal, exePath));
            return;
        }

        const auto restoreExe = [&exePath, &oldExePath] -> void {
            // Drop whatever was partially extracted, rename can't overwrite
            QFile::remove(exePath);
            QFile::rename(oldExePath, exePath);
        };
#else
        const auto restoreExe = [] -> void {};
#endif

        archive* const archive_ = archive_read_new();
        archive* const disk = archive_write_disk_new();

        const auto cleanup = [archive_, disk] -> void {
            archive_read_close(archive_);
            archive_read_free(archive_);
            archive_write_close(disk);
            archive_write_free(disk);
        };

#ifdef Q_OS_WINDOWS
        archive_read_support_format_7zip(archive_);
        archive_read_support_filter_none(archive_);
#elifdef Q_OS_LINUX
        archive_read_support_format_tar(archive_);
        archive_read_support_filter_xz(archive_);
        archive_read_support_filter_lzma(archive_);
#elifdef Q_OS_MACOS
        archive_read_support_format_zip(archive_);
#endif

        // UNLINK: an existing license file is replaced instead of being written into
        archive_write_disk_set_options(
            disk,
            ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_UNLINK
        );

        if (archive_read_open_memory(archive_, archiveData.constData(), scast<usize>(archiveData.size())) !=
            ARCHIVE_OK) {
            qWarning().noquote() << u"libarchive failed to open archive: %1"_qsv.arg(
                svtostr(QUtf8SV(archive_error_string(archive_)))
            );

            present(this, NOTICE("Failed to open update archive.", Warning, Modal));

            cleanup();
            restoreExe();
            return;
        }

#ifdef Q_OS_WINDOWS
        static constexpr string_view targetEntry = "rpgmtranslate.exe";
#else
        const string_view targetEntry = appImage.isEmpty() ? "rpgmtranslate" : "RPGMTranslate-x86_64.AppImage";
#endif
#ifdef Q_OS_WINDOWS
        // The running executable was renamed away above, so its path is free
        const QByteArray outputPath = installTarget.absoluteFilePath().toUtf8();
#else
        // Written next to the target and renamed over it afterwards, so a failed extraction never leaves a broken
        // executable behind
        const QByteArray outputPath = installTarget.absoluteFilePath().toUtf8() + ".new";
#endif
        const QByteArray appDirUtf8 = appDir.toUtf8();

        // Writes the entry the reader is positioned on to the path already set on it
        const auto extractEntry = [archive_, disk](archive_entry* const entry) -> bool {
            if (archive_write_header(disk, entry) != ARCHIVE_OK) {
                qWarning().noquote() << u"libarchive write_header failed: %1"_qsv.arg(
                    svtostr(QUtf8SV(archive_error_string(disk)))
                );
                return false;
            }

            const void* buffer;
            usize size;
            i64 offset;
            bool success = true;

            while (true) {
                const i32 read = archive_read_data_block(archive_, &buffer, &size, &offset);

                if (read == ARCHIVE_EOF) {
                    break;
                }

                if (read != ARCHIVE_OK) {
                    qWarning().noquote() << u"libarchive read_data_block failed: %1"_qsv.arg(
                        svtostr(QUtf8SV(archive_error_string(archive_)))
                    );
                    success = false;
                    break;
                }

                if (archive_write_data_block(disk, buffer, size, offset) != ARCHIVE_OK) {
                    qWarning().noquote() << u"libarchive write_data_block failed: %1"_qsv.arg(
                        svtostr(QUtf8SV(archive_error_string(disk)))
                    );
                    success = false;
                    break;
                }
            }

            archive_write_finish_entry(disk);
            return success;
        };

        bool extracted = false;
        archive_entry* entry;

        while (archive_read_next_header(archive_, &entry) == ARCHIVE_OK) {
            const char* const rawName = archive_entry_pathname_utf8(entry);
            const u32 type = archive_entry_filetype(entry);

            // Only the executable, notice and licenses are installed. Names are joined to the install directory by
            // hand, so an entry that could climb out of it, or is not a plain file or directory, is never written.
            // A link entry would be installed as a link to anywhere
            const bool isPlain = archive_entry_symlink(entry) == nullptr && archive_entry_hardlink(entry) == nullptr;
            const bool isExecutable =
                rawName != nullptr && isPlain && type == AE_IFREG && string_view(rawName) == targetEntry;
            const bool isDocument =
                rawName != nullptr && isPlain && !string_view(rawName).contains("..") &&
                (type == AE_IFREG || type == AE_IFDIR) &&
                (string_view(rawName) == "THIRD-PARTY-NOTICE.md" || string_view(rawName).starts_with("licenses/"));

            if (!isExecutable && !isDocument) {
                archive_read_data_skip(archive_);
                continue;
            }

            if (isExecutable) {
                archive_entry_set_pathname_utf8(entry, outputPath.constData());
                extracted = extractEntry(entry);

                if (!extracted) {
                    break;
                }
            } else {
                // The licenses are not worth failing an update that already installed the executable
                const QByteArray documentPath = appDirUtf8 % '/' % rawName;
                archive_entry_set_pathname_utf8(entry, documentPath.constData());
                extractEntry(entry);
            }
        }

        cleanup();

        if (!extracted) {
            restoreExe();
            present(this, NOTICE("Failed to extract update archive.", Warning, Modal));
            return;
        }

#ifndef _WIN32
        const QString extractedPath = QString::fromUtf8(outputPath);

        QFile::setPermissions(
            extractedPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup |
                QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther
        );

        // rename(2) replaces the target atomically, also while it is running
        error_code renameError;
        fs::rename(extractedPath.toStdString(), installTarget.absoluteFilePath().toStdString(), renameError);

        if (renameError) {
            QFile::remove(extractedPath);
            present(
                this,
                NOTICE(
                    "Failed to install the update: %1",
                    Warning,
                    Modal,
                    QString::fromStdString(renameError.message())
                )
            );
            return;
        }
#endif

        // Starting the new instance has to wait for the old one to give up the single-instance lock, which main()
        // releases in its own aboutToQuit slot. That slot was connected first, so it has run by the time this does.
        connect(qApp, &QApplication::aboutToQuit, qApp, [restartPath = installTarget.absoluteFilePath()] -> void {
            QProcess::startDetached(restartPath, QStringList());
        }, Qt::SingleShotConnection);

        qApp->quit();
    }, Qt::SingleShotConnection);

    connect(updater, &AutoUpdater::versionFetched, this, [=, this](const QString& version) -> void {
        const auto newVersion = QVersionNumber::fromString(version);
        const auto currentVersion = QVersionNumber::fromString(QStringView(RPGMT_VERSION));

        if (newVersion <= currentVersion) {
            if (manual) {
                present(this, NOTICE("Program is up-to-date.", Info, Status));
            } else {
                qInfo().noquote() << u"Program is up-to-date."_qsv;
            }

            updater->deleteLater();
            return;
        }

        auto msgBox = QMessageBox(this);
        auto* const checkbox = new QCheckBox(tr("Don't remind me"), &msgBox);
        msgBox.setWindowTitle(tr("New version is available"));
        msgBox.setText(tr("Version %1 is available.\nCurrent version is %2.").arg(version, QStringView(RPGMT_VERSION)));
        const QPushButton* const installButton = msgBox.addButton(tr("Install"), QMessageBox::AcceptRole);
        const QPushButton* const skipButton = msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);
        msgBox.setCheckBox(checkbox);

        msgBox.exec();
        const auto* const clickedButton = msgBox.clickedButton();

        if (clickedButton == installButton) {
            updater->downloadUpdate();

            updateProgressDialog = new QProgressDialog(tr("Installing update..."), tr("Abort"), 0, 1, this);
            updateProgressDialog->setWindowModality(Qt::WindowModal);
            updateProgressDialog->setAttribute(Qt::WA_DeleteOnClose);

            connect(updateProgressDialog, &QProgressDialog::canceled, updater, [updater] -> void {
                updater->abortDownload();
                updater->deleteLater();
            });

            connect(
                updater,
                &AutoUpdater::updateDownloadProgress,
                this,
                [this](const i32 received, const i32 total) -> void {
                if (updateProgressDialog != nullptr) {
                    updateProgressDialog->setMaximum(total);
                    updateProgressDialog->setValue(received);
                }
            }
            );
        } else if (clickedButton == skipButton) {
            if (checkbox->isChecked()) {
                settings->core.checkForAppUpdates = false;
            }

            updater->deleteLater();
        } else {
            // The window was closed
            updater->deleteLater();
        }
    }, Qt::SingleShotConnection);

    connect(updater, &AutoUpdater::updateFailed, this, [=, this](const QString& error, const bool download) -> void {
        if (updateProgressDialog != nullptr) {
            updateProgressDialog->close();
        }

        // The check at startup stays silent when the network is down
        if (download || manual) {
            present(this, NOTICE("Update failed with error: %1", Warning, Modal, error));
        } else {
            qWarning().noquote() << u"Checking for updates failed: %1"_qsv.arg(error);
        }

        updater->deleteLater();
    });

    updater->checkForUpdates();
#else
    // The startup check stays silent; only an explicit request is answered
    if (manual) {
        present(
            this,
            NOTICE(
                "This build of RPGMTranslate has no self-updater (it was built with ENABLE_UPDATER off). Download new "
                "versions from %1, or update it through the package manager it was installed with.",
                Info,
                Modal,
                u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases"_qsv
            )
        );
    }
#endif
}

void MainWindow::openProject(QString folder, const bool newProject, const bool translationsRewritten) {
    if (!QFile::exists(folder)) {
        present(this, NOTICE("Folder %1 does not exist.", Critical, Modal, folder));
        return;
    }

    if (!closeProject(translationsRewritten)) {
        return;
    }

    if (!newProject && !QFile::exists(folder + PROGRAM_DATA_DIRECTORY)) {
        present(
            this,
            NOTICE(
                "Failed to opened project because couldn't locate `.rpgmtranslate` program directory that was previously located at this path: %1. If this is intentional, please reopen the directory manually.",
                Critical,
                Modal,
                folder + PROGRAM_DATA_DIRECTORY
            )
        );
        return;
    }

    auto tempProjectSettings = make_shared<ProjectSettings>();
    tempProjectSettings->projectPath = folder;

    const QString rootTranslationPath = folder + TRANSLATION_DIRECTORY;

    // Sets the engine from the data files, or from the RGSS archive if there are none. Returns the archive to extract
    // first, or "" when the data files exist. Needs `sourceDirectory`.
    const auto detectEngine = [this, tempProjectSettings] -> Result<QString, Notice> {
        if (tempProjectSettings->sourceDirectory == SourceDirectory::Root) {
            tempProjectSettings->engineType = EngineType::RM2K;
            return QString();
        }

        using Candidate = pair<QStringView, EngineType>;

        static constexpr array<Candidate, 4> SYSTEM_FILES = { {
            { u"rvdata2", EngineType::VXAce },
            { u"rvdata", EngineType::VX },
            { u"rxdata", EngineType::XP },
            { u"json", EngineType::MVMZ },
        } };

        for (const auto& [extension, engine] : SYSTEM_FILES) {
            const QString systemPath = tempProjectSettings->sourcePath() % u"/System."_qsv % extension;

            if (QFile::exists(systemPath)) {
                tempProjectSettings->engineType = engine;
                return QString();
            }
        }

        static constexpr array<Candidate, 3> ARCHIVES = { {
            { u"rgssad", EngineType::XP },
            { u"rgss2a", EngineType::VX },
            { u"rgss3a", EngineType::VXAce },
        } };

        for (const auto& [extension, engine] : ARCHIVES) {
            QString archivePath = tempProjectSettings->projectPath % u"/Game."_qsv % extension;

            if (QFile::exists(archivePath)) {
                tempProjectSettings->engineType = engine;
                return archivePath;
            }
        }

        return Err(NOTICE("Source files, translation or archive file do not exist.", Critical, Modal));
    };

    // What a project keeps of the last read in `.rvpacker-metadata`, for a project that has no settings of its own yet
    const auto applyMetadata = [tempProjectSettings](Metadata&& metadata) -> void {
        tempProjectSettings->hashes = std::move(metadata.hashes);
        tempProjectSettings->readEncoding = QString::fromLatin1(metadata.readEncoding.value_or(""));

        if (metadata.duplicateMode) {
            tempProjectSettings->duplicateMode = scast<DuplicateMode>(*metadata.duplicateMode);
        }
    };

    const auto postRead = [this, folder, tempProjectSettings, detectEngine, applyMetadata](
                              const Result<ByteBuffer, FFIString> result
                          ) -> ::Result<void, FFIString> {
        if (!result) {
            return Err(result.error());
        }

        const auto hashes = result.value();
        const bool freshRead = hashes.ptr != nullptr;

        if (freshRead) {
            tempProjectSettings->hashes = takeHashes(hashes);
        }

        const QString projectSettingsPath = folder % PROGRAM_DATA_DIRECTORY % PROJECT_SETTINGS_FILE;

        bool useTempSettings = true;

        // Falls back to `project-settings.json.bak`, which every save keeps
        if (auto loaded = readJsonFile<ProjectSettings>(projectSettingsPath, hasKnownValues)) {
            projectSettings = make_unique<ProjectSettings>(std::move(*loaded));
            useTempSettings = false;

            // Older versions left it unset for a game without a data directory, and `sourcePath()` needs it
            if (projectSettings->sourceDirectory == SourceDirectory::None ||
                projectSettings->sourceDirectory > SourceDirectory::Root) {
                projectSettings->sourceDirectory = detectSourceDirectory(folder);
            }

            // The project was just read again (e.g. its translation was deleted), so what that read produced
            // replaces what the old file recorded.
            if (freshRead) {
                projectSettings->hashes = std::move(tempProjectSettings->hashes);
                projectSettings->flags = tempProjectSettings->flags;
                projectSettings->duplicateMode = tempProjectSettings->duplicateMode;
                projectSettings->readEncoding = std::move(tempProjectSettings->readEncoding);
                projectSettings->engineType = tempProjectSettings->engineType;
                projectSettings->sourceDirectory = tempProjectSettings->sourceDirectory;
            }
        }

        if (useTempSettings) {
            // A first read brings engine and hashes; a project with a missing or unusable settings file gets its engine
            // detected again and the rest from `.rvpacker-metadata`. Other settings (columns, languages...) are lost.
            if (!freshRead) {
                if (!detectEngine()) {
                    static constexpr string_view message =
                        "project-settings.json is unusable and the engine of the project could not be detected "
                        "from its files.";

                    return Err(FFIString{ .ptr = message.data(), .len = scast<u32>(message.size()), .cap = 0 });
                }

                auto metadataFile = QFile(tempProjectSettings->translationPath() + RVPACKER_METADATA_FILE);

                if (metadataFile.open(QFile::ReadOnly)) {
                    const QByteArray content = metadataFile.readAll();
                    auto metadata = readLenient<Metadata>(string_view(content.data(), content.size()));

                    if (metadata && metadata->duplicateMode <= scast<u8>(DuplicateMode::Remove)) {
                        applyMetadata(std::move(metadata.value()));
                    }
                }
            }

            projectSettings = make_unique<ProjectSettings>(std::move(*tempProjectSettings));
        }

        projectSettings->projectPath = folder;
        settings->core.projectPath = folder;

        QDir().mkdir(projectSettings->backupPath());
        QDir().mkdir(projectSettings->translationPath());

        i32 totalLines = 0;
        i32 totalTranslated = 0;

        u8 columnCount = max<u8>(2, projectSettings->columns.size());

        const auto translationFiles =
            QDirListing(projectSettings->translationPath(), { u"*.txt"_s }, QDirListing::IteratorFlag::FilesOnly);

        vector<TabListItem> tabs;

        bool skippedFlag = false;

        for (const auto& fileInfo : translationFiles) {
            if (!isUsableTabName(fileInfo.baseName())) {
                qWarning().noquote() << u"Skipping %1: its name can't be a file of the translation."_qsv.arg(
                    fileInfo.filePath()
                );
                continue;
            }

            auto file = QFile(fileInfo.filePath());

            if (!file.open(QFile::ReadOnly)) {
                qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(fileInfo.filePath(), file.errorString());
                continue;
            }

            QByteArray normalized = file.readAll();
            file.close();

            // Rewrite only files with carriage returns, atomically, so a crash can't truncate a translation. A failed
            // rewrite is not fatal: the normalized content is still parsed.
            if (const isize sizeBefore = normalized.size();
                normalized.removeIf([](const char byte) -> bool { return byte == '\r'; }).size() != sizeBefore) {
                auto normalizedFile = QSaveFile(fileInfo.filePath());
                normalizedFile.setDirectWriteFallback(true);

                if (!normalizedFile.open(QFile::WriteOnly) || normalizedFile.write(normalized) != normalized.size() ||
                    !normalizedFile.commit()) {
                    qWarning().noquote() << u"Failed to normalize line endings of %1: %2"_qsv.arg(
                        fileInfo.filePath(),
                        normalizedFile.errorString()
                    );
                }
            }

            QString basename = fileInfo.baseName();
            const bool isMap = basename.startsWith(u"map");

            const QString content = QString::fromUtf8(normalized);
            const auto contentView = QStringView(content);
            const u32 size = contentView.size();

            // The game title is on the line after its marker, which is the last line of the file: it is edited in its
            // own input, so the table doesn't hold it (see `changeTab`, `saveCurrentTab`)
            const isize titleMarker = basename.startsWith(u"system") || basename.startsWith(u"terms")
                                          ? contentView.lastIndexOf(GAME_TITLE_MARKER)
                                          : -1;
            const bool holdsTitle =
                titleMarker != -1 && titleMarker + GAME_TITLE_MARKER.size() + 1 < contentView.size();

            u32 pos = 0;
            u32 lineIndex = 0;
            QStringView entry;

            i32 fileTotal = 0;
            i32 fileTranslated = 0;

            QStringView mapID;
            u32 mapSectionStart = 0;

            while (pos < size) {
                const u32 lineStart = pos;
                u32 newlinePos = contentView.indexOf(u'\n', pos);

                if (newlinePos == scast<u32>(-1)) {
                    newlinePos = size;
                }

                // No carriage returns here: they were removed above
                const QStringView line = contentView.sliced(lineStart, newlinePos - lineStart);
                pos = newlinePos + 1;

                if (holdsTitle && pos >= size) {
                    break;
                }

                if (line.trimmed().isEmpty()) {
                    continue;
                }

                // The table holds `MAX_COLUMNS` of them; a line with more parts is kept by it as it is
                columnCount = scast<u8>(
                    max<i64>(columnCount, min<i64>(line.count(SEPARATOR) + 1, TranslationTableModel::MAX_COLUMNS))
                );

                // Bookmarks remember the entry they are in; entries start at their `<!>ID` row.
                if (const auto name = entryNameOfLine(line)) {
                    entry = *name;
                } else if (line.startsWith(ID_COMMENT)) {
                    entry = {};
                }

                if (line.startsWith(BOOKMARK_COMMENT)) {
                    const u32 left = line.indexOf(SEPARATOR) + SEPARATOR.size();
                    const isize right = line.indexOf(SEPARATOR, left);

                    // A map is a tab of its own (`map12`), with rows counted from its `<!>ID` line
                    bookmarkMenu->addBookmark(
                        right == -1 ? line.sliced(left) : line.sliced(left, right - left),
                        isMap ? u"map"_s + mapID : basename,
                        lineIndex,
                        entry
                    );

                    lineIndex++;
                    continue;
                }

                if (isMap && line.startsWith(ID_COMMENT)) {
                    if (!mapID.isEmpty()) {
                        // Every section is kept, also one with no text to translate: the maps are saved from these
                        mapSections.insert(
                            { stoa<u16>(mapID).value_or(0),
                              content.sliced(mapSectionStart, lineStart - mapSectionStart) }
                        );

                        if (fileTotal != 0) {
                            const QString tempMapName = u"map"_s + mapID;

                            tabs.emplace_back(
                                tempMapName,
                                TabListItemProps{ fileTotal,
                                                  fileTranslated,
                                                  projectSettings->completedFiles.contains(tempMapName) }
                            );

                            totalLines += fileTotal;
                            totalTranslated += fileTranslated;

                            fileTotal = 0;
                            fileTranslated = 0;
                        }
                    }

                    mapSectionStart = lineStart;
                    mapID = line.sliced(line.indexOf(SEPARATOR) + SEPARATOR.size());
                    const isize end = mapID.indexOf(SEPARATOR);

                    if (end != -1) {
                        mapID = mapID.sliced(0, end);
                    }

                    // The section's table starts with this line
                    lineIndex = 1;
                    continue;
                }

                if (isRowLine(line)) {
                    const QSVList parts = lineParts(line, lineIndex, basename);

                    // The table keeps such a line as it is, read-only, so it takes a row but is not text to translate
                    if (parts.isEmpty()) {
                        skippedFlag = true;
                        lineIndex++;
                        continue;
                    }

                    fileTotal++;

                    if (hasTranslation(parts)) {
                        fileTranslated++;
                    }
                }

                lineIndex++;
            }

            // The last section of the maps, kept as the others
            if (isMap && (fileTotal != 0 || !mapID.isEmpty())) {
                mapSections.insert(
                    { stoa<u16>(mapID).value_or(0), content.sliced(mapSectionStart, size - mapSectionStart) }
                );
            }

            if (fileTotal == 0) {
                continue;
            }

            totalLines += fileTotal;
            totalTranslated += fileTranslated;

            if (isMap) {
                basename = u"map"_s + mapID;
            }

            tabs.emplace_back(
                basename,
                TabListItemProps{ fileTotal, fileTranslated, projectSettings->completedFiles.contains(basename) }
            );

            if (holdsTitle) {
                const QStringView titleLine = contentView.sliced(titleMarker + GAME_TITLE_MARKER.size() + 1);
                const QSVList parts = lineParts(titleLine, 0, basename);

                if (parts.isEmpty()) {
                    continue;
                }

                const QStringView translation = getTranslation(parts).getTranslation();
                const QStringView source = getSource(parts);

                gameTitleTab = basename;
                ui->gameTitleInput->setPlaceholderText(source.toString());

                if (translation.trimmed().isEmpty()) {
                    ui->gameTitleInput->setText(source.toString());
                } else {
                    ui->gameTitleInput->setText(translation.toString());
                }

                const i32 textWidth = QFontMetrics(qApp->font()).horizontalAdvance(ui->gameTitleInput->text());

                const QMargins margins = ui->gameTitleInput->textMargins();

                const i32 frame = ui->gameTitleInput->style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;

                static constexpr i32 GAME_TITLE_INPUT_PADDING = 32;
                const i32 finalWidth = textWidth + margins.left() + margins.right() + frame + GAME_TITLE_INPUT_PADDING;

                ui->gameTitleInput->setMinimumWidth(finalWidth);
                ui->gameTitleInput->setCursorPosition(0);
            }

            qInfo().noquote() << u"%1: Successfully parsed."_qsv.arg(fileInfo.filePath());
        }

        searchMenu->init(tabs);
        batchMenu->init(tabs, projectSettings.get());
        readMenu->init(tabs);
        purgeMenu->init(tabs);
        writeMenu->init(tabs);
        serdeMenu->init(tabs);
        bookmarkMenu->init(tabs);
        ui->lintMenu->init(ui, tabs, settings.get(), projectSettings.get(), glossaryMenu);
        ui->searchPanel->setFiles(tabs);
        ui->tabPanel->setTabs(std::move(tabs));

        if (const auto glossary = readJsonFile<Glossary>(projectSettings->glossaryPath())) {
            glossaryMenu->fill(*glossary);
        }

        if (projectSettings->columns.empty()) {
            projectSettings->columns.emplace_back(tr("Source"), DEFAULT_COLUMN_WIDTH);
        }

        while (columnCount > projectSettings->columns.size()) {
            projectSettings->columns.emplace_back(tr("Translation"), DEFAULT_COLUMN_WIDTH);
        }

        for (const auto& column : views::drop(projectSettings->columns, 1)) {
            batchMenu->addColumn(column.name);
            searchMenu->addColumn(column.name);
        }

        assetMenu->init(projectSettings.get());
        readMenu->init(projectSettings.get());
        taskWorker->init(settings.get(), projectSettings.get(), &mapSections);

        ui->translationTable->init(settings.get(), projectSettings.get());

        setProjectActionsEnabled(true);
        updateTaskControls();

        ui->globalProgressBar->setEnabled(true);
        ui->globalProgressBar->setMaximum(totalLines);
        ui->globalProgressBar->setValue(totalTranslated);

        if (settings->core.backup.enabled) {
            backupTimer.start((settings->core.backup.period * SECOND_MS));
        }

        // The menu lists the projects in the same order as the settings, the most recently opened last. So the entry
        // of the same position is the one to remove with the project.
        const auto removeRecentProject = [this](const isize index) -> void {
            settings->core.recentProjects.removeAt(index);

            QAction* const action = recentProjectsMenu->actions().at(index);
            recentProjectsMenu->removeAction(action);
            action->deleteLater();
        };

        if (const isize recentIndex = settings->core.recentProjects.indexOf(folder);
            recentIndex != settings->core.recentProjects.size() - 1) {
            if (recentIndex != -1) {
                removeRecentProject(recentIndex);
            }

            settings->core.recentProjects.append(folder);
            addRecentProject(folder);
        }

        while (settings->core.recentProjects.size() > MAX_RECENT_PROJECTS) {
            removeRecentProject(0);
        }

        ui->statusBar->showMessage(
            tr("Before working with the program, check out documentation in Help > Documentation!")
        );

        if (skippedFlag) {
            present(
                this,
                NOTICE(
                    "The program failed to split some lines into parts because of absence of <#> delimiter. Check log to see skipped lines.",
                    Warning,
                    Modal
                )
            );
        }

        saveProjectSettings();

#ifdef ENABLE_LIBGIT2
        const auto openRepoResult =
            ui->sourceControlDock->openRepository(projectSettings->programDataPath(), settings->core.git);

        if (!openRepoResult) {
            present(
                this,
                NOTICE("Failed to open git repository with error: %1", Warning, Modal, openRepoResult.error())
            );
        }
#endif

        const QString baselinePath = projectSettings->baselinePath();
        const QString sourcePath = projectSettings->sourcePath();

        const QString baselineDataPath = baselinePath + u"/data"_qsv;

        // TODO(v2): remove
        if (QFile::exists(baselinePath) && !QFile::exists(baselineDataPath)) {
            const string baselineDataStd = baselineDataPath.toStdString();
            error_code error;
            fs::create_directory(baselineDataStd, error);

            // Stops at the first failure
            for (auto entry = fs::directory_iterator(baselinePath.toStdString(), error);
                 !error && entry != fs::directory_iterator();
                 entry.increment(error)) {
                if (entry->is_regular_file(error)) {
                    fs::rename(entry->path(), baselineDataStd / entry->path().filename(), error);
                }
            }

            if (error) {
                qWarning().noquote() << u"Failed to move the baseline files to %1: %2"_qsv.arg(
                    baselineDataPath,
                    QString::fromStdString(error.message())
                );
            }
        }

        if (QFile::exists(sourcePath)) {
            if (!QFile::exists(baselinePath)) {
                // If there's no baseline, make the current source the baseline.
                present(
                    this,
                    NOTICE("Copying the game data to .rpgmtranslate/baseline-data as a baseline.", Info, Status)
                );

                copyToBaseline(sourcePath, baselinePath);
            } else {
                if (settings->core.checkForSourceUpdates) {
                    checkHashes();
                }
            }
        }

        return {};
    };

    const auto postArchive =
        [this,
         tempProjectSettings,
         postRead](const QString& sourcePath, const QString& translationPath, const QString& title) -> void {
        tempProjectSettings->flags = readMenu->flags();
        tempProjectSettings->duplicateMode = readMenu->duplicateMode();
        tempProjectSettings->readEncoding = readMenu->readEncoding();

        const TaskToken task = startTask(TaskKind::Read);

        if (task == nullptr) {
            return;
        }

        const bool mapEvents = readMenu->parseMapEvents();

        QtConcurrent::run([this, tempProjectSettings, sourcePath, translationPath, title, mapEvents] -> auto {
            return taskWorker->read(
                sourcePath,
                translationPath,
                ReadMode::Default,
                tempProjectSettings->engineType,
                tempProjectSettings->duplicateMode,
                Selected{},
                tempProjectSettings->flags,
                mapEvents,
                tempProjectSettings->hashes,
                title,
                tempProjectSettings->readEncoding
            );
        })
            .then(this, [this, postRead, task](const ReadResult& result) -> void {
            taskWorker->finishTask(task);

            const auto postReadResult = postRead(result);

            if (!postReadResult) {
                present(this, NOTICE("Failed to load project: %1", Critical, Modal, ffitostr(postReadResult.error())));
            }
        }).onFailed(this, [this, task] -> void { taskFailed(task); });
    };

    const auto startOpening =
        [this, folder, rootTranslationPath, tempProjectSettings, postRead, postArchive, detectEngine]
        -> Result<void, Notice> {
        // RPG Maker 2000/2003 has no `data`/`Data` subdirectory at all - every data file lives at the root
        tempProjectSettings->sourceDirectory = detectSourceDirectory(folder);

        if (!QFile::exists(folder + PROGRAM_DATA_DIRECTORY + TRANSLATION_DIRECTORY)) {
            bool copied = false;

            if (QFile::exists(rootTranslationPath)) {
                const auto selected = QMessageBox::question(
                    this,
                    tr("Existing translation folder"),
                    tr("Translation folder is found in the root of the project. Use it?")
                );

                if (selected == QMessageBox::Yes) {
                    qInfo().noquote() << u"Reusing the root translation folder was confirmed by user."_qsv;

                    // Everything that can make the folder unusable is checked before anything is copied
                    if (const auto engine = detectEngine(); !engine) {
                        return Err(engine.error());
                    }

                    // Optional: a translation that rvpacker-txt-rs didn't make has no metadata
                    optional<Metadata> metadata;
                    auto metadataFile = QFile(rootTranslationPath + RVPACKER_METADATA_FILE);

                    if (metadataFile.exists()) {
                        if (!metadataFile.open(QFile::ReadOnly)) {
                            return Err(NOTICE(
                                "Failed to open the .rvpacker-metadata file: %1",
                                Critical,
                                Modal,
                                metadataFile.errorString()
                            ));
                        }

                        const QByteArray content = metadataFile.readAll();
                        const string_view json(content.data(), content.size());
                        auto parsed = readLenient<Metadata>(json);

                        if (!parsed) {
                            return Err(NOTICE(
                                "Failed to parse the .rvpacker-metadata file: %1",
                                Critical,
                                Modal,
                                QString::fromStdString(glz::format_error(parsed.error(), json))
                            ));
                        }

                        metadata = std::move(parsed.value());

                        if (hasCustomFormat(*metadata)) {
                            return Err(NOTICE(
                                "This translation uses a custom line separator, line break or comment prefix, which "
                                "RPGMTranslate does not support.",
                                Critical,
                                Modal
                            ));
                        }

                        if (metadata->duplicateMode > scast<u8>(DuplicateMode::Remove)) {
                            return Err(NOTICE(
                                "The .rvpacker-metadata file has an unknown duplicate mode: %1",
                                Critical,
                                Modal,
                                qsv(qitos(*metadata->duplicateMode))
                            ));
                        }
                    }

                    QDir().mkpath(folder % PROGRAM_DATA_DIRECTORY % TRANSLATION_DIRECTORY);

                    try {
                        fs::copy(
                            rootTranslationPath.toStdString(),
                            QString(folder % PROGRAM_DATA_DIRECTORY % TRANSLATION_DIRECTORY).toStdString(),
                            fs::copy_options::recursive | fs::copy_options::overwrite_existing
                        );

                        copied = true;
                    } catch (const fs::filesystem_error& err) {
                        qWarning().noquote() << u"Failed to copy directory %1: %2"_qsv.arg(
                            rootTranslationPath,
                            svtostr(QUtf8SV(err.what()))
                        );
                    }

                    // `postRead` applies the metadata of the copy, as the project has no settings of its own yet
                }
            }

            if (!copied) {
                const auto archive = detectEngine();

                if (!archive) {
                    return Err(archive.error());
                }

                const QString archivePath = archive.value();
                const bool systemExists = archivePath.isEmpty();

                firstReadPending = true;

                readMenu->show();
                readMenu->move((width() / 2) - (readMenu->width() / 2), (height() / 2) - (readMenu->height() / 2));

                const auto readAnswer =
                    readMenu->exec(tempProjectSettings->projectPath, tempProjectSettings->engineType, !systemExists);

                // Only the ccepted of the menu that xec waited for had to be ignored
                firstReadPending = false;

                if (readAnswer != QDialog::Accepted) {
                    return Err(NOTICE("Read was rejected by user.", Info, Status));
                }

                if (!systemExists) {
                    const TaskToken task = startTask(TaskKind::ExtractArchive);

                    if (task == nullptr) {
                        return Err(NOTICE("Another task is already running.", Warning, Modal));
                    }

                    // Names decode with the Read menu's Archive Encoding
                    const QString archiveEncoding = readMenu->archiveReadEncoding();

                    QtConcurrent::run([this, archivePath, folder, archiveEncoding] -> auto {
                        return taskWorker->extractArchive(archivePath, folder, archiveEncoding);
                    })
                        .then(
                            this,
                            [this, tempProjectSettings, postArchive, task](const ExtractResult& result) -> void {
                        taskWorker->finishTask(task);

                        if (!result) {
                            present(
                                this,
                                NOTICE("Failed to load project: %1", Critical, Modal, ffitostr(result.error()))
                            );
                            return;
                        }

                        postArchive(
                            tempProjectSettings->sourcePath(),
                            tempProjectSettings->translationPath(),
                            readMenu->title()
                        );
                    }
                        )
                        .onFailed(this, [this, task] -> void { taskFailed(task); });

                    return {};
                }

                postArchive(
                    tempProjectSettings->sourcePath(),
                    tempProjectSettings->translationPath(),
                    readMenu->title()
                );

                return {};
            }
        }

        const auto result = postRead(ByteBuffer(nullptr, 0, 0));

        if (!result) {
            return Err(NOTICE("Failed to load project: %1", Critical, Modal, ffitostr(result.error())));
        }

        return {};
    };

    const auto result = startOpening();

    if (!result) {
        present(this, result.error());
        return;
    }
}

void MainWindow::changeTab(const QString& tabName, const QString& previousTabName) {
    if (!previousTabName.isEmpty()) {
        const bool success = saveCurrentTab(previousTabName);

        if (!success) {
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }
    }

    if (tabName.isEmpty()) {
        ui->translationTable->model()->clear();
        updateStatusLabels();
        updateCounts();
    } else {
        // The tab panel is disabled meanwhile, but a tab can also be opened from the search results, the bookmarks...
        if (const TaskToken rewriting = rewritingTask(); rewriting != nullptr) {
            present(
                this,
                NOTICE("Cannot open a tab while %1 is running.", Warning, Status, taskName(rewriting->kind()))
            );
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }

        if (tabName == lockedFile) {
            present(this, NOTICE("File is currently processed and is being locked.", Warning, Modal));
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }

        const auto result = fileLines(QL1SV(tabName.toLatin1()), mapSections, projectSettings.get());

        if (!result) {
            present(this, NOTICE("Failed to open tab: %1.", Warning, Modal, result.error()));
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }

        QSVList lines = result.value().lines;

        // The game title is the file's last line, edited in its own input and appended on save
        const bool dropsTitle = tabName == gameTitleTab && !lines.isEmpty();

        ui->translationTable
            ->fill(views::take(lines, lines.size() - (dropsTitle ? 1 : 0)), projectSettings->columns, tabName);

        updateStatusLabels();
        updateCounts();
    }
}

auto MainWindow::stopTasks() -> bool {
    // It is not a task, but it reads the settings that the callers are about to free
    singleLintFuture.waitForFinished();

    if (taskWorker->idle()) {
        return true;
    }

    // A task that can't be interrupted uses project data the close would free, so it must finish on its own.
    if (const TaskToken blocking = taskWorker->uninterruptibleTask(); blocking != nullptr) {
        present(
            this,
            NOTICE("Cannot close the project while %1 is running.", Warning, Modal, taskName(blocking->kind()))
        );
        return false;
    }

    // The tasks that remain read the project's data until they notice the request and return. Their continuations
    // run in this loop, so `taskFinished` is signaled from it.
    stoppingTasks = true;
    taskWorker->interruptAll();

    QEventLoop loop;

    connect(taskWorker, &TaskWorker::taskFinished, &loop, [this, &loop] -> void {
        if (taskWorker->idle()) {
            loop.quit();
        }
    });

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    stoppingTasks = false;

    return true;
}

auto MainWindow::closeProject(const bool translationsRewritten) -> bool {
    if (!stopTasks()) {
        return false;
    }

    if (!saveEverything(translationsRewritten)) {
        return false;
    }

    glossaryMenu->hide();
    bookmarkMenu->hide();
    searchMenu->hide();
    batchMenu->hide();
    ui->lintMenu->hide();
    translationsMenu->hide();
    assetMenu->hide();
    readMenu->hide();
    purgeMenu->hide();
    writeMenu->hide();
    serdeMenu->hide();

    for (auto* const window : findChildren<SettingsWindow*>(Qt::FindDirectChildrenOnly)) {
        window->hide();
        window->deleteLater();
    }

    for (auto* const dialog : findChildren<StatisticsDialog*>(Qt::FindDirectChildrenOnly)) {
        dialog->hide();
        dialog->deleteLater();
    }

    for (auto* const dialog : findChildren<BackupSelectDialog*>(Qt::FindDirectChildrenOnly)) {
        dialog->hide();
        dialog->deleteLater();
    }

    // Already saved by `saveEverything()`, or by whoever rewrote the files (`translationsRewritten`)
    {
        const QSignalBlocker blocker(ui->tabPanel);
        ui->tabPanel->changeTab(QString());
    }

    ui->translationTable->model()->clear();
    updateStatusLabels();
    updateCounts();

    mapSections.clear();
    gameTitleTab.clear();

    translateDebounceTimer.stop();
    translateGeneration++;
    lintGeneration++;

    ui->tabPanel->clear();

    glossaryMenu->clear();
    bookmarkMenu->clear();
    searchMenu->clear();
    batchMenu->clear();
    ui->lintMenu->clear();
    translationsMenu->clear();
    assetMenu->clear();

    ui->searchPanel->clear();

    readMenu->clear();
    purgeMenu->clear();
    writeMenu->clear();
    serdeMenu->clear();

#ifdef ENABLE_LIBGIT2
    ui->sourceControlDock->clear();
#endif

    setProjectActionsEnabled(false);

    ui->gameTitleInput->clear();
    ui->gameTitleInput->setPlaceholderText(QString());

    ui->globalProgressBar->setMaximum(0);
    ui->globalProgressBar->setValue(0);
    ui->globalProgressBar->setEnabled(false);

    ui->statusBar->clearMessage();

    backupTimer.stop();

    // No task is running here, so nothing is using what the worker points to, and nothing may start to until the
    // next project is opened and `init` is called again
    taskWorker->init(settings.get(), nullptr, nullptr);

    projectSettings.reset();

    updateTaskControls();

    settings->core.projectPath = QString();

    return true;
}

auto MainWindow::startTask(const TaskKind kind) -> TaskToken {
    const auto task = taskWorker->startTask(kind);

    if (!task) {
        present(
            this,
            NOTICE("Cannot start %1 while %2 is running.", Warning, Modal, taskName(kind), taskName(task.error()))
        );
        return nullptr;
    }

    return task.value();
}

void MainWindow::taskFailed(const TaskToken& task) {
    taskWorker->finishTask(task);

    if (!stoppingTasks) {
        present(this, NOTICE("%1 failed because of an internal error.", Critical, Modal, taskName(task->kind())));
    }
}

auto MainWindow::rewritingTask() const -> TaskToken {
    return taskWorker->blockingTask(TaskAccess::Reads);
}

void MainWindow::setProjectActionsEnabled(const bool enabled) {
    for (QAction* const action : { actionTabPanel,
                                   actionGlossaryMenu,
                                   actionTranslationsMenu,
                                   actionBookmarkMenu,
                                   actionSourceControl,
                                   actionAssets,
                                   actionLocateProjectDir,
                                   actionSearchPanel }) {
        action->setEnabled(enabled);
    }

    ui->rvpackerButton->setEnabled(enabled);
    ui->gameTitleInput->setEnabled(enabled);
}

void MainWindow::updateTaskControls() {
    const bool open = projectSettings != nullptr;

    // A task that rewrites the translation files can't have a tab open
    ui->tabPanel->setEnabled(rewritingTask() == nullptr);

    // Menus stay reachable, so `startTask` still refuses; this only controls what the user sees.
    const auto allow = [this, open](QAction* const action, const TaskKind kind) -> void {
        action->setEnabled(open && !taskWorker->blocks(kind));
    };

    allow(actionWrite, TaskKind::Write);
    allow(actionSearch, TaskKind::Search);
    allow(actionBatchMenu, TaskKind::BatchScript);
    allow(actionLintMenu, TaskKind::GlobalLint);
    allow(actionStatistics, TaskKind::Statistics);
    allow(actionSingleTrim, TaskKind::BatchScript);
    allow(actionSingleWrap, TaskKind::BatchScript);
    allow(actionSingleLatinize, TaskKind::BatchScript);
    allow(ui->actionCheckSourceUpdates, TaskKind::Read);

    // These write the translation files themselves: saving, only while none is being rewritten, and loading a backup
    // or switching a branch, only while no task uses them at all
    actionSave->setEnabled(open && taskWorker->blockingTask(TaskAccess::Reads) == nullptr);
    ui->actionLoadBackup->setEnabled(open && taskWorker->idle());
    ui->sourceControlDock->setEnabled(taskWorker->idle());

    // Panels are hidden, not greyed, when their task becomes impossible: some can only be closed by their own buttons.
    // `openProject` shows the one that reads a new project.
    const auto hideIfBlocked = [this](QWidget* const panel, const TaskKind kind) -> void {
        if (taskWorker->blocks(kind)) {
            panel->hide();
        }
    };

    hideIfBlocked(searchMenu, TaskKind::Search);
    hideIfBlocked(batchMenu, TaskKind::BatchScript);
    hideIfBlocked(ui->lintMenu, TaskKind::GlobalLint);
    hideIfBlocked(writeMenu, TaskKind::Write);
    hideIfBlocked(purgeMenu, TaskKind::Purge);
    hideIfBlocked(serdeMenu, TaskKind::SerdeExport);

    if (!firstReadPending) {
        hideIfBlocked(readMenu, TaskKind::Read);
    }
}

void MainWindow::reportSkippedFiles(const span<const FilenameArray> filenames, const u16 skippedCount) {
    if (skippedCount == 0) {
        return;
    }

    QString skippedString;

    for (const auto filename : views::take(filenames, skippedCount)) {
        skippedString += QL1SV(filename.data());
        skippedString += u'\n';
    }

    present(this, NOTICE("The program was unable to open the following files:\n %1", Warning, Modal, skippedString));
}

void MainWindow::checkHashes() {
    const QString sourcePath = projectSettings->sourcePath();

    if (!QFile::exists(sourcePath)) {
        present(this, NOTICE("The source path %1 does not exist.", Warning, Modal, sourcePath));
        return;
    }

    QStringList filters;

    if (projectSettings->engineType == EngineType::RM2K) {
        filters = {
            u"*.lmu"_s,
            u"*.lmt"_s,
            u"*.ldb"_s,
        };
    } else {
        filters = { u"*.%1"_qsv.arg(projectSettings->engineExtension()) };
    }

    const auto sourceFiles =
        QDirListing(sourcePath, filters, QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive);

    vector<FilenameArray> mismatchedFiles;
    mismatchedFiles.reserve(projectSettings->hashes.size());

    vector<FilenameArray> newFiles;
    newFiles.reserve(projectSettings->hashes.size());

    QStringList skippedFiles;

    bool hasAnyFile = false;

    for (const auto& sourceFile : sourceFiles) {
        hasAnyFile = true;

        const QString filename = sourceFile.fileName();

        if (!isTrackedSourceFile(filename)) {
            continue;
        }

        auto file = QFile(sourceFile.filePath());

        if (!file.open(QFile::ReadOnly)) {
            skippedFiles.append(u"%1: %2"_qsv.arg(filename, file.errorString()));
            continue;
        }

        const QByteArray content = file.readAll();
        const ByteBuffer contentBuffer{ .ptr = rcast<const u8*>(content.data()),
                                        .len = scast<u32>(content.size()),
                                        .cap = 0 };

        u64 newHash;
        rpgm_hash_file(contentBuffer, projectSettings->duplicateMode, &newHash);

        const QByteArray filenameUtf8 = filename.toUtf8();
        const QByteArrayView slicedFilename = QByteArrayView(filenameUtf8).slice(0, filenameUtf8.lastIndexOf(u'.'));

        FilenameArray filenameArray{};

        for (const auto [idx, chr] : views::enumerate(slicedFilename)) {
            filenameArray[idx] = scast<char>(tolower(chr));
        }

        const auto entry = projectSettings->hashes.find(filenameArray);

        if (entry == projectSettings->hashes.end()) {
            newFiles.push_back(filenameArray);
            continue;
        }

        const u64 oldHash = entry->second;

        if (oldHash != newHash) {
            mismatchedFiles.push_back(filenameArray);
        }
    }

    if (!hasAnyFile) {
        present(this, NOTICE("No matching source files were found in:\n%1", Warning, Modal, sourcePath));
        return;
    }

    const bool hasMismatched = !mismatchedFiles.empty();
    const bool hasNew = !newFiles.empty();

    if (hasMismatched || hasNew) {
        // One file per line under its heading, for the scrollable details box
        QStringList sections;

        const auto addSection = [&sections](const QString& heading, const vector<FilenameArray>& files) -> void {
            QStringList lines;
            lines.reserve(files.size() + 1);
            lines.append(heading);

            for (const auto& file : files) {
                lines.append(QString::fromLatin1(file.data()));
            }

            sections.append(lines.join(u'\n'));
        };

        if (hasMismatched) {
            addSection(tr("Changed files:"), mismatchedFiles);
        }

        if (hasNew) {
            addSection(tr("New files:"), newFiles);
        }

        if (!skippedFiles.isEmpty()) {
            sections.append(tr("Skipped files:\n%1").arg(skippedFiles.join(u'\n')));
        }

        if (askWithDetails(
                this,
                tr("Source files have been updated"),
                tr("The files have been changed. Do you want to append any new text?"),
                sections.join(u"\n\n"_qsv)
            )) {
            qInfo().noquote() << u"Appending new text from updated source files was confirmed by user."_qsv;

            read(
                ReadMode::AppendDefault,
                projectSettings->duplicateMode,
                Selected(),
                projectSettings->flags,
                false,
                ui->gameTitleInput->placeholderText(),
                projectSettings->readEncoding,
                true
            );
        }
    } else {
        QString skippedText;

        if (!skippedFiles.isEmpty()) {
            skippedText = tr("\n\nSkipped files:\n%1").arg(skippedFiles.join(u'\n'));
        }

        present(this, NOTICE("All source files are up-to-date.%1", Info, Status, skippedText));
    }
}

void MainWindow::read(
    const ReadMode readMode,
    const DuplicateMode duplicateMode,
    const Selected selected,
    const BaseFlags flags,
    const bool mapEvents,
    const QString& title,
    const QString& readEncoding,
    const bool updateBaseline
) {
    const QString sourcePath = projectSettings->sourcePath();
    const QString translationPath = projectSettings->translationPath();

    // Copied here, as the worker must not reach into `projectSettings` while the main thread can change it
    const EngineType engineType = projectSettings->engineType;
    HashMap<FilenameArray, u64> oldHashes = projectSettings->hashes;

    const TaskToken task = startTask(TaskKind::Read);

    if (task == nullptr) {
        return;
    }

    if (!flushTranslations()) {
        taskWorker->finishTask(task);
        return;
    }

    // Only once the task has started: it is refused while any other task runs, and one of those may be reading the
    // baseline (Write)
    if (updateBaseline) {
        copyToBaseline(sourcePath, projectSettings->baselinePath());
    }

    QtConcurrent::run([=, this, oldHashes = std::move(oldHashes)] -> auto {
        return taskWorker->read(
            sourcePath,
            translationPath,
            readMode,
            engineType,
            duplicateMode,
            selected,
            flags,
            mapEvents,
            oldHashes,
            title,
            readEncoding
        );
    })
        .then(this, [this, task](const ReadResult& result) -> void {
        taskWorker->finishTask(task);

        // `openProject` below would run inside the `closeProject` that is waiting for the task
        if (stoppingTasks) {
            return;
        }

        if (!result) {
            present(this, NOTICE("Read failed: %1", Warning, Modal, ffitostr(result.error())));
            return;
        }

        const ByteBuffer hashes = result.value();

        if (hashes.ptr != nullptr) {
            projectSettings->hashes = takeHashes(hashes);
        }

        // The task rewrote the translation files, and `flushTranslations()` left nothing in the editor to save
        openProject(projectSettings->projectPath, false, true);
    }).onFailed(this, [this, task] -> void { taskFailed(task); });
}

#ifdef ENABLE_NUSPELL
auto MainWindow::initDictionary() -> Result<void, QString> {
    // A lint of the open cell may be using the dictionary that is replaced here
    singleLintFuture.waitForFinished();

    const QString path = dataLocation() % u"/dictionaries"_qsv % projectSettings->spellcheckDictionary;

    if (projectSettings->spellcheckDictionary.isEmpty() || !QFile::exists(path)) {
        taskWorker->setDictionary(nullopt);
    } else {
        try {
            nuspell::Dictionary dict;
            dict.load_aff_dic(path.toStdString());
            taskWorker->setDictionary(std::move(dict));
        } catch (nuspell::Dictionary_Loading_Error& err) {
            return Err(QString::fromUtf8(err.what()));
        }
    }

    return {};
}
#endif