#include "BatchMenu.hpp"

#include "Aliases.hpp"
#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "ui_BatchMenu.h"

#include <QDir>
#include <QLineEdit>

namespace {
constexpr i32 TRANSLATE_ACTION_INDEX = 1;

constexpr array BUILTIN_SCRIPTS = { BatchAction::Trim, BatchAction::Wrap, BatchAction::Latinize };

[[nodiscard]] constexpr auto scriptAction(const i32 index) -> BatchAction {
    return scast<usize>(index) < BUILTIN_SCRIPTS.size() ? BUILTIN_SCRIPTS[index] : BatchAction::Custom;
}
}  // namespace

BatchMenu::BatchMenu(QWidget* const parent) :
    PersistentMenu(parent),
    ui(setupUi()),

    fileSelectMenu(new FileSelectMenu(parent)) {
    setDragMoveEnabled(true);

    ui->translationEndpointSelect->hide();
    ui->contextContainer->hide();
    ui->scriptWidget->hide();

    ui->wrapWidget->hide();
    ui->trimWidget->hide();

    connect(ui->batchActionSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
        const bool translating = index == TRANSLATE_ACTION_INDEX;

        ui->translationEndpointSelect->setVisible(translating);
        ui->contextContainer->setVisible(translating);
        ui->scriptWidget->setHidden(translating);

        adjustSize();
    });

    connect(ui->processButton, &QPushButton::pressed, this, [this] -> void {
        const i32 actionIndex = ui->batchActionSelect->currentIndex();
        const i32 column = ui->translationColumnSelect->currentIndex();

        if (actionIndex == 0) {
            present(this, NOTICE("Select the batch action you want to perform.", Warning, Modal));
            return;
        }

        if (column == 0) {
            present(this, NOTICE("Select the translation column you want perform the action in.", Warning, Modal));
            return;
        }

        const bool translating = actionIndex == TRANSLATE_ACTION_INDEX;
        const i32 scriptIndex = ui->scriptSelect->currentIndex();

        JSScript script = { .action = translating ? BatchAction::Translate : scriptAction(scriptIndex),
                            .name = ui->scriptSelect->currentText() };

        if (translating) {
            if (ui->translationEndpointSelect->currentIndex() == 0) {
                present(this, NOTICE("Select the translation endpoint you want to use.", Warning, Modal));
                return;
            }
        } else if (script.action == BatchAction::Wrap && !ui->wrapSpinBox->hasAcceptableInput()) {
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
            case BatchAction::Translate:
                variant =
                    EndpointContext{ .endpointIndex = scast<u8>(ui->translationEndpointSelect->currentIndex() - 1),
                                     .context = ui->contextInput->toPlainText() };
                break;

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

        if (!translating) {
            script.source = ui->scriptEdit->toPlainText();
        }

        emit actionRequested(script, selected, column, variant);
    });

    connect(this, &PersistentMenu::hidden, this, [this] -> void {
        fileSelectMenu->hide();
        ui->fileSelectButton->setChecked(false);
    });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });

    connect(ui->wrapValueButton, &QPushButton::pressed, this, [this] -> void {
        ui->wrapSpinBox->setValue(projectSettings->lineLengthHint);
    });

    connect(ui->scriptSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
        if (scriptAction(prevScript) == BatchAction::Custom) {
            scripts[prevScript - BUILTIN_SCRIPTS.size()] = ui->scriptEdit->toPlainText();
        }

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
    });

    connect(ui->scriptSelect, &QComboBox::editTextChanged, this, [this](const QString& text) -> void {
        ui->scriptSelect->setItemText(ui->scriptSelect->currentIndex(), text);
    });

    connect(ui->createScriptButton, &QPushButton::pressed, this, [this] -> void {
        scripts.push_back(TEMPLATE_SCRIPT.toString());
        ui->scriptSelect->addItem(tr("New Script"));
        ui->scriptSelect->setCurrentIndex(ui->scriptSelect->count() - 1);
    });

    const auto entries =
        QDir(QApplication::applicationDirPath() % u"/scripts"_qsv).entryList(QDir::NoDotAndDotDot | QDir::Files);

    for (const auto& entry : entries) {
        auto file = QFile(entry);

        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << u"Failed to open script %1: %2"_qsv.arg(entry, file.errorString());
            continue;
        }

        const QString name = lastPathComponent(entry).toString();

        ui->scriptSelect->addItem(name);

        scripts.push_back(QString::fromUtf8(file.readAll()));
    }

    adjustSize();
};

BatchMenu::~BatchMenu() {
    delete ui;
}

auto BatchMenu::save() -> bool {
    const auto dest = QApplication::applicationDirPath() % u"/scripts"_qsv;
    QDir().mkdir(dest);

    QStringList errs;

    for (const auto& [idx, script] : views::enumerate(scripts)) {
        const QString filename = ui->scriptSelect->itemText(scast<i32>(idx + BUILTIN_SCRIPTS.size()));
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
    for (u8 index = ui->translationColumnSelect->count() - 1; index >= 1; index++) {
        ui->translationColumnSelect->removeItem(index);
    }

    ui->translationColumnSelect->setCurrentIndex(0);
    ui->batchActionSelect->setCurrentIndex(0);
    ui->translationEndpointSelect->setCurrentIndex(0);

    ui->wrapSpinBox->clear();

    ui->contextInput->clear();
    ui->fileContextList->clear();

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

void BatchMenu::setEndpoints(const vector<EndpointSettings>& endpoints) {
    for (i32 idx = ui->translationEndpointSelect->count() - 1; idx >= 1; idx--) {
        ui->translationEndpointSelect->removeItem(idx);
    }

    for (const auto& endpoint : endpoints) {
        ui->translationEndpointSelect->addItem(endpoint.name);
    }
}