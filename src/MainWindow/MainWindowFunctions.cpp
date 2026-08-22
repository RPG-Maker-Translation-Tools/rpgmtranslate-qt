#include "AssetMenu.hpp"
#include "AutoUpdater.hpp"
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
#include "TabListModel.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "WriteMenu.hpp"
#include "glazemeta.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"
#include "version.h"

#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSignalBlocker>
#include <QVersionNumber>
#include <QtConcurrent>
#include <archive.h>
#include <archive_entry.h>

namespace {
constexpr QStringView ID_COMMENT = u"<!-- ID -->";
constexpr i32 MAX_RECENT_PROJECTS = 10;
}  // namespace

void MainWindow::checkForUpdates(const bool manual) {
    if (!settings->core.checkForAppUpdates && !manual) {
        return;
    }

    auto* const updater = new AutoUpdater(this);

    connect(updater, &AutoUpdater::updateDownloaded, this, [this](const QByteArray& archiveData) -> void {
        const QString appDir = qApp->property("data-location").toString();

#ifdef Q_OS_WINDOWS
        const QString exePath = qApp->applicationFilePath();
        QFile::rename(exePath, appDir + u"/rpgmtranslate-old.exe");
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

        archive_write_disk_set_options(disk, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT);

        if (archive_read_open_memory(archive_, archiveData.constData(), scast<usize>(archiveData.size())) !=
            ARCHIVE_OK) {
            qWarning().noquote() << u"libarchive failed to open archive: %1"_qsv.arg(
                svtostr(QUtf8SV(archive_error_string(archive_)))
            );

            present(this, NOTICE("Failed to open update archive.", Warning, Modal));

            cleanup();
            return;
        }

#ifdef Q_OS_WINDOWS
        static constexpr string_view targetEntry = "rpgmtranslate/rpgmtranslate.exe";
        const QByteArray outputPath = (appDir + u"/rpgmtranslate.exe").toUtf8();
#else
        static constexpr string_view targetEntry = "rpgmtranslate";
        const QByteArray outputPath = (appDir + u"/rpgmtranslate").toUtf8();
#endif

        bool extracted = false;
        archive_entry* entry;

        while (archive_read_next_header(archive_, &entry) == ARCHIVE_OK) {
            if (archive_entry_pathname_utf8(entry) != targetEntry) {
                archive_read_data_skip(archive_);
                continue;
            }

            archive_entry_set_pathname(entry, outputPath.constData());

            if (archive_write_header(disk, entry) != ARCHIVE_OK) {
                qWarning().noquote() << u"libarchive write_header failed: %1"_qsv.arg(
                    svtostr(QUtf8SV(archive_error_string(disk)))
                );
                break;
            }

            const void* buffer;
            usize size;
            i64 offset;
            bool writeOk = true;

            while (true) {
                const i32 read = archive_read_data_block(archive_, &buffer, &size, &offset);

                if (read == ARCHIVE_EOF) {
                    break;
                }

                if (read != ARCHIVE_OK) {
                    qWarning().noquote() << u"libarchive read_data_block failed:"_qsv.arg(
                        svtostr(QUtf8SV(archive_error_string(archive_)))
                    );
                    writeOk = false;
                    break;
                }

                if (archive_write_data_block(disk, buffer, size, offset) != ARCHIVE_OK) {
                    qWarning().noquote() << u"libarchive write_data_block failed:"_qsv.arg(
                        svtostr(QUtf8SV(archive_error_string(disk)))
                    );
                    writeOk = false;
                    break;
                }
            }

            archive_write_finish_entry(disk);
            extracted = writeOk;
            break;
        }

        cleanup();

        if (!extracted) {
            present(this, NOTICE("Failed to extract update archive.", Warning, Modal));
            return;
        }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        QFile::setPermissions(
            appDir + u"/rpgmtranslate",
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup |
                QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther
        );
#endif

        qApp->quit();
        QProcess::startDetached(qApp->arguments()[0]);
    }, Qt::SingleShotConnection);

    connect(updater, &AutoUpdater::versionFetched, this, [=, this](const QString& version) -> void {
        const auto newVersion = QVersionNumber::fromString(version);
        const auto currentVersion = QVersionNumber::fromString(QL1SV(RPGMT_VERSION));

        if (newVersion <= currentVersion) {
            if (manual) {
                present(this, NOTICE("Program is up-to-date.", Info, Status));
            } else {
                qInfo().noquote() << u"Program is up-to-date."_qsv;
            }

            return;
        }

        auto msgBox = QMessageBox(this);
        auto* const checkbox = new QCheckBox(tr("Don't remind me"), &msgBox);
        msgBox.setWindowTitle(tr("New version is available"));
        msgBox.setText(
            tr("Version %1 is available.\nCurrent version is %2.").arg(version, QString::fromLatin1(RPGMT_VERSION))
        );
        const QPushButton* const installButton = msgBox.addButton(tr("Install"), QMessageBox::AcceptRole);
        const QPushButton* const skipButton = msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);
        msgBox.setCheckBox(checkbox);

        msgBox.exec();
        const auto* const clickedButton = msgBox.clickedButton();

        if (clickedButton == installButton) {
            updater->downloadUpdate();

            updateProgressDialog = new QProgressDialog(tr("Installing update..."), tr("Abort"), 0, 1, this);
            updateProgressDialog->setWindowModality(Qt::WindowModal);

            connect(updateProgressDialog, &QProgressDialog::canceled, this, [updater] -> void {
                updater->abortDownload();
            });

            connect(
                updater,
                &AutoUpdater::updateDownloadProgress,
                this,
                [this](const i32 received, const i32 total) -> void {
                updateProgressDialog->setMaximum(total);
                updateProgressDialog->setValue(received);
            }
            );
        } else if (clickedButton == skipButton) {
            if (checkbox->isChecked()) {
                settings->core.checkForAppUpdates = false;
            }

            updater->deleteLater();
        }
    }, Qt::SingleShotConnection);

    connect(updater, &AutoUpdater::updateFailed, this, [=, this](const QNetworkReply::NetworkError error) -> void {
        present(this, NOTICE("Update failed with error: %1", Warning, Modal, itos(scast<i32>(error)).qsv()));

        updater->deleteLater();
    });

    connect(
        ui->translationTable,
        &TranslationTable::multilineAction,
        this,
        [this](const TranslationTable::MultilineAction action, const u32 count) -> void {
        switch (action) {
            case TranslationTable::MultilineAction::Cut:
                present(this, NOTICE_N("%n row(s) cut.", nullptr, scast<i32>(count), Info, Status));
                break;
            case TranslationTable::MultilineAction::Copy:
                present(this, NOTICE_N("%n row(s) copied.", nullptr, scast<i32>(count), Info, Status));
                break;
            case TranslationTable::MultilineAction::Paste:
                present(this, NOTICE_N("%n row(s) pasted.", nullptr, scast<i32>(count), Info, Status));
                break;
        }
    }
    );

    updater->checkForUpdates();
}

// Awful
void MainWindow::openProject(const QString& folder, const bool newProject) {
    if (!QFile::exists(folder)) {
        present(this, NOTICE("Folder %1 does not exist.", Critical, Modal, folder));
        return;
    }

    QString tempFolder = folder;
    closeProject();
    settings->core.projectPath = std::move(tempFolder);

    if (!newProject && !QFile::exists(settings->core.projectPath + PROGRAM_DATA_DIRECTORY)) {
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

    const auto postRead =
        [this, folder, tempProjectSettings](const result<ByteBuffer, FFIString> result) -> ::result<void, FFIString> {
        if (!result) {
            return Err(result.error());
        }

        const auto hashes = result.value();

        if (hashes.ptr != nullptr) {
            const u8* const input = hashes.ptr;
            const u32 size = hashes.len;

            u32 cursor = 0;
            const u32 hashesCount = *rcast<const u32*>(input);
            cursor += 4;

            tempProjectSettings->hashes = {};
            tempProjectSettings->hashes.reserve(hashesCount);

            while (cursor < size) {
                const FilenameArray filename = *rcast<const FilenameArray*>(input + cursor);
                cursor += sizeof(FilenameArray);

                const u64 hash = *rcast<const u64*>(input + cursor);
                tempProjectSettings->hashes.insert({ filename, hash });
                cursor += sizeof(u64);
            }

            rpgm_buffer_free(hashes);
        }

        const QString projectSettingsPath = folder % PROGRAM_DATA_DIRECTORY % PROJECT_SETTINGS_FILE;

        auto projectSettingsFile = QFile(projectSettingsPath);

        if (projectSettingsFile.open(QFile::ReadOnly)) {
            const QByteArray json = projectSettingsFile.readAll();
            auto projSettings = glz::read_json<ProjectSettings>(string_view(json.data(), json.size()));

            if (projSettings) {
                projectSettings = make_shared<ProjectSettings>(std::move(projSettings.value()));
            } else {
                qWarning().noquote() << u"Parsing project-settings.json failed: %1"_qsv.arg(
                    svtostr(QUtf8SV(glz::format_error(projSettings.error(), string_view(json.data(), json.size()))))
                );
                //! Could use improper settings, if application aborted (because
                //! of crash, power outage etc.).
                // We guard against it by saving project settings in backup and
                // after creating them
                projectSettings = tempProjectSettings;
            }
        } else {
            qWarning().noquote() << u"Failed to open project-settings.json: %1"_qsv.arg(
                projectSettingsFile.errorString()
            );

            projectSettings = tempProjectSettings;
        }

        projectSettings->projectPath = folder;
        settings->core.projectPath = folder;

        QDir().mkdir(projectSettings->backupPath());
        QDir().mkdir(projectSettings->translationPath());

        i32 totalLines = 0;
        i32 totalTranslated = 0;

        u8 columnCount = max<u8>(2, projectSettings->columns.size());

        const auto translationFiles =
            QDir(projectSettings->translationPath()).entryInfoList({ u"*.txt"_s }, QDir::Files);

        vector<TabListItem> tabs;

        bool skippedFlag = false;

        for (const auto& fileInfo : translationFiles) {
            auto file = QFile(fileInfo.filePath());

            if (!file.open(QFile::ReadWrite)) {
                qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(fileInfo.filePath(), file.errorString());
                continue;
            }

            const QByteArray normalized =
                file.readAll().removeIf([](const char byte) -> bool { return byte == u'\r'; });

            file.seek(0);
            file.resize(normalized.size());
            file.write(normalized);

            QString basename = fileInfo.baseName();
            const bool isMap = basename.startsWith(u"map");
            const bool isSystem = basename.startsWith(u"system");

            const QString content = QString::fromUtf8(normalized);
            const auto contentView = QStringView(content);
            const u32 size = contentView.size();

            u32 pos = 0;
            u32 lineIndex = 0;

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

                u32 lineEnd = newlinePos;

                if (lineEnd > lineStart && contentView[lineEnd - 1] == u'\r') {
                    lineEnd--;
                }

                const QStringView line = contentView.sliced(lineStart, lineEnd - lineStart);
                pos = newlinePos + 1;

                if (isSystem && pos >= size) {
                    break;
                }

                if (line.trimmed().isEmpty()) {
                    continue;
                }

                columnCount = max<u8>(columnCount, line.count(SEPARATOR) + 1);

                if (line.startsWith(BOOKMARK_COMMENT)) {
                    const u32 left = line.indexOf(SEPARATOR) + SEPARATOR.size();
                    const isize right = line.indexOf(SEPARATOR, left);

                    bookmarkMenu->addBookmark(
                        basename,
                        right == -1 ? line.sliced(left) : line.sliced(left, right - left),
                        lineIndex
                    );

                    lineIndex++;
                    continue;
                }

                if (isMap && line.startsWith(ID_COMMENT)) {
                    if (!mapID.isEmpty()) {
                        if (fileTotal != 0) {
                            const QString tempMapName = u"map"_s + mapID;

                            tabs.emplace_back(
                                tempMapName,
                                fileTotal,
                                fileTranslated,
                                projectSettings->completedFiles.contains(basename)
                            );

                            totalLines += fileTotal;
                            totalTranslated += fileTranslated;

                            fileTotal = 0;
                            fileTranslated = 0;

                            mapSections.insert(
                                { stoa<u16>(mapID), content.sliced(mapSectionStart, lineStart - mapSectionStart) }
                            );
                        }
                    }

                    mapSectionStart = lineStart;
                    mapID = line.sliced(line.indexOf(SEPARATOR) + SEPARATOR.size());
                    const isize end = mapID.indexOf(SEPARATOR);

                    if (end != -1) {
                        mapID = mapID.sliced(0, end);
                    }

                    lineIndex++;
                    continue;
                }

                if (!line.startsWith(COMMENT_PREFIX)) {
                    fileTotal++;

                    const QSVList parts = lineParts(line, lineIndex, basename);

                    if (parts.isEmpty()) {
                        skippedFlag = true;
                        break;
                    }

                    const QStringView translation = getTranslation(parts).translation;

                    if (!translation.isEmpty()) {
                        fileTranslated++;
                    }
                }

                lineIndex++;
            }

            if (fileTotal == 0) {
                continue;
            }

            totalLines += fileTotal;
            totalTranslated += fileTranslated;

            if (isMap) {
                mapSections.insert({ stoa<u16>(mapID), content.sliced(mapSectionStart, size - mapSectionStart) });
                basename = u"map"_s + mapID;
            }

            tabs.emplace_back(basename, fileTotal, fileTranslated, projectSettings->completedFiles.contains(basename));

            if (isSystem && contentView.lastIndexOf(u"<!-- ID --><#>8") != -1) {
                const QStringView titleLine = contentView.sliced(contentView.lastIndexOf(u'\n') + 1);
                const QSVList parts = lineParts(titleLine, 0, basename);
                const QStringView translation = getTranslation(parts).translation;
                const QStringView source = getSource(parts);

                ui->gameTitleInput->setPlaceholderText(source.toString());

                if (translation.isEmpty()) {
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

        {
            auto glossaryFile = QFile(projectSettings->glossaryPath());

            if (glossaryFile.open(QFile::ReadOnly)) {
                const QByteArray json = glossaryFile.readAll();
                const auto result = glz::read_json<Glossary>(string_view(json.data(), json.size()));

                if (result) {
                    glossaryMenu->fill(result.value());
                } else {
                    qWarning().noquote() << u"Parsing glossary.json failed: %1"_qsv.arg(
                        svtostr(QUtf8SV(glz::format_error(result.error(), string_view(json.data(), json.size()))))
                    );
                }
            } else {
                qWarning().noquote() << u"Failed to open glossary.json: %1"_qsv.arg(glossaryFile.errorString());
            }
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

        assetMenu->init(projectSettings);
        readMenu->init(projectSettings);
        taskWorker->init(settings, projectSettings, &mapSections);

        ui->translationTable->init(settings.get(), projectSettings.get());

        actionTabPanel->setEnabled(true);
        actionSave->setEnabled(true);
        actionWrite->setEnabled(true);
        actionSearch->setEnabled(true);
        actionBatchMenu->setEnabled(true);
        actionGlossaryMenu->setEnabled(true);
        actionLintMenu->setEnabled(true);
        actionTranslationsMenu->setEnabled(true);
        actionBookmarkMenu->setEnabled(true);
        actionSourceControl->setEnabled(true);
        actionAssets->setEnabled(true);
        ui->rvpackerButton->setEnabled(true);
        ui->gameTitleInput->setEnabled(true);
        actionLocateProjectDir->setEnabled(true);
        actionSearchPanel->setEnabled(true);

        ui->globalProgressBar->setEnabled(true);
        ui->globalProgressBar->setMaximum(totalLines);
        ui->globalProgressBar->setValue(totalTranslated);

        if (settings->core.backup.enabled) {
            backupTimer.start((settings->core.backup.period * SECOND_MS));
        }

        if (!settings->core.recentProjects.contains(folder)) {
            settings->core.recentProjects.append(folder);
            const QAction* const action = ui->actionRecentProjects->menu()->addAction(folder);

            connect(action, &QAction::triggered, this, [this, folder] -> void { openProject(folder, true); });
        }

        if (settings->core.recentProjects.size() > MAX_RECENT_PROJECTS) {
            settings->core.recentProjects.removeFirst();
            ui->actionRecentProjects->menu()->removeAction(
                scast<QAction*>(ui->actionRecentProjects->menu()->children()[0])
            );
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

        const QString baselineSourcePath = projectSettings->baselineSourcePath();
        const QString sourcePath = projectSettings->sourcePath();

        if (QFile::exists(sourcePath)) {
            if (!QFile::exists(baselineSourcePath)) {
                // If there's no baseline, make the current source the baseline.
                present(
                    this,
                    NOTICE("Copying the data directory to .rpgmtranslate/baseline-data as a baseline.", Info, Status)
                );

                try {
                    fs::copy(
                        sourcePath.toStdString(),
                        baselineSourcePath.toStdString(),
                        fs::copy_options::recursive | fs::copy_options::overwrite_existing
                    );
                } catch (const fs::filesystem_error& error) {
                    present(
                        this,
                        NOTICE(
                            "Failed to copy %1 to %2 as a baseline data: %3. The original source data from the root will be used instead.",
                            Warning,
                            Modal,
                            sourcePath,
                            baselineSourcePath,
                            QUtf8SV(error.what())
                        )
                    );
                }
            } else {
                if (settings->core.checkForSourceUpdates) {
                    checkHashes();
                }
            }
        }

        firstReadPending = false;
        return {};
    };

    const auto postArchive =
        [this,
         tempProjectSettings,
         postRead](const QString& sourcePath, const QString& translationPath, const QString& title) -> void {
        tempProjectSettings->flags = readMenu->flags();
        tempProjectSettings->duplicateMode = readMenu->duplicateMode();

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
                title
            );
        }).then(this, [this, postRead, task](const ReadResult& result) -> void {
            taskWorker->finishTask(task);

            const auto postReadResult = postRead(result);

            if (!postReadResult) {
                present(this, NOTICE("Failed to load project: %1", Critical, Modal, ffitostr(postReadResult.error())));
            }
        });
    };

    const auto startOpening =
        [this, folder, rootTranslationPath, tempProjectSettings, postRead, postArchive] -> result<void, Notice> {
        if (QFile::exists(folder + u"/Data")) {
            tempProjectSettings->sourceDirectory = SourceDirectory::UppercaseData;
        }

        if (QFile::exists(folder + u"/data")) {
            tempProjectSettings->sourceDirectory = SourceDirectory::LowercaseData;
        }

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
                }
            }

            if (!copied) {
                QString archivePath;
                bool systemExists = false;

                if (QFile::exists(tempProjectSettings->sourcePath() + u"/System.json")) {
                    tempProjectSettings->engineType = EngineType::New;
                    systemExists = true;
                } else if (QFile::exists(tempProjectSettings->sourcePath() + u"/System.rvdata2")) {
                    tempProjectSettings->engineType = EngineType::VXAce;
                    systemExists = true;
                } else if (QFile::exists(tempProjectSettings->sourcePath() + u"/System.rvdata")) {
                    tempProjectSettings->engineType = EngineType::VX;
                    systemExists = true;
                } else if (QFile::exists(tempProjectSettings->sourcePath() + u"/System.rxdata")) {
                    tempProjectSettings->engineType = EngineType::XP;
                    systemExists = true;
                }

                if (!systemExists) {
                    bool archiveExists = false;

                    if (archivePath = tempProjectSettings->projectPath + u"/Game.rgssad"; QFile::exists(archivePath)) {
                        tempProjectSettings->engineType = EngineType::XP;
                        archiveExists = true;
                    } else if (
                        archivePath = tempProjectSettings->projectPath + u"/Game.rgss2a"; QFile::exists(archivePath)
                    ) {
                        tempProjectSettings->engineType = EngineType::VX;
                        archiveExists = true;
                    } else if (
                        archivePath = tempProjectSettings->projectPath + u"/Game.rgss3a"; QFile::exists(archivePath)
                    ) {
                        tempProjectSettings->engineType = EngineType::VXAce;
                        archiveExists = true;
                    }

                    if (!archiveExists) {
                        return Err(NOTICE("Source files, translation or archive file do not exist.", Critical, Modal));
                    }

                    tempProjectSettings->sourceDirectory = SourceDirectory::UppercaseData;
                }

                firstReadPending = true;

                readMenu->show();
                readMenu->move((width() / 2) - (readMenu->width() / 2), (height() / 2) - (readMenu->height() / 2));

                if (readMenu->exec(tempProjectSettings->projectPath, tempProjectSettings->engineType) !=
                    QDialog::Accepted) {
                    return Err(NOTICE("Read was rejected by user.", Info, Status));
                }

                if (!systemExists) {
                    const TaskToken task = startTask(TaskKind::ExtractArchive);

                    if (task == nullptr) {
                        return Err(NOTICE("Another task is already running.", Warning, Modal));
                    }

                    QtConcurrent::run([this, archivePath, folder] -> auto {
                        return taskWorker->extractArchive(archivePath, folder);
                    }).then(this, [this, tempProjectSettings, postArchive, task](const ExtractResult& result) -> void {
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
                    });
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
        linesStatusLabel->clear();
        progressStatusLabel->clear();
        tabNameStatusLabel->clear();
    } else {
        if (tabName == lockedFile) {
            present(this, NOTICE("File is currently processed and is being locked.", Warning, Modal));
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }

        const auto result = fileLines(QL1SV(tabName.toLatin1()), mapSections, projectSettings);

        if (!result) {
            present(this, NOTICE("Failed to open tab: %1.", Warning, Modal, result.error()));
            const QSignalBlocker blocker(ui->tabPanel);
            ui->tabPanel->changeTab(previousTabName);
            return;
        }

        QSVList lines = result.value().lines;

        const bool isSystem = tabName.startsWith(u"system");
        const bool gameTitleAbsent = !ui->gameTitleInput->placeholderText().isEmpty();

        ui->translationTable
            ->fill(views::drop(lines, isSystem && gameTitleAbsent ? 1 : 0), projectSettings->columns, tabName);

        linesStatusLabel->setText(
            tr("%1 Lines / %2 Comments")
                .arg(itos(lines.size()).qsv(), itos(lines.size() - ui->tabPanel->currentTotal()).qsv())
        );
        progressStatusLabel->setText(
            tr("%1 Translated / %2 Total")
                .arg(itos(ui->tabPanel->currentTranslated()).qsv(), itos(ui->tabPanel->currentTotal()).qsv())
        );
        tabNameStatusLabel->setText(tabName);

        // TODO(v1.x): Display total source words/characters in the status bar
    }
}

void MainWindow::closeProject() {
    const auto result = saveEverything();

    if (!result) {
        return;
    }

    glossaryMenu->hide();
    bookmarkMenu->hide();
    searchMenu->hide();
    batchMenu->hide();
    searchMenu->hide();
    ui->lintMenu->hide();
    translationsMenu->hide();
    assetMenu->hide();

    ui->tabPanel->changeTab(QString());

    mapSections.clear();

    ui->tabPanel->clear();

    glossaryMenu->clear();
    bookmarkMenu->clear();
    searchMenu->clear();
    batchMenu->clear();
    searchMenu->clear();
    ui->lintMenu->clear();
    translationsMenu->clear();
    assetMenu->clear();

    ui->searchPanel->clear();

    readMenu->clear();
    purgeMenu->clear();
    writeMenu->clear();
    serdeMenu->clear();

    ui->sourceControlDock->clear();

    actionTabPanel->setEnabled(false);
    actionSave->setEnabled(false);
    actionWrite->setEnabled(false);
    actionSearch->setEnabled(false);
    actionBatchMenu->setEnabled(false);
    actionGlossaryMenu->setEnabled(false);
    actionLintMenu->setEnabled(false);
    actionTranslationsMenu->setEnabled(false);
    actionBookmarkMenu->setEnabled(false);
    actionSourceControl->setEnabled(false);
    actionAssets->setEnabled(false);
    ui->rvpackerButton->setEnabled(false);

    ui->gameTitleInput->setEnabled(false);
    ui->gameTitleInput->clear();
    ui->gameTitleInput->setPlaceholderText(QString());

    actionLocateProjectDir->setEnabled(false);
    actionSearchPanel->setEnabled(false);

    ui->globalProgressBar->setMaximum(0);
    ui->globalProgressBar->setValue(0);
    ui->globalProgressBar->setEnabled(false);

    ui->statusBar->clearMessage();

    backupTimer.stop();

    projectSettings.reset();

    settings->core.projectPath = QString();
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

    const auto sourceFiles = QDirListing(
        sourcePath,
        { u"*.%1"_qsv.arg(projectSettings->engineExtension()) },
        QDirListing::IteratorFlag::FilesOnly
    );

    vector<FilenameArray> mismatchedFiles;
    mismatchedFiles.reserve(projectSettings->hashes.size());

    vector<FilenameArray> newFiles;
    newFiles.reserve(projectSettings->hashes.size());

    QStringList skippedFiles;

    bool hasAnyFile = false;

    for (const auto& sourceFile : sourceFiles) {
        hasAnyFile = true;

        const QString filename = sourceFile.fileName();

        if (filename.startsWith(u"Animations") || filename.startsWith(u"Tilesets") ||
            filename.startsWith(u"MapInfos")) {
            // Not important
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

    QString skippedText;

    if (!skippedFiles.isEmpty()) {
        skippedText = tr("\n\nSkipped files:\n%1").arg(skippedFiles.join(u'\n'));
    }

    const bool hasMismatched = !mismatchedFiles.empty();
    const bool hasNew = !newFiles.empty();

    if (hasMismatched || hasNew) {
        const auto buildFileList = [](const vector<FilenameArray>& files) -> QString {
            QList<char> result;

            for (const auto& file : files) {
                for (const char chr : file) {
                    if (chr == '\0') {
                        break;
                    }

                    result.append(chr);
                }

                result.append(',');
                result.append(' ');
            }

            if (!result.isEmpty()) {
                result.removeLast();
                result.removeLast();
            }

            return QString::fromUtf8(result.data(), result.size());
        };

        QString messageBody;

        if (hasMismatched) {
            messageBody += tr("Changed files: [%1]").arg(buildFileList(mismatchedFiles));
        }

        if (hasNew) {
            if (!messageBody.isEmpty()) {
                messageBody += u'\n';
            }

            messageBody += tr("New files: [%1]").arg(buildFileList(newFiles));
        }

        const QMessageBox::StandardButton button = QMessageBox::question(
            this,
            tr("Source files have been updated"),
            tr("%1\n\nThe files have been changed. "
               "Do you want to append any new text?%2")
                .arg(messageBody, skippedText)
        );

        if (button == QMessageBox::Yes) {
            qInfo().noquote() << u"Appending new text from updated source files was confirmed by user."_qsv;

            const QString baselineSourcePath = projectSettings->baselineSourcePath();

            try {
                fs::copy(
                    sourcePath.toStdString(),
                    baselineSourcePath.toStdString(),
                    fs::copy_options::recursive | fs::copy_options::overwrite_existing
                );
            } catch (const fs::filesystem_error& error) {
                present(
                    this,
                    NOTICE(
                        "Failed to copy %1 to %2 as a baseline data: %3. The original source data from the root will be used instead.",
                        Warning,
                        Modal,
                        sourcePath,
                        baselineSourcePath,
                        QUtf8SV(error.what())
                    )
                );
            }

            read(
                ReadMode::AppendDefault,
                projectSettings->duplicateMode,
                Selected(),
                projectSettings->flags,
                false,
                ui->gameTitleInput->placeholderText()
            );
        }
    } else {
        present(this, NOTICE("All source files are up-to-date.%1", Info, Status, skippedText));
    }
}

void MainWindow::read(
    const ReadMode readMode,
    const DuplicateMode duplicateMode,
    const Selected selected,
    const BaseFlags flags,
    const bool mapEvents,
    const QString& title
) {
    const QString sourcePath = projectSettings->actualSourcePath();
    const QString translationPath = projectSettings->translationPath();

    const TaskToken task = startTask(TaskKind::Read);

    if (task == nullptr) {
        return;
    }

    QtConcurrent::run([=, this] -> auto {
        return taskWorker->read(
            sourcePath,
            translationPath,
            readMode,
            projectSettings->engineType,
            duplicateMode,
            selected,
            flags,
            mapEvents,
            projectSettings->hashes,
            title
        );
    }).then(this, [this, task](const ReadResult& result) -> void {
        taskWorker->finishTask(task);

        if (!result) {
            present(this, NOTICE("Read failed: %1", Warning, Modal, ffitostr(result.error())));
            return;
        }

        const ByteBuffer hashes = result.value();

        if (hashes.ptr != nullptr) {
            const u8* const input = hashes.ptr;
            const u32 size = hashes.len;

            u32 cursor = 0;
            const u32 hashesCount = *rcast<const u32*>(input);
            cursor += 4;

            projectSettings->hashes = {};
            projectSettings->hashes.reserve(hashesCount);

            while (cursor < size) {
                const FilenameArray filename = *rcast<const FilenameArray*>(input + cursor);
                cursor += sizeof(FilenameArray);

                const u64 hash = *rcast<const u64*>(input + cursor);
                projectSettings->hashes.insert({ filename, hash });
                cursor += sizeof(u64);
            }

            rpgm_buffer_free(hashes);
        }

        openProject(projectSettings->projectPath, false);
    });
}

auto MainWindow::initDictionary() -> result<void, QString> {
    const QString path =
        qApp->property("data-location").toString() % u"/dictionaries"_qsv % projectSettings->spellcheckDictionary;

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
