#include "AboutWindow.hpp"
#include "BookmarkMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "PopupInput.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "SettingsWindow.hpp"
#include "TaskPanel.hpp"
#include "TranslationInput.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QtConcurrent>

namespace {
constexpr i32 GO_TO_ROW_INPUT_MAX_LENGTH = 10;
constexpr i32 GO_TO_ROW_INPUT_WIDTH = 256;
constexpr i32 GO_TO_ROW_INPUT_X_OFFSET = 128;
constexpr i32 GO_TO_ROW_INPUT_Y_OFFSET = 64;

constexpr string_view COMMENT_PREFIX_BYTES = "<!>";
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

    optional<TaskKind> kind;

    if (message.endsWith(u"purged.")) {
        kind = TaskKind::Purge;
    } else if (message.endsWith(u"written.")) {
        kind = TaskKind::Write;
    } else if (message.endsWith(u"read.")) {
        kind = TaskKind::Read;
    }

    if (!kind) {
        return;
    }

    const TaskToken task = taskWorker->runningTask(*kind);

    if (task == nullptr) {
        return;
    }

    const u32 spacePos = message.indexOf(u' ') + 1;

    taskPanel->updateTask(
        task->id(),
        QStringView(message).sliced(spacePos, message.lastIndexOf(u':') - spacePos).toString(),
        0,
        0
    );
}

void MainWindow::handleGoToRow() {
    if (ui->tabPanel->currentTabName().isEmpty()) {
        return;
    }

    auto* const popupInput = new PopupInput(this);
    popupInput->setValidator(new QIntValidator(0, INT32_MAX, popupInput));
    popupInput->setMaxLength(GO_TO_ROW_INPUT_MAX_LENGTH);
    popupInput->setFixedWidth(GO_TO_ROW_INPUT_WIDTH);
    popupInput->setPlaceholderText(
        tr("Input line from %1 to %2").arg(u"1", QStringView(linesStatusLabel->text()).split(u' ').first())
    );

    popupInput->move((width() / 2) - GO_TO_ROW_INPUT_X_OFFSET, x() + GO_TO_ROW_INPUT_Y_OFFSET);
    popupInput->show();
    popupInput->setFocus();

    connect(popupInput, &PopupInput::inputRejected, this, [popupInput] -> void {
        delete popupInput;
    }, Qt::SingleShotConnection);

    connect(popupInput, &PopupInput::editingFinished, this, [this, popupInput] -> void {
        const i32 rowIndex = stoa<i32>(popupInput->text()) - 1;

        ui->translationTable->scrollTo(
            ui->translationTable->model()->index(rowIndex, 0),
            TranslationTable::PositionAtCenter
        );

        delete popupInput;
    }, Qt::SingleShotConnection);
}

void MainWindow::handleWrite() {
    const QString sourcePath = projectSettings->actualSourcePath();

    if (!QFile::exists(sourcePath)) {
        present(this, NOTICE("Cannot write, source files are absent.", Warning, Modal));
        return;
    }

    const TaskToken task = startTask(TaskKind::Write);

    if (task == nullptr) {
        return;
    }

    QtConcurrent::run([this] -> auto {
        return taskWorker->write(Selected());
    }).then(this, [this, task](const WriteResult& result) -> void {
        taskWorker->finishTask(task);

        if (!result) {
            present(this, NOTICE("Write failed: %1", Critical, Modal, ffitostr(result.error())));
            return;
        }

        present(
            this,
            NOTICE(
                "Written successfully. Elapsed: %1s. Check `.rpgmtranslate/output` for output files.",
                Info,
                Status,
                ftos(result.value(), 2).qsv()
            )
        );
    });
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
    }).then(this, [=, this](SearchResult results) -> void {
        taskWorker->finishTask(searchTask);

        if (results.skippedCount != 0) {
            reportSkippedFiles(results.filenames, results.skippedCount);
        }

        if (action == SearchAction::Search) {
            ui->searchPanel->showMatches(std::move(results.matches), mapSections, projectSettings);
            return;
        }

        const TaskToken replaceTask = startTask(TaskKind::Replace);

        if (replaceTask == nullptr) {
            for (const auto& [filename, matches] : results.matches) {
                for (const auto match : matches) {
                    delete[] match.matches;
                }
            }

            return;
        }

        const QString currentTabName = ui->tabPanel->currentTabName();

        for (const auto filenameArray : selected.filenames(projectSettings->engineType)) {
            if (QL1SV(filenameArray.data()) == currentTabName) {
                ui->tabPanel->changeTab(QString());
            }
        }

        QtConcurrent::run([=, this, matches = std::move(results.matches)] -> auto {
            return taskWorker->replace(replaceTask, matches, action, replaceText);
        }).then(this, [this, replaceTask](const ReplaceResult& skipped) -> void {
            taskWorker->finishTask(replaceTask);

            if (!skipped.empty()) {
                reportSkippedFiles(skipped, scast<u16>(skipped.size()));
            }
        });
    });
}

void MainWindow::handleRowRemoved(const RemovedRowInfo info) {
    if ((info.flags() & CommentFlag) == 0) {
        ui->globalProgressBar->setMaximum(ui->globalProgressBar->maximum() - 1);
        ui->tabPanel->setCurrentTotal(ui->tabPanel->currentTotal() - 1);

        if ((info.flags() & TranslatedFlag) != 0) {
            ui->tabPanel->setCurrentTotal(ui->tabPanel->currentTotal() - 1);
            ui->globalProgressBar->setValue(ui->globalProgressBar->value() - 1);
        }
    } else if ((info.flags() & BookmarkFlag) != 0) {
        bookmarkMenu->removeBookmark(info.row());
    }

    bookmarkMenu->shiftIndices(ui->tabPanel->currentTabName(), info.row());

    linesStatusLabel->setText(
        tr("%1 Lines / %2 Comments")
            .arg(
                itos(ui->translationTable->model()->rowCount()).qsv(),
                itos(ui->translationTable->model()->rowCount() - ui->tabPanel->currentTotal()).qsv()
            )
    );
}

void MainWindow::handleBatchMenuAction(
    const JSScript& script,
    Selected selected,
    const u8 columnIndex,
    const BatchVariant& variant
) {
    const bool isTranslate = script.action == BatchAction::Translate;

    const QString currentTabName = ui->tabPanel->currentTabName();

    for (const auto filenameArray : selected.filenames(projectSettings->engineType)) {
        if (QL1SV(filenameArray.data()) == currentTabName) {
            ui->tabPanel->changeTab(QString());
        }
    }

    if (isTranslate) {
        if (projectSettings->sourceLang.isEmpty()) {
            present(
                this,
                NOTICE(
                    "Cannot perform batch-translate. You need to set source language in Settings > Project first.",
                    Warning,
                    Modal
                )
            );
            return;
        }

        if (projectSettings->translationLang.isEmpty()) {
            present(
                this,
                NOTICE(
                    "Cannot perform batch-translate. You need to set translation language in Settings > Project first.",
                    Warning,
                    Modal
                )
            );
            return;
        }
    }

    const TaskToken task = startTask(isTranslate ? TaskKind::BatchTranslate : TaskKind::BatchScript);

    if (task == nullptr) {
        return;
    }

    const Glossary glossary = isTranslate ? glossaryMenu->glossary() : Glossary{};

    QtConcurrent::run([=, this] -> auto {
        return taskWorker->performBatchAction(task, script, selected, columnIndex, variant, glossary);
    }).then(this, [this, columnIndex, selected, task](const BatchResult& batchResult) -> void {
        taskWorker->finishTask(task);

        if (const auto* const scriptResult = std::get_if<ScriptResult>(&batchResult)) {
            if (!*scriptResult) {
                const auto& [message, filename, lineNumber] = scriptResult->error();

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
                            itos(lineNumber).qsv(),
                            message
                        )
                    );
                }

                return;
            }

            const auto& [filenames, skippedCount] = scriptResult->value();
            reportSkippedFiles(filenames, scast<u16>(skippedCount));

            openProject(projectSettings->projectPath, false);
            return;
        }

        const auto& result = std::get<TranslateResult>(batchResult);

        if (!result) {
            const auto error = result.error();

            present(this, NOTICE("Batch translation failed with error: %1", Warning, Modal, ffitostr(error)));

            return;
        }

        const auto [translatedFiles, translatedFilesFFI] = result.value();

        const auto stringsArray = span(rcast<const ByteBuffer*>(translatedFilesFFI.ptr), translatedFilesFFI.len);

        u16 skippedCount = 0;
        auto filenames = selected.filenames(projectSettings->engineType);

        for (const auto [idx, filenameArray] : views::enumerate(filenames)) {
            if (stringsArray[idx].len == 0) {
                qInfo().noquote() << u"Translated strings array at index %1 in file %2 is empty."_qsv.arg(
                    itos(idx).qsv(),
                    svtostr(QL1SV(filenameArray.data()))
                );
                continue;
            }

            const auto strings = span(rcast<const FFIString*>(stringsArray[idx].ptr), stringsArray[idx].len);

            const auto filename = QL1SV(filenameArray.data());

            if (ui->tabPanel->currentTabName() == filename) {
                ui->tabPanel->changeTab(QString());
            }
            lockedFile = filename;

            QString content;
            unique_ptr<QFile> file;

            const bool isMap = filename.startsWith("map"_L1);
            const u16 mapNumber = isMap ? stoa<u16>(filename.sliced(3)) : 0;

            if (isMap) {
                content = mapSections[mapNumber];
            } else {
                const QString path = projectSettings->translationPath() + u'/' + filename + u".txt";
                file = make_unique<QFile>(path);

                if (!file->open(QFile::ReadWrite)) {
                    qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(path, file->errorString());

                    std::swap(filenames[idx], filenames[skippedCount++]);
                    continue;
                }

                content = QString::fromUtf8(file->readAll());
            }

            auto filteredStrings = views::filter(strings, [](const FFIString element) -> bool {
                return element.len < COMMENT_PREFIX_BYTES.size() ||
                       std::memcmp(element.ptr, COMMENT_PREFIX_BYTES.data(), COMMENT_PREFIX_BYTES.size()) != 0;
            });

            auto stringsIterator = filteredStrings.begin();
            const QSVList lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);

            QStringList newLines;
            newLines.reserve(lines.size());

            for (const QStringView line : lines) {
                if (line.startsWith(COMMENT_PREFIX)) {
                    newLines.push_back(line.toString());
                    continue;
                }

                const auto translated = *stringsIterator++;
                QSVList parts = line.split(SEPARATOR);

                if (parts.size() <= columnIndex) {
                    newLines.push_back(line.toString());
                    continue;
                }

                const QString translatedText = QString::fromUtf8(translated.ptr, scast<isize>(translated.len));
                parts[columnIndex] = translatedText;

                newLines.push_back(joinQSVList(parts, SEPARATOR));
            }

            const QString replaced = newLines.join(u'\n');

            const u16 tabIndex = ui->tabPanel->tabIndex(filename);

            const i32 total = ui->tabPanel->tabTotal(tabIndex);
            const i32 translated = ui->tabPanel->tabTranslated(tabIndex);

            ui->tabPanel->setTabTranslated(tabIndex, total);

            ui->globalProgressBar->setValue((ui->globalProgressBar->value() - translated) + total);

            if (isMap) {
                mapSections.insert_or_assign(mapNumber, replaced);
            } else {
                const QByteArray utf8 = replaced.toUtf8();

                file->seek(0);
                file->resize(utf8.size());
                file->write(utf8);
            }
        }

        rpgm_free_translated_files(translatedFiles, translatedFilesFFI);

        reportSkippedFiles(filenames, skippedCount);

        lockedFile = QString();
    });
};

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
            if (filename == ui->tabPanel->currentTabName()) {
                ui->tabPanel->changeTab(QString());
            }

            const QString replaceText = searchMenu->replaceText();

            const TaskToken task = startTask(TaskKind::ReplaceSingle);

            if (task == nullptr) {
                return;
            }

            QtConcurrent::run([=, this] -> auto {
                return taskWorker->replaceSingle(replaceText, action, filename, rowIndex, columnIndex, matches);
            }).then(this, [this, &item, matches, task, filename](const ReplaceSingleResult& result) -> void {
                taskWorker->finishTask(task);

                if (!result) {
                    present(this, NOTICE("Replace failed, unable to open file %1", Warning, Modal, result.error()));
                    return;
                }

                const auto [text, newMatches] = result.value();

                item.title = text;
                memcpy(item.cellMatch.matches, newMatches, matches.size() * sizeof(TextMatch));

                delete[] newMatches;
            });

            break;
        }
    }
};

void MainWindow::handleSingleLint() {
    if ((settings->translation.lintFlags & LintFlags::LintingDisabled) != 0) {
        return;
    }

    const auto* input = ui->translationTable->activeInput();

    if (input == nullptr) {
        return;
    }

    const QModelIndex index = ui->translationTable->currentIndex();
    const auto& sourceItem = ui->translationTable->model()->item(index.row(), 0);
    const QString* const source = sourceItem.text();
    const QString currentTab = ui->tabPanel->currentTabName();

    if (!index.isValid()) {
        return;
    }

    const QString text = input->toPlainText();

    if (text.isEmpty()) {
        return;
    }

    const i32 size = scast<i32>(text.size());

    ui->lintMenu->clear();
    warnIfGlossaryLanguagesUnset();

    const QString sourceText = *source;
    const Glossary glossary = glossaryMenu->glossary();
    const i32 row = index.row();

    QtConcurrent::run([this, currentTab, row, sourceText, text, glossary] -> auto {
        return taskWorker->lint(currentTab, scast<u32>(row + 1), sourceText, text, glossary, true);
    }).then(this, [this, currentTab, row, size](LintOutcome outcome) -> void {
        if (ui->tabPanel->currentTabName() != currentTab || ui->translationTable->currentIndex().row() != row) {
            return;
        }

        auto* const currentInput = ui->translationTable->activeInput();

        if (currentInput == nullptr) {
            return;
        }

        applyLintRows(std::move(outcome.rows));
        currentInput->applyLintFormats(std::move(outcome.charStates), size);
    });
}

void MainWindow::handleGlobalLint(const Selected selected) {
    ui->lintMenu->clear();

    const TaskToken task = startTask(TaskKind::GlobalLint);

    if (task == nullptr) {
        return;
    }

    warnIfGlossaryLanguagesUnset();

    const Glossary glossary = glossaryMenu->glossary();

    QtConcurrent::run([this, task, selected, glossary] -> auto {
        return taskWorker->lintFiles(task, selected, glossary);
    }).then(this, [this, task](GlobalLintResult result) -> void {
        taskWorker->finishTask(task);

        reportSkippedFiles(result.filenames, scast<u16>(result.skippedCount));
        applyLintRows(std::move(result.rows));

        ui->lintMenu->show();
    });
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

    QString message = tr("Set translation language in Settings > Project to show glossary matches.");

    if (sourceLangUnset && translationLangUnset) {
        message = tr("Set source and translation languages in Settings > Project to show glossary matches.");
    } else if (sourceLangUnset) {
        message = tr("Set source language in Settings > Project to show glossary matches.");
    }

    ui->lintMenuTable->appendRow(
        LintRow{ .filename = QString(),
                 .sourceText = u"-"_s,
                 .translationText = u"-"_s,
                 .info = message,
                 .lineNumber = 0 }
    );
}

void MainWindow::applyLintRows(vector<LintRow> rows) {
    ui->lintMenuTable->appendRows(std::move(rows));
}

void MainWindow::showSettingsWindow() {
    auto* const settingsWindow = new SettingsWindow(settings, projectSettings, ui->tabPanel->tabs(), this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->show();

    connect(settingsWindow, &QDialog::destroyed, this, [this] -> void {
        loadSettings();
        saveSettings();
    });
}

void MainWindow::showAboutWindow() {
    auto* const aboutWindow = new AboutWindow(this);
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
}

void MainWindow::handleInputFocused() {
    if (settings->translation.endpoints.empty() ||
        ranges::all_of(settings->translation.endpoints, [](const auto& endpoint) -> bool {
        return !endpoint.singleTranslation;
    })) {
        return;
    };

    if (!projectSettings->sourceLang.isEmpty() && !projectSettings->translationLang.isEmpty()) {
        const auto& sourceItem = ui->translationTable->model()->item(ui->translationTable->currentIndex().row(), 0);
        const QString* const text = sourceItem.text();
        const Glossary glossary = glossaryMenu->glossary();

        const QString currentTabName = ui->tabPanel->currentTabName();
        const QString sourceText = *text;

        QtConcurrent::run([this, currentTabName, sourceText, glossary] -> auto {
            return taskWorker->translateSingle(currentTabName, sourceText, glossary);
        }).then(this, [this](const TranslateSingleResult& translations) -> void {
            translationsMenu->showTranslations(translations, settings);
        });
    } else {
        const bool sourceLangUnset = projectSettings->sourceLang.isEmpty();
        const bool translationLangUnset = projectSettings->translationLang.isEmpty();

        QString message = tr("Set translation language in Settings > Project to show translations.");

        if (sourceLangUnset && translationLangUnset) {
            message = tr("Set source and translation languages in Settings > Project to show translations.");
        } else if (sourceLangUnset) {
            message = tr("Set source language in Settings > Project to show translations.");
        }

        translationsMenu->showError(message);
    }
}