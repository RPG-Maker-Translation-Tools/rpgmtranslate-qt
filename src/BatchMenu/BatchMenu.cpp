#include "BatchMenu.hpp"

#include "FileSelectMenu.hpp"
#include "Types.hpp"
#include "ui_BatchMenu.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QMouseEvent>

BatchMenu::BatchMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    ui(setupUi()),

    fileSelectMenu(new FileSelectMenu(parent)) {
    setDragMoveEnabled(true);

    ui->translationEndpointSelect->hide();
    ui->wrapLengthInput->hide();
    ui->contextContainer->hide();
    ui->trimSelect->hide();

    ui->wrapLengthInput->setValidator(
        new QIntValidator(20, UINT8_MAX, ui->wrapLengthInput)
    );

    connect(
        ui->batchActionSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u8 index) -> void {
        const auto action = BatchAction(index);

        ui->translationEndpointSelect->hide();
        ui->contextContainer->hide();
        ui->wrapLengthInput->hide();
        ui->trimSelect->hide();

        switch (action) {
            case BatchAction::None:
            case BatchAction::Trim:
                ui->trimSelect->show();
                break;
            case BatchAction::Translate:
                ui->translationEndpointSelect->show();
                ui->contextContainer->show();
                break;
            case BatchAction::Wrap:
                ui->wrapLengthInput->show();
                break;
        }

        adjustSize();
    }
    );

    connect(ui->processButton, &QPushButton::pressed, this, [this] -> void {
        const auto action = BatchAction(ui->batchActionSelect->currentIndex());
        const u8 column = ui->translationColumnSelect->currentIndex();

        if (action == BatchAction::None) {
            QMessageBox::warning(
                nullptr,
                tr("Batch action not selected"),
                tr("Select the batch action you want to perform.")
            );
            return;
        }

        if (column == 0) {
            QMessageBox::warning(
                nullptr,
                tr("Translation column not selected"),
                tr(
                    "Select the translation column you want perform the action in."
                )
            );
            return;
        }

        switch (action) {
            case BatchAction::None:
                std::unreachable();
            case BatchAction::Trim:
                break;
            case BatchAction::Translate: {
                const u8 endpoint =
                    ui->translationEndpointSelect->currentIndex();

                if (endpoint == 0) {
                    QMessageBox::warning(
                        nullptr,
                        tr("Translation endpoint not selected"),
                        tr("Select the translation endpoint you want to use.")
                    );
                }

                break;
            }
            case BatchAction::Wrap:
                if (!ui->wrapLengthInput->hasAcceptableInput()) {
                    QMessageBox::information(
                        nullptr,
                        tr("Invalid value"),
                        tr("Wrap length input requires a number from 20 to 255.")
                    );
                }
                break;
        }

        const Selected selected = fileSelectMenu->selected();

        if (selected.empty()) {
            QMessageBox::warning(
                nullptr,
                tr("No files selected"),
                tr("Select files you want to process in file select menu.")
            );
            return;
        }

        std::variant<TrimFlags, std::tuple<TranslationEndpoint, QString>, u8>
            variant;

        switch (action) {
            case BatchAction::None:
                std::unreachable();
            case BatchAction::Trim:
                switch (ui->trimSelect->currentIndex()) {
                    case 0:
                        variant = TrimFlags::Leading | TrimFlags::Trailing;
                        break;
                    case 1:
                        variant = TrimFlags::Leading;
                        break;
                    case 2:
                        variant = TrimFlags::Trailing;
                        break;
                    default:
                        std::unreachable();
                }

                break;
            case BatchAction::Translate: {
                variant = std::tuple(
                    TranslationEndpoint(
                        ui->translationEndpointSelect->currentIndex() - 1
                    ),
                    ui->contextInput->toPlainText()
                );
                break;
            }
            case BatchAction::Wrap:
                variant = u8(ui->wrapLengthInput->text().toUInt());
                break;
        }

        emit actionRequested(selected, action, column, variant);
    });

    connect(this, &PersistentMenu::hidden, this, [this] -> void {
        fileSelectMenu->hide();
        ui->fileSelectButton->setChecked(false);
    });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });

    adjustSize();
};

BatchMenu::~BatchMenu() {
    delete ui;
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
    for (const u8 index : range<u8>(1, ui->translationColumnSelect->count())) {
        ui->translationColumnSelect->removeItem(index);
    }

    ui->translationColumnSelect->setCurrentIndex(0);
    ui->batchActionSelect->setCurrentIndex(0);
    ui->translationEndpointSelect->setCurrentIndex(0);

    ui->wrapLengthInput->clear();

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

void BatchMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file);
}