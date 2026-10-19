#include "SerdeMenu.hpp"

#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ui_SerdeMenu.h"

#include <QDir>
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

    connect(ui->browseButton, &QPushButton::clicked, this, [this] -> void {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Select a folder"), ui->directoryInput->text());

        if (!dir.isEmpty()) {
            ui->directoryInput->setText(dir);
        }
    });

    // What both buttons need before they start; false after telling the user what is missing
    const auto ready = [this](const bool exporting) -> bool {
        const QString directory = ui->directoryInput->text();

        if (directory.isEmpty()) {
            present(
                this,
                exporting ? NOTICE("Pick a destination folder first.", Warning, Modal)
                          : NOTICE("Pick a source folder first.", Warning, Modal)
            );
            return false;
        }

        if (fileSelectMenu->selectedCount() == 0) {
            present(this, NOTICE("Select some files to process in file select menu.", Warning, Modal));
            return false;
        }

        if (const auto* const unavailable = unavailableSerdeFormatName(format())) {
            present(
                this,
                NOTICE("%1 support was not compiled into this build.", Warning, Modal, QString::fromLatin1(unavailable))
            );
            return false;
        }

        // Typed by the user: with no such folder every file would fail to be written
        if (exporting && !QDir().mkpath(directory)) {
            present(this, NOTICE("Cannot create the folder %1.", Warning, Modal, directory));
            return false;
        }

        return true;
    };

    connect(ui->exportButton, &QPushButton::clicked, this, [this, ready] -> void {
        if (ready(true)) {
            emit exportRequested();
            hide();
        }
    });

    connect(ui->importButton, &QPushButton::clicked, this, [this, ready] -> void {
        if (ready(false)) {
            emit importRequested();
            hide();
        }
    });
    connect(ui->closeButton, &QPushButton::clicked, this, [this] -> void { hide(); });

    connect(ui->fileSelectButton, &QPushButton::clicked, this, [this] -> void {
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
