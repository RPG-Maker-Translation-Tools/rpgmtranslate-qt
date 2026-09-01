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
#include <QtConcurrent>

namespace {
constexpr i32 LINT_DEBOUNCE_MS = 100;
}  // namespace

void MainWindow::setupConnections() {
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exit);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutWindow);

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsWindow);

    connect(ui->actionCloseProject, &QAction::triggered, this, &MainWindow::closeProject);

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

    connect(actionGoToRow, &QAction::triggered, this, &MainWindow::handleGoToRow);

    connect(ui->tabPanel, &TabPanel::tabChanged, this, &MainWindow::changeTab);

    connect(actionTabPanel, &QAction::triggered, this, [this] -> void {
        ui->tabPanel->setHidden(!ui->tabPanel->isHidden());
    });

    connect(actionSave, &QAction::triggered, this, [this] -> void {
        const auto saveSuccess = saveCurrentTab();

        if (!saveSuccess) {
            return;
        }

        const auto saveSuccess2 = saveMaps();

        if (!saveSuccess2) {
            return;
        }
    });

    connect(actionWrite, &QAction::triggered, this, &MainWindow::handleWrite);

    connect(actionSearch, &QAction::triggered, this, [this] -> void {
        searchMenu->setHidden(!searchMenu->isHidden());

        if (!searchMenu->mouseMoved()) {
            searchMenu->move(ui->searchButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
        }
    });

    connect(actionBatchMenu, &QAction::triggered, this, [this] -> void {
        batchMenu->setHidden(!batchMenu->isHidden());
        batchMenu->move(ui->batchButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
    });

    connect(actionGlossaryMenu, &QAction::triggered, this, [this] -> void {
        glossaryMenu->setHidden(!glossaryMenu->isHidden());

        if (!glossaryMenu->mouseMoved()) {
            glossaryMenu->move(ui->glossaryButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
        }
    });

    connect(actionLintMenu, &QAction::triggered, this, [this] -> void {
        ui->lintMenu->setHidden(!ui->lintMenu->isHidden());
    });

    connect(actionTranslationsMenu, &QAction::triggered, this, [this] -> void {
        translationsMenu->setHidden(!translationsMenu->isHidden());

        if (!translationsMenu->mouseMoved()) {
            translationsMenu->move(ui->translationsButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
        }
    });

    connect(actionBookmarkMenu, &QAction::triggered, this, [this] -> void {
        bookmarkMenu->setHidden(!bookmarkMenu->isHidden());

        bookmarkMenu->move(ui->bookmarksButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
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
            assetMenu->move(ui->assetsButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
            assetMenu->show();
            assetMenu->adjustSize();
        } else {
            assetMenu->hide();
        }
    });

    connect(ui->rvpackerButton, &QPushButton::pressed, this, [this] -> void {
        auto menu = QMenu(this);

        const QAction* const readAction = menu.addAction(tr("Read"));
        const QAction* const purgeAction = menu.addAction(tr("Purge"));
        const QAction* const serdeAction = menu.addAction(tr("Export/Import"));

        const QAction* const selectedAction =
            menu.exec(ui->rvpackerButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));

        if (selectedAction == readAction) {
            readMenu->show();
            readMenu->move(ui->rvpackerButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
        } else if (selectedAction == purgeAction) {
            purgeMenu->show();
            purgeMenu->move(ui->rvpackerButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
        } else if (selectedAction == serdeAction) {
            serdeMenu->show();
            serdeMenu->move(ui->rvpackerButton->mapToGlobal(QPoint(0, ui->controlContainer->height())));
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

        connect(dialog, &BackupSelectDialog::accepted, this, [this, dialog] -> void {
            loadBackup(dialog->backupPath());
            delete dialog;
        });

        connect(dialog, &BackupSelectDialog::rejected, this, [dialog] -> void { delete dialog; });
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
        progressStatusLabel->setText(
            tr("%1 Translated / %2 Total")
                .arg(itos(ui->tabPanel->currentTranslated()).qsv(), itos(ui->tabPanel->currentTotal()).qsv())
        );
    });

    connect(ui->translationTable, &TranslationTable::bookmarkChanged, this, [this](const u32 row) -> void {
        bookmarkMenu->updateBookmark(row, *ui->translationTable->model()->item(scast<i32>(row), 1).text());
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
        projectSettings->columns[index].name = name;
        searchMenu->renameColumn(index, name);
        batchMenu->renameColumn(index, name);
    }
    );

    connect(
        ui->translationTable,
        &TranslationTable::columnResized,
        this,
        [this](const i32 index, const u16 width) -> void { projectSettings->columns[index].width = width; }
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

    connect(ui->translationTable, &TranslationTable::inputFocused, this, &MainWindow::handleInputFocused);

    connect(ui->translationTable, &TranslationTable::bookmarked, this, [this](const u32 row) -> void {
        bookmarkMenu->addBookmark(ui->tabPanel->currentTabName(), QStringView(), row - 1);
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
        const TaskToken task = startTask(TaskKind::Purge);

        if (task == nullptr) {
            return;
        }

        const Selected selected = purgeMenu->selected(true);

        QtConcurrent::run([this, selected] -> auto {
            return taskWorker->purge(selected);
        }).then(this, [this, task](const PurgeResult& result) -> void {
            taskWorker->finishTask(task);

            if (!result) {
                present(this, NOTICE("Purge failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            openProject(settings->core.projectPath, false);
        });
    });

    connect(serdeMenu, &SerdeMenu::exportRequested, this, [this] -> void {
        const TaskToken task = startTask(TaskKind::SerdeExport);

        if (task == nullptr) {
            return;
        }

        auto filenames = serdeMenu->selected(true).filenames(projectSettings->engineType);
        const QString directory = serdeMenu->directory();
        const SerdeFormat format = serdeMenu->format();

        QtConcurrent::run([this, task, filenames = std::move(filenames), directory, format]() mutable -> auto {
            return taskWorker->serdeExport(task, filenames, directory, format);
        }).then(this, [this, task](const SerdeResult& result) -> void {
            taskWorker->finishTask(task);

            if (!result) {
                present(this, NOTICE("Export failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            reportSkippedFiles(result.value().filenames, scast<u16>(result.value().skippedCount));
        });
    });

    connect(serdeMenu, &SerdeMenu::importRequested, this, [this] -> void {
        const TaskToken task = startTask(TaskKind::SerdeImport);

        if (task == nullptr) {
            return;
        }

        auto filenames = serdeMenu->selected(true).filenames(projectSettings->engineType);
        const QString directory = serdeMenu->directory();
        const SerdeFormat format = serdeMenu->format();

        QtConcurrent::run([this, task, filenames = std::move(filenames), directory, format]() mutable -> auto {
            return taskWorker->serdeImport(task, filenames, directory, format);
        }).then(this, [this, task](const SerdeResult& result) -> void {
            taskWorker->finishTask(task);

            if (!result) {
                present(this, NOTICE("Import failed with error: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            reportSkippedFiles(result.value().filenames, scast<u16>(result.value().skippedCount));

            openProject(settings->core.projectPath, false);
        });
    });

    connect(writeMenu, &WriteMenu::accepted, this, [this] -> void {
        const TaskToken task = startTask(TaskKind::Write);

        if (task == nullptr) {
            return;
        }

        const Selected selected = writeMenu->selected(true);

        QtConcurrent::run([this, selected] -> auto {
            return taskWorker->write(selected);
        }).then(this, [this, task](const WriteResult& result) -> void {
            taskWorker->finishTask(task);

            if (!result) {
                present(this, NOTICE("Write failed: %1", Warning, Modal, ffitostr(result.error())));
                return;
            }

            present(this, NOTICE("Written successfully. Elapsed: %1s.", Info, Status, ftos(result.value(), 2).qsv()));
        });
    });

    connect(&backupTimer, &QTimer::timeout, this, [this] -> void { saveBackup(); });

    connect(taskWorker, &TaskWorker::lockFile, this, [this](const QString& lockFile) -> void {
        lockedFile = lockFile;
    });

    connect(taskWorker, &TaskWorker::taskStarted, this, [this](const TaskToken& task) -> void {
        taskPanel->addTask(task);
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
    });

    connect(taskPanel, &TaskPanel::taskCountChanged, this, [this](const i32 count) -> void {
        ui->taskButton->setEnabled(count != 0);
        ui->taskButton->setText(count == 0 ? tr("No tasks") : tr("%n task(s)", nullptr, count));

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

    for (const auto& recentProject : settings->core.recentProjects) {
        const QAction* const action = recentProjectsMenu->addAction(recentProject);

        connect(action, &QAction::triggered, this, [this, recentProject] -> void { openProject(recentProject, true); });
    }
}