#include "SearchMenu.hpp"

#include "Enums.hpp"
#include "FileSelectMenu.hpp"
#include "ui_SearchMenu.h"

#include <QMessageBox>

void SearchMenu::requestAction(const bool replace) {
    if (fileSelectMenu->selectedCount() == 0) {
        QMessageBox::warning(
            this,
            tr("No files selected"),
            tr("Select some files to process in file select menu.")
        );
        return;
    }

    if (replace && ui->searchColumnSelect->currentIndex() <= 1) {
        QMessageBox::warning(
            this,
            tr("Invalid column for replace"),
            tr("Replace can only be performed in a certain column.")
        );
        return;
    }

    const QString searchText = ui->searchInput->toPlainText();

    if (searchText.isEmpty()) {
        QMessageBox::warning(
            this,
            tr("No search text"),
            tr("Search input is empty.")
        );
        return;
    }

    QString replaceText;

    if (replace) {
        replaceText = ui->replaceInput->toPlainText();

        if (replaceText.isEmpty()) {
            const auto pressed = QMessageBox::question(
                this,
                tr("Replace input is empty"),
                tr("Do you really want to replace the text to nothing?")
            );

            if (pressed != QMessageBox::Yes) {
                return;
            }
        }
    }

    auto searchFlags = SearchFlags(0);

    if (ui->sensitivityButton->isChecked()) {
        searchFlags ^= SearchFlags::CaseSensitive;
    }

    if (ui->wholeButton->isChecked()) {
        searchFlags ^= SearchFlags::WholeWord;
    };

    if (ui->regexButton->isChecked()) {
        searchFlags ^= SearchFlags::RegExp;
    };

    if (ui->commentButton->isChecked()) {
        searchFlags ^= SearchFlags::Comment;
    };

    if (ui->putButton->isChecked()) {
        searchFlags ^= SearchFlags::Put;
    };

    emit actionRequested(
        fileSelectMenu->selected(),
        replace ? (ui->putButton->isChecked() ? Action::Put : Action::Replace)
                : Action::Search,
        searchText,
        replaceText,
        SearchLocation(
            ui->searchLocationSelect->currentIndex() == 0
                ? 3
                : ui->searchLocationSelect->currentIndex()
        ),
        ui->searchColumnSelect->currentIndex() - 1,
        searchFlags
    );
}

SearchMenu::SearchMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    ui(setupUi()),

    fileSelectMenu(new FileSelectMenu(parent)) {
    setDragMoveEnabled(true);

    connect(ui->searchButton, &QPushButton::pressed, this, [=, this] -> void {
        requestAction(false);
    });

    connect(ui->replaceButton, &QPushButton::pressed, this, [=, this] -> void {
        requestAction(true);
    });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });

    connect(this, &PersistentMenu::hidden, this, [this] -> void {
        fileSelectMenu->hide();
        ui->fileSelectButton->setChecked(false);
    });
}

SearchMenu::~SearchMenu() {
    delete ui;
}

auto SearchMenu::setupUi() -> Ui::SearchMenu* {
    auto* const ui_ = new Ui::SearchMenu();
    ui_->setupUi(this);
    return ui_;
}

void SearchMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    PersistentMenu::changeEvent(event);
};

void SearchMenu::clear() {
    for (const u8 index : range<u8>(2, ui->searchColumnSelect->count())) {
        ui->searchColumnSelect->removeItem(index);
    }

    ui->searchInput->clear();
    ui->replaceInput->clear();

    ui->sensitivityButton->setChecked(false);
    ui->wholeButton->setChecked(false);
    ui->regexButton->setChecked(false);
    ui->commentButton->setChecked(false);
    ui->fileSelectButton->setChecked(false);

    fileSelectMenu->clear();

    ui->searchColumnSelect->setCurrentIndex(0);
    ui->searchLocationSelect->setCurrentIndex(0);
}

void SearchMenu::addColumn(const QString& name) {
    ui->searchColumnSelect->addItem(name);
};

void SearchMenu::renameColumn(const u8 index, const QString& name) {
    ui->searchColumnSelect->setItemText(index + 1, name);
};

auto SearchMenu::replaceText() const -> QString {
    return ui->replaceInput->toPlainText();
};

void SearchMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file, true);
}