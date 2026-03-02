#include "PurgeMenu.hpp"

#include "FileSelectMenu.hpp"
#include "ui_PurgeMenu.h"

PurgeMenu::PurgeMenu(QWidget* const parent) :
    QWidget(parent),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    hide();
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("PurgeMenu { background-color: %1 }"_L1.arg(
        qApp->palette().color(QPalette::Window).name()
    ));

    connect(ui->applyButton, &QPushButton::pressed, this, [this] -> void {
        emit accepted();
        hide();
    });

    connect(ui->closeButton, &QPushButton::pressed, this, [this] -> void {
        emit rejected();
        hide();
    });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });
}

PurgeMenu::~PurgeMenu() {
    delete ui;
}

auto PurgeMenu::setupUi() -> Ui::PurgeMenu* {
    auto* const ui_ = new Ui::PurgeMenu();
    ui_->setupUi(this);
    return ui_;
}

void PurgeMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
};

void PurgeMenu::hideEvent(QHideEvent* const event) {
    fileSelectMenu->hide();
    QWidget::hideEvent(event);
};

void PurgeMenu::clear() {
    ui->createIgnoreCheckbox->setChecked(false);
    fileSelectMenu->clear();
}

void PurgeMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file);
}

auto PurgeMenu::selected(const bool skipped) const -> Selected {
    return fileSelectMenu->selected(skipped);
}