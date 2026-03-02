#include "ReadMenu.hpp"

#include "Enums.hpp"
#include "FileSelectMenu.hpp"
#include "ProjectSettings.hpp"
#include "ui_ReadMenu.h"

#include <QMessageBox>

ReadMenu::ReadMenu(QWidget* const parent) :
    QWidget(parent),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    hide();
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ReadMenu { background-color: %1 }"_L1.arg(
        qApp->palette().color(QPalette::Window).name()
    ));

    connect(
        ui->readModeSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u8 index) -> void {
        switch (ReadMode(index)) {
            case ReadMode::Default:
                if (!fileSelectMenu->empty()) {
                    QMessageBox::warning(
                        nullptr,
                        tr("Invalid mode"),
                        tr(
                            "Default mode does nothing when files are already read."
                        )
                    );

                    ui->readModeSelect->setCurrentIndex(2);
                    return;
                }

                ui->readModeLabel->setText(tr("Parses the game text."));
                break;
            case ReadMode::DefaultForce:
                ui->readModeLabel->setText(
                    tr("Force rewrites existing translation files.")
                );

                ui->duplicateModeSelect->setCurrentIndex(false);
                ui->duplicateModeSelect->setEnabled(true);

                ui->romanizeCheckbox->setChecked(false);
                ui->romanizeCheckbox->setEnabled(true);
                ui->trimCheckbox->setChecked(false);
                ui->trimCheckbox->setEnabled(true);
                ui->disableCustomProcessingCheckbox->setChecked(false);
                ui->disableCustomProcessingCheckbox->setEnabled(true);
                break;
            case ReadMode::AppendDefault:
                ui->readModeLabel->setText(tr(
                    "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event. Default mode does nothing, when the source files are unchanged since the last read - in this case use force append mode."
                ));

                ui->duplicateModeSelect->setCurrentIndex(
                    u8(projectSettings->duplicateMode)
                );
                ui->duplicateModeSelect->setEnabled(false);

                ui->romanizeCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags::Romanize)
                );
                ui->romanizeCheckbox->setEnabled(false);
                ui->trimCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags::Trim)
                );
                ui->trimCheckbox->setEnabled(false);
                ui->disableCustomProcessingCheckbox->setChecked(
                    bool(
                        projectSettings->flags &
                        BaseFlags::DisableCustomProcessing
                    )
                );
                ui->disableCustomProcessingCheckbox->setEnabled(false);
                break;
            case ReadMode::AppendForce:
                ui->readModeLabel->setText(tr(
                    "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event."
                ));

                ui->duplicateModeSelect->setCurrentIndex(
                    u8(projectSettings->duplicateMode)
                );
                ui->duplicateModeSelect->setEnabled(false);

                ui->romanizeCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags::Romanize)
                );
                ui->romanizeCheckbox->setEnabled(false);
                ui->trimCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags::Trim)
                );
                ui->trimCheckbox->setEnabled(false);
                ui->disableCustomProcessingCheckbox->setChecked(
                    bool(
                        projectSettings->flags &
                        BaseFlags::DisableCustomProcessing
                    )
                );
                ui->disableCustomProcessingCheckbox->setEnabled(false);
                break;
        }

        adjustSize();
    }
    );

    connect(
        ui->duplicateModeSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u8 index) -> void {
        switch (DuplicateMode(index)) {
            case DuplicateMode::Allow:
                ui->duplicateModeLabel->setText(tr(
                    "Allow duplicates across maps and events. This may bloat your translation. This mode is always set for system, scripts, and plugins files."
                ));
                break;
            case DuplicateMode::Remove:
                ui->duplicateModeLabel->setText(tr(
                    "Remove duplicates across maps and events. Recommended. In system, scripts and plugins files this mode is always overridden by allow mode."
                ));
                break;
        }
    }
    );

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

    ui->readModeSelect->setCurrentIndex(0);
    ui->duplicateModeSelect->setCurrentIndex(0);
};

ReadMenu::~ReadMenu() {
    delete ui;
}

auto ReadMenu::setupUi() -> Ui::ReadMenu* {
    auto* const ui_ = new Ui::ReadMenu();
    ui_->setupUi(this);
    return ui_;
}

void ReadMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
};

void ReadMenu::hideEvent(QHideEvent* const event) {
    fileSelectMenu->hide();
    QWidget::hideEvent(event);
};

void ReadMenu::clear() {
    fileSelectMenu->clear();

    ui->readModeSelect->setCurrentIndex(0);
    ui->duplicateModeSelect->setCurrentIndex(1);

    ui->readModeSelect->setDisabled(true);
    ui->duplicateModeSelect->setDisabled(false);

    ui->romanizeCheckbox->setChecked(false);
    ui->trimCheckbox->setChecked(false);
    ui->disableCustomProcessingCheckbox->setChecked(false);
    ui->ignoreCheckbox->setChecked(false);
    ui->skipObsoleteCheckbox->setChecked(false);
    ui->mapEventsCheckbox->setChecked(false);
}

void ReadMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file);
}

void ReadMenu::init(const shared_ptr<ProjectSettings>& settings) {
    projectSettings = settings;

    ui->readModeSelect->setEnabled(true);
    ui->readModeSelect->setCurrentIndex(2);

    ui->ignoreCheckbox->setEnabled(true);
    ui->skipObsoleteCheckbox->setEnabled(true);
}

[[nodiscard]] auto ReadMenu::parseMapEvents() const -> bool {
    return ui->mapEventsCheckbox->isChecked();
};

[[nodiscard]] auto ReadMenu::readMode() const -> ReadMode {
    return ReadMode(ui->readModeSelect->currentIndex());
};

[[nodiscard]] auto ReadMenu::duplicateMode() const -> DuplicateMode {
    return DuplicateMode(ui->duplicateModeSelect->currentIndex());
};

[[nodiscard]] auto ReadMenu::flags() const -> BaseFlags {
    auto flags = BaseFlags(0);

    if (ui->romanizeCheckbox->isChecked()) {
        flags |= BaseFlags::Romanize;
    }

    if (ui->trimCheckbox->isChecked()) {
        flags |= BaseFlags::Trim;
    }

    if (ui->disableCustomProcessingCheckbox->isChecked()) {
        flags |= BaseFlags::DisableCustomProcessing;
    }

    if (ui->ignoreCheckbox->isChecked()) {
        flags |= BaseFlags::Ignore;
    }

    if (ui->skipObsoleteCheckbox->isChecked()) {
        flags |= BaseFlags::SkipObsolete;
    }

    return flags;
};

[[nodiscard]] auto ReadMenu::selected(const bool skipped) const -> Selected {
    return fileSelectMenu->selected(skipped);
};