#include "WriteMenu.hpp"

#include "FileSelectMenu.hpp"
#include "ui_WriteMenu.h"

WriteMenu::WriteMenu(QWidget* const parent) :
    QDialog(parent, Qt::FramelessWindowHint | Qt::Popup),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    connect(ui->applyButton, &QPushButton::pressed, this, [this] -> void {
        accept();
    });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });
}

WriteMenu::~WriteMenu() {
    delete ui;
}

auto WriteMenu::setupUi() -> Ui::WriteMenu* {
    auto* const ui_ = new Ui::WriteMenu();
    ui_->setupUi(this);
    return ui_;
}

void WriteMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QDialog::changeEvent(event);
};

void WriteMenu::hideEvent(QHideEvent* const event) {
    fileSelectMenu->hide();
    QDialog::hideEvent(event);
};

void WriteMenu::clear() {
    fileSelectMenu->clear();
}

void WriteMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file);
}

auto WriteMenu::selected(const bool skipped) const -> Selected {
    return fileSelectMenu->selected(skipped);
}