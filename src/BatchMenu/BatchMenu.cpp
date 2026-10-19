#include "BatchMenu.hpp"

#include "Aliases.hpp"
#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "ScriptEditor.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "ui_BatchMenu.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

namespace {
constexpr array<BatchAction, 3> BUILTIN_SCRIPTS = { BatchAction::Trim, BatchAction::Wrap, BatchAction::Latinize };

[[nodiscard]] constexpr auto scriptAction(const i32 index) -> BatchAction {
    return scast<usize>(index) < BUILTIN_SCRIPTS.size() ? BUILTIN_SCRIPTS[index] : BatchAction::Custom;
}
}  // namespace

BatchMenu::BatchMenu(QWidget* const parent) :
    PersistentMenu(parent),
    ui(setupUi()),

    fileSelectMenu(new FileSelectMenu(parent)) {
    setDragMoveEnabled(true);

    // Not `pressed`: the handler may run a modal dialog or start the task, and a second press (double click) of the
    // still-held button would request the script again while the first runs
    connect(ui->processButton, &QPushButton::clicked, this, [this] -> void {
        const i32 column = ui->translationColumnSelect->currentIndex();

        if (column == 0) {
            present(this, NOTICE("Select the translation column you want perform the action in.", Warning, Modal));
            return;
        }

        const i32 scriptIndex = ui->scriptSelect->currentIndex();

        JSScript script = { .action = scriptAction(scriptIndex), .name = ui->scriptSelect->currentText() };

        if (script.action == BatchAction::Wrap && !ui->wrapSpinBox->hasAcceptableInput()) {
            present(this, NOTICE("Wrap length input requires a number from 20 to 255.", Warning, Modal));
            return;
        }

        const Selected selected = fileSelectMenu->selected();

        if (selected.empty()) {
            present(this, NOTICE("Select files you want to process in file select menu.", Warning, Modal));
            return;
        }

        BatchVariant variant;

        switch (script.action) {
            case BatchAction::Trim:
                switch (ui->trimSelect->currentIndex()) {
                    case 0:
                        variant = LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace;
                        break;
                    case 1:
                        variant = LintFlags::LeadingWhitespace;
                        break;
                    case 2:
                        variant = LintFlags::TrailingWhitespace;
                        break;
                    default:
                        std::unreachable();
                }
                break;

            case BatchAction::Wrap:
                variant = scast<u8>(ui->wrapSpinBox->value());
                break;

            default:
                break;
        }

        script.source = ui->scriptEdit->toPlainText();

        emit actionRequested(script, selected, column, variant);
    });

    connect(this, &PersistentMenu::hidden, this, [this] -> void {
        fileSelectMenu->hide();
        ui->fileSelectButton->setChecked(false);
    });

    connect(ui->fileSelectButton, &QPushButton::clicked, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });

    connect(ui->wrapValueButton, &QPushButton::clicked, this, [this] -> void {
        ui->wrapSpinBox->setValue(projectSettings->lineLengthHint);
    });

    const auto showScript = [this](const i32 index) -> void {
        storeCurrentScript();

        const BatchAction action = scriptAction(index);

        ui->scriptEdit->setReadOnly(true);
        ui->scriptSelect->setEditable(false);

        ui->wrapWidget->setHidden(action != BatchAction::Wrap);
        ui->trimWidget->setHidden(action != BatchAction::Trim);

        switch (action) {
            case BatchAction::Trim:
                ui->scriptEdit->setPlainText(TRIM_SCRIPT.toString());
                break;
            case BatchAction::Wrap:
                ui->scriptEdit->setPlainText(WRAP_SCRIPT.toString());
                break;
            case BatchAction::Latinize:
                ui->scriptEdit->setPlainText(LATINIZE_SCRIPT.toString());
                break;
            default:
                ui->scriptEdit->setPlainText(scripts[index - BUILTIN_SCRIPTS.size()]);
                ui->scriptSelect->setEditable(true);
                ui->scriptEdit->setReadOnly(false);
                break;
        }

        prevScript = index;
    };

    connect(ui->scriptSelect, &QComboBox::currentIndexChanged, this, showScript);

    connect(ui->scriptSelect, &QComboBox::editTextChanged, this, [this](const QString& text) -> void {
        ui->scriptSelect->setItemText(ui->scriptSelect->currentIndex(), text);
    });

    connect(ui->editScriptButton, &QPushButton::clicked, this, [this] -> void {
        auto editor = ScriptEditor(
            this,
            ui->scriptSelect->currentText(),
            ui->scriptEdit->toPlainText(),
            ui->scriptEdit->isReadOnly()
        );

        if (editor.exec() == QDialog::Accepted) {
            ui->scriptEdit->setPlainText(editor.source());
        }
    });

    connect(ui->createScriptButton, &QPushButton::clicked, this, [this] -> void {
        scripts.push_back(TEMPLATE_SCRIPT.toString());
        savedNames.emplace_back();
        ui->scriptSelect->addItem(tr("New Script"));
        ui->scriptSelect->setCurrentIndex(ui->scriptSelect->count() - 1);
    });

    ui->scriptSelect->installEventFilter(this);

    refreshScripts();

    // The combo box starts on the first built-in script, which nothing has shown yet
    showScript(0);

    adjustSize();
};

BatchMenu::~BatchMenu() {
    delete ui;
}

void BatchMenu::storeCurrentScript() {
    // `prevScript` is the script the editor shows
    if (scriptAction(prevScript) == BatchAction::Custom) {
        scripts[prevScript - BUILTIN_SCRIPTS.size()] = ui->scriptEdit->toPlainText();
    }
}

void BatchMenu::refreshScripts() {
    // The edit of the open script is not in `scripts` until it is stored
    storeCurrentScript();

    const QString dir = dataLocation() % u"/scripts"_qsv;

    QStringList onDisk;

    for (const auto& entry : QDirListing(dir, { u"*.js"_s }, QDirListing::IteratorFlag::FilesOnly)) {
        onDisk.push_back(entry.fileName());
    }

    // A script that was never saved has no file, so it is never removed
    const auto isRemoved = [&onDisk](const QString& name) -> bool { return !name.isEmpty() && !onDisk.contains(name); };

    const QString openName =
        scriptAction(prevScript) == BatchAction::Custom ? savedNames[prevScript - BUILTIN_SCRIPTS.size()] : QString();
    const bool openRemoved = isRemoved(openName);

    // Moves the editor off the removed script, so the item can go without leaving a stale selection
    if (openRemoved) {
        ui->scriptSelect->setCurrentIndex(0);
    }

    {
        const QSignalBlocker blocker(ui->scriptSelect);

        // Walks backward, so erasing an index does not shift the ones still to visit
        for (usize idx = scripts.size(); idx-- > 0;) {
            if (isRemoved(savedNames[idx])) {
                ui->scriptSelect->removeItem(scast<i32>(idx + BUILTIN_SCRIPTS.size()));
                scripts.erase(scripts.begin() + scast<isize>(idx));
                savedNames.erase(savedNames.begin() + scast<isize>(idx));
            }
        }

        for (const QString& name : onDisk) {
            if (ranges::contains(savedNames, name)) {
                continue;
            }

            const QString path = dir % u'/' % name;
            auto file = QFile(path);

            if (!file.open(QFile::ReadOnly)) {
                qWarning().noquote() << u"Failed to open script %1: %2"_qsv.arg(path, file.errorString());
                continue;
            }

            ui->scriptSelect->addItem(name);

            scripts.push_back(QString::fromUtf8(file.readAll()));
            savedNames.push_back(name);
        }

        // The combo box tracks its current item through removals, so this is the open script's new index
        prevScript = ui->scriptSelect->currentIndex();
    }

    if (openRemoved) {
        present(this, NOTICE("The script %1 has been removed from the scripts directory.", Warning, Modal, openName));
    }
}

auto BatchMenu::eventFilter(QObject* const watched, QEvent* const event) -> bool {
    if (watched == ui->scriptSelect) {
        if (event->type() == QEvent::MouseButtonPress) {
            refreshScripts();
        } else if (event->type() == QEvent::KeyPress) {
            const auto* const keyEvent = scast<QKeyEvent*>(event);
            const bool popupPressed =
                keyEvent->key() == Qt::Key_F4 ||
                (keyEvent->key() == Qt::Key_Down && keyEvent->modifiers().testFlag(Qt::AltModifier));

            if (popupPressed) {
                refreshScripts();
            }
        }
    }

    return PersistentMenu::eventFilter(watched, event);
}

auto BatchMenu::save() -> bool {
    // The edit of the open script is not in `scripts` until another script is picked
    storeCurrentScript();

    const auto dest = dataLocation() % u"/scripts"_qsv;
    QDir().mkpath(dest);

    QStringList errs;

    for (const auto& [idx, script] : views::enumerate(scripts)) {
        const i32 itemIndex = scast<i32>(idx + BUILTIN_SCRIPTS.size());

        // The name is typed by the user: only a file name, and one the next start loads (`*.js`)
        QString filename = QFileInfo(ui->scriptSelect->itemText(itemIndex)).fileName();

        if (filename.isEmpty()) {
            filename = u"script"_s;
        }

        if (!filename.endsWith(u".js"_qsv, Qt::CaseInsensitive)) {
            filename = filename % u".js"_qsv;
        }

        const QString suffix = u'/' + filename;
        QString path = dest % suffix;

        auto opened = openForWriting(this, path, suffix);

        if (!opened) {
            return false;
        }

        auto file = std::move(opened.value());

        if (file == nullptr) {
            errs.push_back(u"Skipped saving script %1"_qsv.arg(filename));
            continue;
        }

        file->write(script.toUtf8());

        if (!commitFile(this, *file)) {
            errs.push_back(u"Failed to save script %1"_qsv.arg(filename));
            continue;
        }

        // A renamed script would otherwise be loaded twice, under the old name and the new one
        if (!savedNames[idx].isEmpty() && savedNames[idx] != filename) {
            QFile::remove(dest % u'/' % savedNames[idx]);
        }

        savedNames[idx] = filename;

        if (ui->scriptSelect->itemText(itemIndex) != filename) {
            ui->scriptSelect->setItemText(itemIndex, filename);
        }
    }

    if (!errs.empty()) {
        present(this, NOTICE("Failed to save scripts:\n%1", Warning, Modal, errs.join(u'\n')));
    }

    return errs.empty();
}

auto BatchMenu::setupUi() -> Ui::BatchMenu* {
    auto* const ui_ = new Ui::BatchMenu();
    ui_->setupUi(this);
    return ui_;
};

void BatchMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    PersistentMenu::changeEvent(event);
};

void BatchMenu::clear() {
    // Item 0 is the "- Translation Column -" placeholder
    while (ui->translationColumnSelect->count() > 1) {
        ui->translationColumnSelect->removeItem(ui->translationColumnSelect->count() - 1);
    }

    ui->translationColumnSelect->setCurrentIndex(0);
    ui->wrapSpinBox->clear();

    fileSelectMenu->clear();
}

void BatchMenu::addColumn(const QString& name) {
    ui->translationColumnSelect->addItem(name);
};

void BatchMenu::renameColumn(const u8 index, const QString& name) {
    ui->translationColumnSelect->setItemText(index, name);
};

void BatchMenu::init(const vector<TabListItem>& files, const ProjectSettings* const projectSettings) {
    fileSelectMenu->init(files);
    this->projectSettings = projectSettings;
}
