#include "AboutWindow.hpp"

#include "Constants.hpp"
#include "ui_AboutWindow.h"
#include "version.h"

AboutWindow::AboutWindow(QWidget* const parent) :
    QDialog(parent),
    ui(setupUi()) {
    ui->versionLabel->setText(u"RPGMTranslate v"_s + APP_VERSION);
    ui->qtVersionLabel->setText(u"Qt "_s + qVersion());
}

AboutWindow::~AboutWindow() {
    delete ui;
}

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* const ui_ = new Ui::AboutWindow();
    ui_->setupUi(this);
    return ui_;
}

void AboutWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QDialog::changeEvent(event);
}
