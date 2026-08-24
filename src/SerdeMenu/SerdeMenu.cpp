#include "SerdeMenu.hpp"

#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ui_SerdeMenu.h"

#include <QFileDialog>

namespace {
// Returns the format's display name if its rvpacker-txt-rs-lib feature wasn't compiled into
// this build, or nullptr if the format is available.
[[nodiscard]] auto unavailableSerdeFormatName(const SerdeFormat format) -> const char* {
    switch (format) {
        case SerdeFormat::Csv:
#ifndef ENABLE_SERDE_CSV
            return "CSV";
#else
            return nullptr;
#endif
        case SerdeFormat::Xlsx:
#ifndef ENABLE_SERDE_XLSX
            return "XLSX";
#else
            return nullptr;
#endif
        case SerdeFormat::Xml:
#ifndef ENABLE_SERDE_XML
            return "XML";
#else
            return nullptr;
#endif
        case SerdeFormat::Json:
            return nullptr;
        case SerdeFormat::Yaml:
#ifndef ENABLE_SERDE_YAML
            return "YAML";
#else
            return nullptr;
#endif
    }

    std::unreachable();
}
}  // namespace

SerdeMenu::SerdeMenu(QWidget* const parent) :
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    hide();
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(u"SerdeMenu { background-color: palette(window) }"_s);

    connect(ui->browseButton, &QPushButton::pressed, this, [this] -> void {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Select a folder"), ui->directoryInput->text());

        if (!dir.isEmpty()) {
            ui->directoryInput->setText(dir);
        }
    });

    connect(ui->exportButton, &QPushButton::pressed, this, [this] -> void {
        if (ui->directoryInput->text().isEmpty()) {
            present(this, NOTICE("Pick a destination folder first.", Warning, Modal));
            return;
        }

        if (const auto* const unavailable = unavailableSerdeFormatName(format())) {
            present(
                this,
                NOTICE("%1 support was not compiled into this build.", Warning, Modal, QString::fromLatin1(unavailable))
            );
            return;
        }

        emit exportRequested();
        hide();
    });

    connect(ui->importButton, &QPushButton::pressed, this, [this] -> void {
        if (ui->directoryInput->text().isEmpty()) {
            present(this, NOTICE("Pick a source folder first.", Warning, Modal));
            return;
        }

        if (const auto* const unavailable = unavailableSerdeFormatName(format())) {
            present(
                this,
                NOTICE("%1 support was not compiled into this build.", Warning, Modal, QString::fromLatin1(unavailable))
            );
            return;
        }

        emit importRequested();
        hide();
    });

    connect(ui->closeButton, &QPushButton::pressed, this, [this] -> void { hide(); });

    connect(ui->fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });
}

SerdeMenu::~SerdeMenu() {
    delete ui;
}

auto SerdeMenu::setupUi() -> Ui::SerdeMenu* {
    auto* const ui_ = new Ui::SerdeMenu();
    ui_->setupUi(this);
    return ui_;
}

void SerdeMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
};

void SerdeMenu::hideEvent(QHideEvent* const event) {
    fileSelectMenu->hide();
    QWidget::hideEvent(event);
};

void SerdeMenu::clear() {
    ui->directoryInput->clear();
    fileSelectMenu->clear();
}

auto SerdeMenu::selected(const bool skipped) const -> Selected {
    return fileSelectMenu->selected(skipped);
}

auto SerdeMenu::format() const -> SerdeFormat {
    return SerdeFormat(ui->formatSelect->currentIndex());
}

auto SerdeMenu::directory() const -> QString {
    return ui->directoryInput->text();
}

void SerdeMenu::init(const vector<TabListItem>& files) {
    fileSelectMenu->init(files);
}
