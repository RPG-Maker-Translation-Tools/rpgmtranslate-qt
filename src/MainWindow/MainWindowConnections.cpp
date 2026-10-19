#include "AssetMenu.hpp"
#include "BackupSelectDialog.hpp"
#include "BatchMenu.hpp"
#include "BookmarkMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "PurgeMenu.hpp"
#include "ReadMenu.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "SerdeMenu.hpp"
#include "TaskPanel.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "WriteMenu.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>

namespace {
constexpr i32 LINT_DEBOUNCE_MS = 100;
constexpr i32 COUNTS_DEBOUNCE_MS = 1000;
constexpr i32 TRANSLATE_DEBOUNCE_MS = 300;
}  // namespace

void MainWindow::setupConnections() {
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exit);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutWindow);

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsWindow);

    connect(ui->actionCloseProject, &QAction::triggered, this, [this] -> void { closeProject(); });

    connect(ui->actionCloseTab, &QAction::triggered, this, [this] -> void { ui->tabPanel->changeTab(QString()); });

    connect(bookmarkMenu, &BookmarkMenu::bookmarkClicked, this, [this](const QL1SV file, const u32 row) -> void {
        ui->tabPanel->changeTab(file);

        QTimer::singleShot(1, [this, row] -> void {
            ui->translationTable->scrollTo(
                ui->translationTable->model()->index(scast<i32>(row), 0),
                TranslationTable::PositionAtCenter
            );
        });
    });

    connect(ui->actionDocumentation, &QAction::triggered, this, [] -> void {
        QDesktopServices::openUrl(QUrl(u"https://RPG-Maker-Translation-Tools.github.io/rpgmtranslate-qt"_s));
    });

    connect(ui->actionEnglish, &QAction::triggered, this, [this] -> void { retranslate(QLocale::English); });

    connect(ui->actionRussian, &QAction::triggered, this, [this] -> void { retranslate(QLocale::Russian); });

    connect(ui->actionChinese, &QAction::triggered, this, [this] -> void { retranslate(QLocale::Chinese); });

    connect(ui->actionCheckForUpdates, &QAction::triggered, this, [this] -> void { checkForUpdates(true); });

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

    connect(actionGoToRow, &QAction::triggered, this, &MainWindow::handleGoToRow);

    connect(ui->tabPanel, &TabPanel::tabChanged, this, &MainWindow::changeTab);

    connect(actionTabPanel, &QAction::triggered, this, [this] -> void {
        ui->tabPanel->setHidden(!ui->tabPanel->isHidden());
    });

    connect(actionSave, &QAction::triggered, this, [this] -> void {
        // The action is greyed out, but a menu opened earlier can still reach it. `saveMaps()` would overwrite maps a
        // task is rewriting, or race with it.
        if (const TaskToken blocking = taskWorker->blockingTask(TaskAccess::Reads); blocking != nullptr) {
            present(this, NOTICE("Cannot save while %1 is running.", Warning, Status, taskName(blocking->kind())));
            return;
        }

        if (!saveTranslations()) {
            return;
        }
    });

    connect(actionWrite, &QAction::triggered, this, &MainWindow::handleWrite);

    connect(actionSearch, &QAction::triggered, this, [this] -> void {
        searchMenu->setHidden(!searchMenu->isHidden());

        if (!searchMenu->mouseMoved()) {
            showBelow(searchMenu, ui->searchButton);
        }
    });

    connect(actionBatchMenu, &QAction::triggered, this, [this] -> void {
        batchMenu->setHidden(!batchMenu->isHidden());
        showBelow(batchMenu, ui->batchButton);
    });

    connect(actionGlossaryMenu, &QAction::triggered, this, [this] -> void {
        glossaryMenu->setHidden(!glossaryMenu->isHidden());

        if (!glossaryMenu->mouseMoved()) {
            showBelow(glossaryMenu, ui->glossaryButton);
        }
    });

    connect(actionStatistics, &QAction::triggered, this, [this] -> void { showProjectStatistics(); });

    connect(actionSingleTrim, &QAction::triggered, this, [this] -> void { applyCellAction(BatchAction::Trim); });
    connect(actionSingleWrap, &QAction::triggered, this, [this] -> void { applyCellAction(BatchAction::Wrap); });
    connect(actionSingleLatinize, &QAction::triggered, this, [this] -> void {
        applyCellAction(BatchAction::Latinize);
    });

    connect(actionLintMenu, &QAction::triggered, this, [this] -> void {
        ui->lintMenu->setHidden(!ui->lintMenu->isHidden());
    });

    connect(actionTranslationsMenu, &QAction::triggered, this, [this] -> void {
        translationsMenu->setHidden(!translationsMenu->isHidden());

        if (!translationsMenu->mouseMoved()) {
            showBelow(translationsMenu, ui->translationsButton);
        }
    });

    connect(actionBookmarkMenu, &QAction::triggered, this, [this] -> void {
        bookmarkMenu->setHidden(!bookmarkMenu->isHidden());

        showBelow(bookmarkMenu, ui->bookmarksButton);
    });

    connect(actionSourceControl, &QAction::triggered, this, [this] -> void {
#ifndef ENABLE_LIBGIT2
        present(
            this,
            NOTICE(
                "Program was compiled without support for libgit2, so it's impossible to access source control.",
                Warning,
                Modal
            )
        );
        return;
#endif

        ui->sourceControlDock->setHidden(!ui->sourceControlDock->isHidden());
    });

    connect(actionAssets, &QAction::triggered, this, [this] -> void {
        if (assetMenu->isHidden()) {
            showBelow(assetMenu, ui->assetsButton);
            assetMenu->show();
            assetMenu->adjustSize();
        } else {
            assetMenu->hide();
        }
    });

    connect(ui->rvpackerButton, &QPushButton::clicked, this, [this] -> void {
        auto menu = QMenu(this);

        QAction* const readAction = menu.addAction(tr("Read"));
        QAction* const purgeAction = menu.addAction(tr("Purge"));
        QAction* const serdeAction = menu.addAction(tr("Export/Import"));

        readAction->setEnabled(!taskWorker->blocks(TaskKind::Read));
        purgeAction->setEnabled(!taskWorker->blocks(TaskKind::Purge));
        serdeAction->setEnabled(!taskWorker->blocks(TaskKind::SerdeExport));

        const QAction* const selectedAction =
            menu.exec(ui->rvpackerButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));

        if (selectedAction == readAction) {
            readMenu->show();
            showBelow(readMenu, ui->rvpackerButton);
        } else if (selectedAction == purgeAction) {
            purgeMenu->show();
            showBelow(purgeMenu, ui->rvpackerButton);
        } else if (selectedAction == serdeAction) {
            serdeMenu->show();
            showBelow(serdeMenu, ui->rvpackerButton);
        }
    });

    connect(actionLocateProjectDir, &QAction::triggered, this, [this] -> void {
        QDesktopServices::openUrl(QUrl::fromLocalFile(projectSettings->projectPath));
    });

    connect(actionSearchPanel, &QAction::triggered, this, [this] -> void {
        ui->searchPanel->setHidden(!ui->searchPanel->isHidden());
    });

    connect(searchMenu, &SearchMenu::actionRequested, this, &MainWindow::handleSearchMenuAction);

    connect(ui->actionLoadBackup, &QAction::triggered, this, [this] -> void {
        if (!projectSettings) {
            return;
        }

        auto* const dialog = new BackupSelectDialog(projectSettings->backupPath(), this);

        // Deleted by itself when it closes
        connect(dialog, &BackupSelectDialog::accepted, this, [this, dialog] -> void {
            qInfo().noquote() << u"Loading a backup was confirmed by user."_qsv;

            // Loading rewrites the translation files, which every running task reads or rewrites as well
            if (taskWorker->idle()) {
                // What the editor holds is written first and the tab closed, then the project is reopened from the
                // restored files: otherwise the next save would put the editor's text and `mapSections` over them
                if (flushTranslations()) {
                    loadBackup(dialog->backupPath());
                    openProject(settings->core.projectPath, false, true);
                }
            } else {
                present(this, NOTICE("Cannot load a backup while a task is running.", Warning, Modal));
            }
        });
    });

    connect(ui->actionCheckSourceUpdates, &QAction::triggered, this, [this] -> void {
        if (!projectSettings) {
            return;
        }

        checkHashes();
    });

    connect(ui->translationTable, &TranslationTable::translatedChanged, this, [this](const i8 count) -> void {
        ui->globalProgressBar->setValue(ui->globalProgressBar->value() + count);
        ui->tabPanel->setCurrentTranslated(count);
        updateStatusLabels();
    });

    connect(ui->translationTable, &TranslationTable::bookmarkChanged, this, [this](const u32 row) -> void {
        bookmarkMenu->updateBookmark(
            ui->tabPanel->currentTabName(),
            row,
            *ui->translationTable->model()->item(scast<i32>(row), 1)
        );
    });

    connect(ui->translationTable, &TranslationTable::columnAdded, this, [this] -> void {
        projectSettings->columns.emplace_back(tr("Translation"), DEFAULT_COLUMN_WIDTH);
        searchMenu->addColumn(tr("Translation"));
        batchMenu->addColumn(tr("Translation"));
    });

    connect(
        ui->translationTable,
        &TranslationTable::columnRenamed,
        this,
        [this](const i32 index, const QString& name) -> void {
        // A file edited outside can have more columns than the project knows
        if (scast<usize>(index) >= projectSettings->columns.size()) {
            return;
        }

        projectSettings->columns[index].name = name;
        searchMenu->renameColumn(index, name);
        batchMenu->renameColumn(index, name);
    }
    );

    connect(
        ui->translationTable,
        &TranslationTable::columnResized,
        this,
        [this](const i32 index, const u16 width) -> void {
        if (scast<usize>(index) < projectSettings->columns.size()) {
            projectSettings->columns[index].width = width;
        }
    }
    );

    connect(ui->translationTable, &TranslationTable::rowRemoved, this, &MainWindow::handleRowRemoved);

    connect(ui->tabPanel, &TabPanel::displayToggled, this, [this] -> void {
        settings->appearance.displayPercents = !settings->appearance.displayPercents;
    });

    connect(
        ui->tabPanel,
        &TabPanel::completedToggled,
        this,
        [this](const QString& tabName, const bool completed) -> void {
        if (completed) {
            projectSettings->completedFiles.append(tabName);
        } else {
            projectSettings->completedFiles.removeIf([&tabName](const QString& pred) -> bool {
                return pred == tabName;
            });
        }
    }
    );

    connect(batchMenu, &BatchMenu::actionRequested, this, &MainWindow::handleBatchMenuAction);

    connect(ui->searchPanel, &SearchPanelDock::actionRequested, this, &MainWindow::handleSearchPanelAction);

    connect(ui->lintMenu, &LintMenu::checkRequested, this, &MainWindow::handleGlobalLint);

    connect(ui->translationTable, &TranslationTable::textChanged, this, [this] -> void {
        lintDebounceTimer.start(LINT_DEBOUNCE_MS);
    });

    connect(ui->translationTable, &TranslationTable::inputFocused, this, [this] -> void {
        // Invalidates the request in flight, as its cell is not the current one anymore
        translateGeneration++;
        translateDebounceTimer.start(TRANSLATE_DEBOUNCE_MS);
    });

    connect(&translateDebounceTimer, &QTimer::timeout, this, &MainWindow::handleInputFocused);

    connect(ui->translationTable, &TranslationTable::bookmarked, this, [this](const u32 row) -> void {
        // The entry is that of the nearest `<!>NAME` row above the bookmark, up to the `<!>ID` row starting the entry.
        auto* const model = ui->translationTable->model();
        QString entry;

        for (i32 idx = scast<i32>(row) - 1; idx >= 0; idx--) {
            const QStringView source = *model->item(idx, 0);

            if (const auto name = entryNameOfLine(source)) {
                entry = name->toString();
                break;
            }

            if (source.startsWith(ID_COMMENT)) {
                break;
            }
        }

        bookmarkMenu->addBookmark(QStringView(), ui->tabPanel->currentTabName(), row, entry);
    });

    connect(ui->actionOpenFolder, &QAction::triggered, this, [this] -> void {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Select a game folder"));

        if (dir.isEmpty()) {
            return;
        }

        openProject(dir, true);
    });

    connect(readMenu, &ReadMenu::accepted, this, [this] -> void {
        if (firstReadPending) {
            return;
        }

        if (readMenu->readMode() == ReadMode::DefaultForce) {
            if (QMessageBox::question(
                    this,
                    tr("Force Read"),
                    tr("Force read rewrites the existing translation files. Continue?")
                ) != QMessageBox::Yes) {
                return;
            }

            qInfo().noquote() << u"Force read was confirmed by user."_qsv;
        }

        read(
            readMenu->readMode(),
            readMenu->duplicateMode(),
            readMenu->selected(true),
            readMenu->flags(),
            readMenu->parseMapEvents(),
            readMenu->title(),
            readMenu->readEncoding()
        );
    });

    connect(purgeMenu, &PurgeMenu::accepted, this, [this] -> void {
        // With the ignore file the purged lines are kept in it
        if (!purgeMenu->createIgnore()) {
            if (QMessageBox::question(
                    this,
                    tr("Purge"),
                    tr("Purge removes the translation lines that the game no longer uses. Continue?")
                ) != QMessageBox::Yes) {
                return;
            }

            qInfo().noquote() << u"Purge was confirmed by user."_qsv;
        }

        const TaskToken task = startTask(TaskKind::Purge);

        if (task == nullptr) {
            return;
        }

        if (!flushTranslations()) {
            taskWorker->finishTask(task);
            return;
        }

        const Selected selected = purgeMenu->selected(true);
        const bool createIgnore = purgeMenu->createIgnore();

        QtConcurrent::run([this, selected, createIgnore] -> auto { return taskWorker->purge(selected, createIgnore); })
            .then(this, [this, task](const PurgeResult& result) -> void {
            taskWorker->finishTask(task);

            // `openProject` below would run inside the `closeProject` that is waiting for the task
            if (stoppingTasks) {
                return;
            }

            if (!result) {
                present(this, NOTICE("Purge failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            // The task rewrote the translation files, and `flushTranslations()` left nothing in the editor to save
            openProject(settings->core.projectPath, false, true);
        }).onFailed(this, [this, task] -> void { taskFailed(task); });
    });

    connect(serdeMenu, &SerdeMenu::exportRequested, this, [this] -> void {
        const TaskToken task = startTask(TaskKind::SerdeExport);

        if (task == nullptr) {
            return;
        }

        // The export reads the translation files, which must hold what is edited
        if (!saveTranslations()) {
            taskWorker->finishTask(task);
            return;
        }

        auto filenames = serdeMenu->selected().filenames(projectSettings->engineType);
        const QString directory = serdeMenu->directory();
        const SerdeFormat format = serdeMenu->format();

        QtConcurrent::run([this, task, filenames = std::move(filenames), directory, format]() mutable -> auto {
            return taskWorker->serdeExport(task, filenames, directory, format);
        })
            .then(this, [this, task](const SerdeResult& result) -> void {
            taskWorker->finishTask(task);

            if (stoppingTasks) {
                return;
            }

            if (!result) {
                present(this, NOTICE("Export failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            reportSkippedFiles(result.value().filenames, scast<u16>(result.value().skippedCount));
        }).onFailed(this, [this, task] -> void { taskFailed(task); });
    });

    connect(serdeMenu, &SerdeMenu::importRequested, this, [this] -> void {
        auto filenames = serdeMenu->selected().filenames(projectSettings->engineType);
        const QString directory = serdeMenu->directory();
        const SerdeFormat format = serdeMenu->format();

        if (QMessageBox::question(
                this,
                tr("Import"),
                tr("Import replaces the translation of %n file(s) with the imported one. Continue?",
                   nullptr,
                   scast<i32>(filenames.size()))
            ) != QMessageBox::Yes) {
            return;
        }

        qInfo().noquote() << u"Serde import was confirmed by user."_qsv;

        const TaskToken task = startTask(TaskKind::SerdeImport);

        if (task == nullptr) {
            return;
        }

        // The import rewrites these files, so what is edited goes to disk first and the table is emptied: the reload
        // that follows must not save the tab over what the import wrote
        if (!flushTranslations()) {
            taskWorker->finishTask(task);
            return;
        }

        QtConcurrent::run([this, task, filenames = std::move(filenames), directory, format]() mutable -> auto {
            return taskWorker->serdeImport(task, filenames, directory, format);
        })
            .then(this, [this, task](const SerdeResult& result) -> void {
            taskWorker->finishTask(task);

            if (stoppingTasks) {
                return;
            }

            if (!result) {
                present(this, NOTICE("Import failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            reportSkippedFiles(result.value().filenames, scast<u16>(result.value().skippedCount));

            // Not `translationsRewritten`: the imported maps exist only in `mapSections`, which the reload has to
            // save. The other files were written by the task, and the tab is closed.
            openProject(settings->core.projectPath, false);
        }).onFailed(this, [this, task] -> void { taskFailed(task); });
    });

    connect(writeMenu, &WriteMenu::accepted, this, [this] -> void { write(writeMenu->selected(true)); });

    // A backup would archive translation files that Read or Purge are in the middle of rewriting
    connect(&backupTimer, &QTimer::timeout, this, [this] -> void {
        if (rewritingTask() == nullptr) {
            saveBackup();
        }
    });

    connect(taskWorker, &TaskWorker::lockFile, this, [this](const QString& lockFile) -> void {
        lockedFile = lockFile;
    });

    connect(taskWorker, &TaskWorker::taskStarted, this, [this](const TaskToken& task) -> void {
        taskPanel->addTask(task);
        updateTaskControls();
    });

    connect(
        taskWorker,
        &TaskWorker::taskProgress,
        this,
        [this](const u32 taskId, const QString& filename, const u32 done, const u32 total) -> void {
        taskPanel->updateTask(taskId, filename, done, total);
    }
    );

    connect(taskWorker, &TaskWorker::taskFinished, this, [this](const u32 taskId) -> void {
        taskPanel->removeTask(taskId);
        updateTaskControls();
    });

    connect(taskPanel, &TaskPanel::taskCountChanged, this, [this](const i32 count) -> void {
        updateTaskButton(count);

        if (count == 0) {
            taskPanel->hide();
        }
    });

    connect(translationsMenu, &TranslationsMenu::translationClicked, this, [this](const QString& translation) -> void {
        ui->translationTable->insertTranslation(translation);
    });

    connect(&ffiLogger, &FFILogger::logReceived, this, &MainWindow::logFFI);

    connect(ui->taskButton, &QToolButton::pressed, this, [this] -> void {
        if (taskPanel->taskCount() == 0) {
            return;
        }

        taskPanel->adjustSize();
        taskPanel->move(ui->taskButton->mapToGlobal(QPoint(0, ui->taskButton->height())));
        taskPanel->show();
    });

    connect(&lintDebounceTimer, &QTimer::timeout, this, [this] -> void { handleSingleLint(); });

    // Counts follow the committed cell text, so they are refreshed a moment after the table model changes.
    connect(ui->translationTable->model(), &QAbstractItemModel::dataChanged, this, [this] -> void {
        if (settings->translation.displayWordsAndCharacters) {
            countsDebounceTimer.start(COUNTS_DEBOUNCE_MS);
        }
    });
    connect(&countsDebounceTimer, &QTimer::timeout, this, [this] -> void { updateCounts(); });

    for (const auto& recentProject : settings->core.recentProjects) {
        addRecentProject(recentProject);
    }
}

void MainWindow::addRecentProject(const QString& folder) {
    const QAction* const action = recentProjectsMenu->addAction(folder);

    connect(action, &QAction::triggered, this, [this, folder] -> void { openProject(folder, true); });
}

void MainWindow::showBelow(QWidget* const panel, const QWidget* const button) const {
    panel->move(button->mapToGlobal(QPoint(0, ui->controlContainer->height())));
}