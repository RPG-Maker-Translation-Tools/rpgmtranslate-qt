#include "AboutWindow.hpp"
#include "BookmarkMenu.hpp"
#include "Constants.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "PopupInput.hpp"
#include "ScriptHost.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "SettingsWindow.hpp"
#include "StatisticsDialog.hpp"
#include "TaskPanel.hpp"
#include "TranslationInput.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>

namespace {
constexpr i32 GO_TO_ROW_INPUT_MAX_LENGTH = 10;
constexpr i32 GO_TO_ROW_INPUT_WIDTH = 256;
constexpr i32 GO_TO_ROW_INPUT_X_OFFSET = 128;
constexpr i32 GO_TO_ROW_INPUT_Y_OFFSET = 64;

// The tasks report their per-file progress as log lines that end like this. Must match the wording in the Rust core.
struct ProgressSuffix {
    QStringView suffix;
    TaskKind kind;
};

constexpr array<ProgressSuffix, 3> PROGRESS_SUFFIXES = { {
    { .suffix = u"purged.", .kind = TaskKind::Purge },
    { .suffix = u"written.", .kind = TaskKind::Write },
    { .suffix = u"read.", .kind = TaskKind::Read },
} };
// The translated rows of a translation file or of a map section, counted as `openProject` counts them
[[nodiscard]] auto countTranslatedRows(const QString& content, const QL1SV basename) -> i32 {
    const QStringView view = content;
    const isize size = view.size();

    const isize titleMarker = (basename.startsWith("system"_L1) || basename.startsWith("terms"_L1))
                                  ? view.lastIndexOf(GAME_TITLE_MARKER)
                                  : -1;

    // The game title is the line after its marker, which is the last line: it is not a row
    const bool holdsTitle = titleMarker != -1 && titleMarker + GAME_TITLE_MARKER.size() + 1 < size;

    i32 translated = 0;
    isize pos = 0;

    for (u32 lineNumber = 0; pos < size; lineNumber++) {
        isize newlinePos = view.indexOf(u'\n', pos);

        if (newlinePos == -1) {
            newlinePos = size;
        }

        const QStringView line = view.sliced(pos, newlinePos - pos);
        pos = newlinePos + 1;

        if (holdsTitle && pos >= size) {
            break;
        }

        if (line.trimmed().isEmpty() || !isRowLine(line)) {
            continue;
        }

        if (hasTranslation(lineParts(line, lineNumber, basename))) {
            translated++;
        }
    }

    return translated;
}
}  // namespace

void MainWindow::logFFI(const u8 level, const QString& message) {
    switch (level) {
        case 0:
            qCritical().noquote() << message;
            break;
        case 1:
            qWarning().noquote() << message;
            break;
        case 2:
            qInfo().noquote() << message;
            break;
        default:
            qDebug().noquote() << message;
            break;
    }

    const auto progress = ranges::find_if(PROGRESS_SUFFIXES, [&message](const ProgressSuffix& entry) -> bool {
        return message.endsWith(entry.suffix);
    });

    if (progress == PROGRESS_SUFFIXES.end()) {
        return;
    }

    const TaskToken task = taskWorker->runningTask(progress->kind);

    if (task == nullptr) {
        return;
    }

    // "[target] filename: text": the name of the file is between the first space and the last colon
    const isize nameStart = message.indexOf(u' ') + 1;
    const isize nameEnd = message.lastIndexOf(u':');

    if (nameStart == 0 || nameEnd <= nameStart) {
        return;
    }

    taskPanel->updateTask(task->id(), QStringView(message).sliced(nameStart, nameEnd - nameStart).toString(), 0, 0);
}

void MainWindow::handleGoToRow() {
    if (ui->tabPanel->currentTabName().isEmpty() || goToRowPopup != nullptr) {
        return;
    }

    auto* const popupInput = new PopupInput(this);
    goToRowPopup = popupInput;
    popupInput->setValidator(new QIntValidator(0, INT32_MAX, popupInput));
    popupInput->setMaxLength(GO_TO_ROW_INPUT_MAX_LENGTH);
    popupInput->setFixedWidth(GO_TO_ROW_INPUT_WIDTH);
    const QString linesStatusText = linesStatusLabel->text();
    popupInput->setPlaceholderText(
        tr("Enter line from %1 to %2").arg(u"1", QStringView(linesStatusText).slice(0, linesStatusText.indexOf(u" ")))
    );

    // A child of the window, so its position is relative to it
    popupInput->move((width() / 2) - GO_TO_ROW_INPUT_X_OFFSET, GO_TO_ROW_INPUT_Y_OFFSET);
    popupInput->show();
    popupInput->setFocus();

    // The popup emits these from its own event handlers, so it is deleted after them. Disconnected first: hiding it
    // loses the focus, which would finish the editing once more.
    const auto dismiss = [this, popupInput] -> void {
        popupInput->disconnect(this);
        popupInput->hide();
        popupInput->deleteLater();
    };

    connect(popupInput, &PopupInput::escapePressed, this, dismiss);

    connect(popupInput, &PopupInput::editingFinished, this, [this, popupInput, dismiss] -> void {
        const i32 rowIndex = stoa<i32>(popupInput->text()).value_or(0) - 1;

        dismiss();

        ui->translationTable->scrollTo(
            ui->translationTable->model()->index(rowIndex, 0),
            TranslationTable::PositionAtCenter
        );
    });
}

void MainWindow::handleWrite() {
    write(Selected());
}

void MainWindow::write(const Selected selected) {
    const QString sourcePath = projectSettings->actualSourcePath();

    if (!QFile::exists(sourcePath)) {
        present(this, NOTICE("Cannot write, source files are absent.", Warning, Modal));
        return;
    }

    const TaskToken task = startTask(TaskKind::Write);

    if (task == nullptr) {
        return;
    }

    // The write reads the translation files, which must hold what is edited
    if (!saveTranslations()) {
        taskWorker->finishTask(task);
        return;
    }

    QtConcurrent::run([this, selected] -> auto { return taskWorker->write(selected); })
        .then(this, [this, task](const WriteResult& result) -> void {
        taskWorker->finishTask(task);

        if (stoppingTasks) {
            return;
        }

        if (!result) {
            present(this, NOTICE("Write failed: %1", Warning, Modal, ffitostr(result.error())));
            return;
        }

        present(
            this,
            NOTICE(
                "Written successfully. Elapsed: %1s. Check `.rpgmtranslate/output` for output files.",
                Info,
                Status,
                qsv(qftos(result.value(), 2))
            )
        );
    }).onFailed(this, [this, task] -> void { taskFailed(task); });
}

void MainWindow::handleSearchMenuAction(
    const Selected selected,
    const SearchAction action,
    const QString& searchText,
    const QString& replaceText,
    const SearchLocation searchLocation,
    const i8 columnIndex,
    const SearchFlags searchFlags
) {
    const auto saveSuccess = saveCurrentTab();

    if (!saveSuccess) {
        return;
    }

    const TaskToken searchTask = startTask(TaskKind::Search);

    if (searchTask == nullptr) {
        return;
    }

    const u16 tabCount = ui->tabPanel->tabCount();

    QtConcurrent::run([=, this] -> auto {
        return taskWorker
            ->search(searchTask, action, selected, searchText, searchLocation, columnIndex, searchFlags, tabCount);
    })
        .then(this, [=, this](const SearchResult& searchResult) -> void {
        // Moved out of the future's store instead of copied; nothing else reads the result
        auto& results = ccast<SearchResult&>(searchResult);

        taskWorker->finishTask(searchTask);

        if (stoppingTasks) {
            return;
        }

        if (results.skippedCount != 0) {
            reportSkippedFiles(results.filenames, results.skippedCount);
        }

        if (action == SearchAction::Search) {
            ui->searchPanel->showMatches(std::move(results.matches), projectSettings.get());
            return;
        }

        if (results.matches.empty()) {
            present(this, NOTICE("Nothing found to replace.", Info, Status));
            return;
        }

        // The files are rewritten at once, so say how much of them
        u32 cellCount = 0;

        for (const auto& fileMatches : views::values(results.matches)) {
            cellCount += fileMatches.cells.size();
        }

        const auto confirmed = QMessageBox::question(
            this,
            tr("Confirmation"),
            tr("Replace the text in %n cell(s) of %1 file(s)?", nullptr, scast<i32>(cellCount))
                .arg(qsv(qitos(results.matches.size())))
        );

        if (confirmed != QMessageBox::Yes) {
            qInfo().noquote() << u"Replacing in all found cells was rejected by user."_qsv;
            return;
        }

        qInfo().noquote() << u"Replacing in all found cells was confirmed by user."_qsv;

        const TaskToken replaceTask = startTask(TaskKind::Replace);

        if (replaceTask == nullptr) {
            return;
        }

        // The files are replaced on disk, and the editor would save its own text over them
        if (!closeTabIfSelected(selected)) {
            taskWorker->finishTask(replaceTask);
            return;
        }

        QtConcurrent::run([=, this, matches = std::move(results.matches)] -> auto {
            return taskWorker->replace(replaceTask, matches, action, replaceText);
        })
            .then(this, [this, replaceTask](const ReplaceResult& replaced) -> void {
            taskWorker->finishTask(replaceTask);

            if (stoppingTasks) {
                return;
            }

            // The progress counters follow what the replacement did to the translations
            for (const auto& [filename, delta] : replaced.deltas) {
                applyTranslatedDelta(QString::fromLatin1(filename.data()), delta);
            }

            if (!replaced.skipped.empty()) {
                reportSkippedFiles(replaced.skipped, scast<u16>(replaced.skipped.size()));
            }
        }).onFailed(this, [this, replaceTask] -> void { taskFailed(replaceTask); });
    }).onFailed(this, [this, searchTask] -> void { taskFailed(searchTask); });
}

// A replacement changed the number of rows with a translation in a file that is not open: the counters of its tab and
// of the project follow, as they are computed once when the project opens.
void MainWindow::applyTranslatedDelta(const QString& tabName, const i32 delta) {
    if (delta == 0) {
        return;
    }

    for (i32 idx = 0; idx < ui->tabPanel->tabCount(); idx++) {
        if (ui->tabPanel->tabName(idx) == tabName) {
            ui->tabPanel->setTabTranslated(idx, ui->tabPanel->tabTranslated(idx) + delta);
            ui->globalProgressBar->setValue(ui->globalProgressBar->value() + delta);
            return;
        }
    }
}

// A task rewrote rows of tabs that are not open. Their translated counts are computed again from what they hold now,
// the maps it changed are saved, and the search results and lints, which show the rows as they were, are cleared.
// A map is read from `mapSections`, which holds it until it is saved.
void MainWindow::refreshRewrittenTabs(const vector<FilenameArray>& filenames) {
    for (const auto& filenameArray : filenames) {
        const auto filename = QL1SV(filenameArray.data());
        const QString tabName = svtostr(filename);

        i32 tabIndex = 0;

        while (tabIndex < ui->tabPanel->tabCount() && ui->tabPanel->tabName(tabIndex) != tabName) {
            tabIndex++;
        }

        // No tab: the file has no rows to count
        if (tabIndex == ui->tabPanel->tabCount()) {
            continue;
        }

        QString content;

        if (filename.startsWith("map"_L1)) {
            const std::scoped_lock lock(mapSectionsMutex);
            const auto section = mapSections.find(stoa<u16>(filename.sliced(3)).value_or(0));

            if (section == mapSections.end()) {
                continue;
            }

            content = section->second;
        } else {
            auto file = QFile(projectSettings->translationPath() % u'/' % filename % u".txt"_qsv);

            if (!file.open(QFile::ReadOnly)) {
                qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(file.fileName(), file.errorString());
                continue;
            }

            content = QString::fromUtf8(file.readAll());
        }

        applyTranslatedDelta(tabName, countTranslatedRows(content, filename) - ui->tabPanel->tabTranslated(tabIndex));
    }

    const bool mapsRewritten = ranges::any_of(filenames, [](const FilenameArray& name) -> bool {
        return QL1SV(name.data()).startsWith("map"_L1);
    });

    ui->searchPanel->clearList();
    ui->lintMenu->clearResults();

    // A failed save has shown its notice, and the maps stay in memory until the next save
    if (mapsRewritten && !saveMaps()) {
        // Nothing else to do here
    }
}

void MainWindow::handleRowRemoved(const RemovedRowInfo info) {
    // The tab's setters add their argument to the current value
    if ((info.get<"flags">() & CommentFlag) == 0) {
        ui->globalProgressBar->setMaximum(ui->globalProgressBar->maximum() - 1);
        ui->tabPanel->setCurrentTotal(-1);

        if ((info.get<"flags">() & TranslatedFlag) != 0) {
            ui->tabPanel->setCurrentTranslated(-1);
            ui->globalProgressBar->setValue(ui->globalProgressBar->value() - 1);
        }
    } else if ((info.get<"flags">() & BookmarkFlag) != 0) {
        bookmarkMenu->removeBookmark(ui->tabPanel->currentTabName(), info.get<"row">());
    }

    bookmarkMenu->shiftIndices(ui->tabPanel->currentTabName(), info.get<"row">());

    updateStatusLabels();
    updateCounts();
}

void MainWindow::handleBatchMenuAction(
    const JSScript& script,
    Selected selected,
    const u8 columnIndex,
    const BatchVariant& variant
) {
    JSScript runnable = script;

    if (script.action == BatchAction::Custom) {
        if (!confirmScriptAccess(this, settings->core.approvedScripts, script.name, script.source)) {
            return;
        }

        runnable.hostApi = true;
    }

    const TaskToken task = startTask(TaskKind::BatchScript);

    if (task == nullptr) {
        return;
    }

    // The files are written by the task, and the editor would save its own text over them
    if (!closeTabIfSelected(selected)) {
        taskWorker->finishTask(task);
        return;
    }

    const Glossary glossary = glossaryMenu->glossary();
    const vector<FilenameArray> filenames = selected.filenames(projectSettings->engineType);

    QtConcurrent::run([=, this] -> auto {
        return taskWorker->performBatchAction(task, runnable, selected, columnIndex, variant, glossary);
    })
        .then(this, [this, task, filenames](const ScriptResult& scriptResult) -> void {
        taskWorker->finishTask(task);

        if (stoppingTasks) {
            return;
        }

        if (!scriptResult) {
            const auto& [message, filename, lineNumber] = scriptResult.error();

            // An aborted script ends with an interruption error, which is no failure of the script
            if (!task->interrupted()) {
                if (filename[0] == '\0') {
                    present(this, NOTICE("Script failed: %1", Warning, Modal, message));
                } else {
                    present(
                        this,
                        NOTICE(
                            "Script failed at %1, line %2: %3",
                            Warning,
                            Modal,
                            QL1SV(filename.data()),
                            qsv(qitos(lineNumber)),
                            message
                        )
                    );
                }
            }

            // Only an error inside a file's callback comes after a file was read, so only then may files have been
            // rewritten: the ones before the failed one. Otherwise nothing on disk changed.
            if (filename[0] != '\0') {
                refreshRewrittenTabs(filenames);
            }

            return;
        }

        const auto& [filenames, log, skippedCount, processedCount] = scriptResult.value();
        reportSkippedFiles(filenames, scast<u16>(skippedCount));

        // A script may catch its own errors and only log them, so what it logged is the only report there is
        if (!log.isEmpty()) {
            present(this, NOTICE("The script reported:\n%1", Info, Modal, log.join(u'\n')));
        }

        // A script rewrites rows of the translation files: the tabs it changed, and the maps, are refreshed
        if (processedCount != 0) {
            refreshRewrittenTabs(filenames);
        }
    }).onFailed(this, [this, task] -> void { taskFailed(task); });
}

auto MainWindow::closeTabIfSelected(const Selected& selected) -> bool {
    const QString currentTabName = ui->tabPanel->currentTabName();

    if (currentTabName.isEmpty()) {
        return true;
    }

    for (const auto filenameArray : selected.filenames(projectSettings->engineType)) {
        if (QL1SV(filenameArray.data()) == currentTabName) {
            ui->tabPanel->changeTab(QString());

            // Saving the tab failed, so it is still open
            return ui->tabPanel->currentTabName() != currentTabName;
        }
    }

    return true;
}

void MainWindow::handleSearchPanelAction(
    const SearchPanelDock::Action action,
    const QString& filename,
    const u32 rowIndex,
    const u8 columnIndex,
    span<const TextMatch> matches,
    SearchResultListItem& item
) {
    switch (action) {
        case SearchPanelDock::Action::GoTo:
            ui->tabPanel->changeTab(filename);

            QTimer::singleShot(1, this, [this, rowIndex] -> void {
                ui->translationTable->scrollTo(
                    ui->translationTable->model()->index(scast<i32>(rowIndex), 0),
                    TranslationTable::PositionAtCenter
                );
            });
            break;

        case SearchPanelDock::Action::Put:
        case SearchPanelDock::Action::Replace: {
            const QString replaceText = searchMenu->replaceText();

            const TaskToken task = startTask(TaskKind::ReplaceSingle);

            if (task == nullptr) {
                item.replaced = false;
                return;
            }

            // The file is written by the task, and the editor would save its own text over it
            if (filename == ui->tabPanel->currentTabName()) {
                ui->tabPanel->changeTab(QString());

                if (filename == ui->tabPanel->currentTabName()) {
                    taskWorker->finishTask(task);
                    item.replaced = false;
                    return;
                }
            }

            // The panel can be refilled while the task works, and the new items may lie where this one did
            const u32 panelGeneration = ui->searchPanel->generation();

            // The panel can be cleared while the task works, which frees the matches
            const vector<TextMatch> matchesCopy(matches.begin(), matches.end());

            // The cell as the search saw it: the replacement is refused if the cell changed since
            const QString expectedText = qsvReplace(item.file.text(item.cellMatch), LINE_SEPARATOR, u'\n');

            QtConcurrent::run([=, this] -> auto {
                return taskWorker->replaceSingle(
                    replaceText,
                    action,
                    filename,
                    rowIndex,
                    columnIndex,
                    span(matchesCopy),
                    expectedText
                );
            })
                .then(
                    this,
                    [this, &item, task, filename, columnIndex, panelGeneration](const ReplaceSingleResult& result)
                        -> void {
                taskWorker->finishTask(task);

                // The item is there only if the panel was not cleared or refilled
                const bool itemExists = ui->searchPanel->generation() == panelGeneration;

                if (!result) {
                    if (itemExists) {
                        item.replaced = false;
                    }

                    const auto& [failedFilename, stale] = result.error();

                    if (stale) {
                        present(
                            this,
                            NOTICE(
                                "The text in file %1 changed after the search, so it was not replaced. Search again.",
                                Warning,
                                Modal,
                                failedFilename
                            )
                        );
                    } else {
                        present(this, NOTICE("Replace failed, unable to open file %1", Warning, Modal, failedFilename));
                    }

                    return;
                }

                auto& replaced = ccast<ReplaceSingleResult&>(result).value();

                applyTranslatedDelta(filename, replaced.translatedDelta);

                // A put into another column leaves the cell that the item shows as it was
                if (itemExists && columnIndex == item.cellMatch.colIndex()) {
                    item.file.setText(item.cellMatch, replaced.text);
                    item.file.setMatches(item.cellMatch, replaced.matches);
                    ui->searchPanel->itemChanged(item);
                }
            }
                );

            break;
        }
    }
}

// Counts the open tab's source and first translation column as the table holds them, including committed edits.
// Runs in the background; a result that is stale or for another tab is dropped.
void MainWindow::updateCounts() {
    const QString tabName = ui->tabPanel->currentTabName();

    if (!settings->translation.displayWordsAndCharacters || tabName.isEmpty()) {
        countsGeneration++;
        countsStatusLabel->clear();
        countsStatusLabel->setToolTip(QString());
        return;
    }

    auto* const model = ui->translationTable->model();
    const i32 rowCount = model->rowCount();

    // Only the cells are picked here, as shared copies: joining them is what takes time on a big table
    vector<pair<QString, QString>> cells;
    cells.reserve(rowCount);

    for (i32 row = 0; row < rowCount; row++) {
        if (translatableSource(*model->item(row, 0))) {
            cells.emplace_back(*model->item(row, 0), *model->item(row, 1));
        }
    }

    const u32 generation = ++countsGeneration;

    QtConcurrent::run([cells = std::move(cells)] -> auto {
        isize sourceSize = 0;
        isize translationSize = 0;

        for (const auto& [sourceCell, translationCell] : cells) {
            sourceSize += translatableSource(sourceCell)->size() + 1;
            translationSize += translationCell.size() + 1;
        }

        QString source;
        QString translation;
        source.reserve(sourceSize);
        translation.reserve(translationSize);

        for (const auto& [sourceCell, translationCell] : cells) {
            source += *translatableSource(sourceCell);
            source += u'\n';
            translation += translationCell;
            translation += u'\n';
        }

        const auto counted = scast<u32>(cells.size());

        return pair{ countText(source, counted), countText(translation, counted) };
    }).then(this, [this, generation](const pair<TextCounts, TextCounts>& counts) -> void {
        if (generation != countsGeneration) {
            return;
        }

        const auto& [source, translation] = counts;

        countsStatusLabel->setText(tr("Source: %1 words / %2 chars | Translation: %3 words / %4 chars")
                                       .arg(
                                           qsv(qitos(source.words)),
                                           qsv(qitos(source.characters)),
                                           qsv(qitos(translation.words)),
                                           qsv(qitos(translation.characters))
                                       ));
        countsStatusLabel->setToolTip(
            tr("Characters without whitespace: source %1, translation %2")
                .arg(qsv(qitos(source.characters_no_whitespace)), qsv(qitos(translation.characters_no_whitespace)))
        );
    });
}

// Counts every file of the project in the background, several at a time, and shows the result in a dialog.
void MainWindow::showProjectStatistics() {
    // The open tab is counted from its file, so write out what is edited in it first.
    if (!saveCurrentTab()) {
        return;
    }

    const TaskToken task = startTask(TaskKind::Statistics);

    if (task == nullptr) {
        return;
    }

    vector<FilenameArray> filenames;
    filenames.reserve(ui->tabPanel->tabCount());

    for (i32 idx = 0; idx < ui->tabPanel->tabCount(); idx++) {
        const QByteArray name = ui->tabPanel->tabName(idx).toUtf8();
        FilenameArray filename{};
        copyFilenameArray(name.constData(), name.size(), filename.data());
        filenames.push_back(filename);
    }

    QtConcurrent::run([this, task, filenames = std::move(filenames)] -> auto {
        return taskWorker->projectStatistics(task, filenames);
    })
        .then(this, [this, task](const StatisticsResult& result) -> void {
        const bool cancelled = task->interrupted();
        taskWorker->finishTask(task);

        if (cancelled || stoppingTasks) {
            return;
        }

        reportSkippedFiles(result.skipped, scast<u16>(result.skipped.size()));

        auto* const dialog = new StatisticsDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->fill(result.files);
        dialog->show();
    }).onFailed(this, [this, task] -> void { taskFailed(task); });
}

void MainWindow::applyCellAction(const BatchAction action) {
    const QModelIndex index = ui->translationTable->currentIndex();
    auto* const model = ui->translationTable->model();

    // Only translation cells of real rows: not the source column, not read-only comments, not bookmark rows.
    if (!index.isValid() || !model->flags(index).testFlag(Qt::ItemIsEditable) ||
        *model->item(index.row(), 0) == BOOKMARK_COMMENT) {
        return;
    }

    BatchVariant variant;
    QStringView script;

    switch (action) {
        case BatchAction::Trim:
            variant = LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace;
            script = TRIM_SCRIPT;
            break;
        case BatchAction::Wrap:
            if (projectSettings->lineLengthHint == 0) {
                present(this, NOTICE("Set the line length hint in Settings > Project to wrap text.", Warning, Modal));
                return;
            }

            variant = scast<u8>(min<u16>(projectSettings->lineLengthHint, UINT8_MAX));
            script = WRAP_SCRIPT;
            break;
        case BatchAction::Latinize:
            script = LATINIZE_SCRIPT;
            break;
        default:
            std::unreachable();
    }

    auto* const input = ui->translationTable->activeInput();
    const QString current = input != nullptr ? input->toPlainText() : *model->item(index.row(), index.column());

    // Scripts see the text the way it is stored in the file, with `\#` as the line break.
    const Glossary noGlossary;
    const auto result = runCellScript(
        script,
        action,
        qsvReplace(current, LINE_FEED, NEW_LINE),
        scast<u8>(index.column()),
        variant,
        projectSettings.get(),
        noGlossary
    );

    if (!result) {
        present(this, NOTICE("Script failed: %1", Warning, Modal, result.error()));
        return;
    }

    const QString updated = qsvReplace(result.value(), NEW_LINE, LINE_FEED);

    if (updated == current) {
        return;
    }

    if (input != nullptr) {
        input->replaceText(updated);
    } else {
        model->setData(index, updated);
        ui->translationTable->resizeRowToContents(index.row());
    }
}

void MainWindow::handleSingleLint() {
    if ((settings->translation.lintFlags & LintFlags::LintingDisabled) != 0) {
        return;
    }

    const auto* input = ui->translationTable->activeInput();

    if (input == nullptr) {
        return;
    }

    const QModelIndex index = ui->translationTable->currentIndex();

    if (!index.isValid()) {
        return;
    }

    const QString* const source = ui->translationTable->model()->item(index.row(), 0);
    const QString currentTab = ui->tabPanel->currentTabName();

    const QString text = input->toPlainText();

    ui->lintMenu->clearResults();

    if (text.isEmpty()) {
        return;
    }

    const i32 size = scast<i32>(text.size());
    warnIfGlossaryLanguagesUnset();

    const QString sourceText = *source;
    const Glossary glossary = glossaryMenu->glossary();
    const i32 row = index.row();
    const u32 generation = ++lintGeneration;

    singleLintFuture = QtConcurrent::run([this, currentTab, row, sourceText, text, glossary] -> auto {
        const PreparedGlossary prepared = prepareGlossary(glossary);

        return taskWorker
            ->lint(currentTab, scast<u32>(row + 1), sourceText, text, prepared, taskWorker->customLintMatcher(), true);
    });

    singleLintFuture.then(this, [this, currentTab, row, size, generation, text](LintOutcome outcome) -> void {
        // A slower, older run would add its rows to those of the newer one
        if (generation != lintGeneration || ui->tabPanel->currentTabName() != currentTab ||
            ui->translationTable->currentIndex().row() != row) {
            return;
        }

        auto* const currentInput = ui->translationTable->activeInput();

        // The text typed since the lint started has other positions, and a newer lint is on its way
        if (currentInput == nullptr || currentInput->toPlainText() != text) {
            return;
        }

        applyLintRows(std::move(outcome.rows));
        currentInput->applyLintFormats(std::move(outcome.charStates), size);
    });
}

void MainWindow::handleGlobalLint(const Selected selected) {
    const TaskToken task = startTask(TaskKind::GlobalLint);

    if (task == nullptr) {
        return;
    }

    ui->lintMenu->clearResults();

    warnIfGlossaryLanguagesUnset();

    const Glossary glossary = glossaryMenu->glossary();

    QtConcurrent::run([this, task, selected, glossary] -> auto {
        return taskWorker->lintFiles(task, selected, glossary);
    })
        .then(this, [this, task](GlobalLintResult result) -> void {
        taskWorker->finishTask(task);

        if (stoppingTasks) {
            return;
        }

        reportSkippedFiles(result.filenames, scast<u16>(result.skippedCount));
        applyLintRows(std::move(result.rows));

        ui->lintMenu->show();
    }).onFailed(this, [this, task] -> void { taskFailed(task); });
}

void MainWindow::warnIfGlossaryLanguagesUnset() {
    if ((settings->translation.lintFlags & LintFlags::TermMismatch) == 0) {
        return;
    }

    const bool sourceLangUnset = projectSettings->sourceLang.isEmpty();
    const bool translationLangUnset = projectSettings->translationLang.isEmpty();

    if (!sourceLangUnset && !translationLangUnset) {
        return;
    }

    ui->lintMenuTable->appendRow(
        LintRow{ .filename = QString(),
                 .sourceText = u"-"_s,
                 .translationText = u"-"_s,
                 .info = languagesUnsetMessage(tr("glossary matches")),
                 .lineNumber = 0 }
    );
}

void MainWindow::applyLintRows(vector<LintRow> rows) {
    ui->lintMenuTable->appendRows(std::move(rows));
}

void MainWindow::showSettingsWindow() {
    // The tasks read the settings from their threads, and the window rewrites them when it is closed. So it opens only
    // when none runs, and, being modal to this window, none can be started while it is open.
    if (!taskWorker->idle()) {
        present(this, NOTICE("Settings cannot be opened while a task is running.", Warning, Status));
        return;
    }

    auto* const settingsWindow = new SettingsWindow(settings.get(), projectSettings.get(), this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->setWindowModality(Qt::WindowModal);
    settingsWindow->show();

    connect(settingsWindow, &QDialog::destroyed, this, [this] -> void {
        loadSettings();

        if (!saveSettings()) {
            qWarning().noquote() << u"Saving the settings after the settings window was closed failed."_qsv;
        }
    });
}

void MainWindow::showAboutWindow() {
    auto* const aboutWindow = new AboutWindow(this);
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
}

// Runs a moment after a cell got the focus (see `translateDebounceTimer`), so moving through the cells doesn't send a
// request, which can cost money, for each of them.
void MainWindow::handleInputFocused() {
    const QModelIndex index = ui->translationTable->currentIndex();

    if (projectSettings == nullptr || settings->translation.endpoints.empty() || !index.isValid() ||
        ui->translationTable->activeInput() == nullptr) {
        return;
    }

    if (!projectSettings->sourceLang.isEmpty() && !projectSettings->translationLang.isEmpty()) {
        const QString* const text = ui->translationTable->model()->item(index.row(), 0);
        const Glossary glossary = glossaryMenu->glossary();
        const u32 generation = translateGeneration;

        const QString currentTabName = ui->tabPanel->currentTabName();
        const QString sourceText = *text;

        // The request runs while the settings can change, so it works on a copy of what it reads
        auto context = TranslateContext{ .endpoints = settings->translation.endpoints,
                                         .approvedScripts = settings->core.approvedScripts,
                                         .sourceLang = projectSettings->sourceLang,
                                         .translationLang = projectSettings->translationLang };

        QtConcurrent::run([this, currentTabName, sourceText, glossary, context = std::move(context)] -> auto {
            return taskWorker->translateSingle(currentTabName, sourceText, glossary, context);
        }).then(this, [this, generation](const TranslateSingleResult& translations) -> void {
            // The answer for a cell that is not the current one would be inserted into the wrong cell
            if (generation == translateGeneration) {
                translationsMenu->showTranslations(translations, settings.get());
            }
        });
    } else {
        translationsMenu->showError(languagesUnsetMessage(tr("translations")));
    }
}

auto MainWindow::languagesUnsetMessage(const QString& what) const -> QString {
    const bool sourceLangUnset = projectSettings->sourceLang.isEmpty();
    const bool translationLangUnset = projectSettings->translationLang.isEmpty();

    if (sourceLangUnset && translationLangUnset) {
        return tr("Set source and translation languages in Settings > Project to show %1.").arg(what);
    }

    if (sourceLangUnset) {
        return tr("Set source language in Settings > Project to show %1.").arg(what);
    }

    return tr("Set translation language in Settings > Project to show %1.").arg(what);
}