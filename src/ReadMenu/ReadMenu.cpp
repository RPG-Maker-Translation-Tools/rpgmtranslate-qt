#include "ReadMenu.hpp"

#include "FileSelectMenu.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"
#include "ui_ReadMenu.h"

#include <QMessageBox>

ReadMenu::ReadMenu(QWidget* const parent) :
    QWidget(parent),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    ui->iniTitleDisplayWidget->hide();

    hide();

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(u"ReadMenu { background-color: palette(window) }"_s);

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
                    bool(projectSettings->flags & BaseFlags_Romanize)
                );
                ui->romanizeCheckbox->setEnabled(false);
                ui->trimCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags_Trim)
                );
                ui->trimCheckbox->setEnabled(false);
                ui->disableCustomProcessingCheckbox->setChecked(
                    bool(
                        projectSettings->flags &
                        BaseFlags_DisableCustomProcessing
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
                    bool(projectSettings->flags & BaseFlags_Romanize)
                );
                ui->romanizeCheckbox->setEnabled(false);
                ui->trimCheckbox->setChecked(
                    bool(projectSettings->flags & BaseFlags_Trim)
                );
                ui->trimCheckbox->setEnabled(false);
                ui->disableCustomProcessingCheckbox->setChecked(
                    bool(
                        projectSettings->flags &
                        BaseFlags_DisableCustomProcessing
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

    connect(
        ui->useIniTitleCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [this](const Qt::CheckState state) -> void {
        if (state == Qt::CheckState::Checked) {
            const bool success = rpgm_get_ini_title(
                FFIString{ .ptr = projectPath.data(),
                           .len = u32(projectPath.size()) },
                &title_
            );

            if (!success) {
                const QUtf8SV error = ffitostr(rpgm_error());
                qCritical()
                    << "Failed to extract title from the Game.ini file: %1"_L1
                           .arg(
                               error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                                   .toString()
#endif
                           );
                QMessageBox::critical(
                    this,
                    tr("Failed to extract INI title"),
                    tr("Failed to extract title from the Game.ini file: %1")
                        .arg(
                            error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                                .toString()
#endif
                        )
                );
                return;
            }

            if (title_.len == 0) {
                qWarning() << "Title is empty in Game.ini file"_L1;
                QMessageBox::warning(
                    this,
                    tr("Title is empty"),
                    tr("Title is empty in Game.ini file.")
                );
                rpgm_buffer_free(title_);
                return;
            }

            ui->iniTitleDisplayWidget->show();

            ui->titleEncodingSelect->setCurrentText(u"UTF-8"_s);

            ui->iniTitleLabel->setText(
                QStringDecoder(QStringDecoder::Utf8)
                    .decode(QByteArrayView(title_.ptr, title_.len))
            );
        } else {
            ui->iniTitleDisplayWidget->hide();
        }
    }
    );

    connect(
        ui->titleEncodingSelect,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString& encoding) -> void {
        ui->iniTitleLabel->setText(QStringDecoder(encoding).decode(
            QByteArrayView(title_.ptr, title_.len)
        ));
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

void ReadMenu::init(const shared_ptr<ProjectSettings>& settings) {
    projectSettings = settings;

    projectPath = projectSettings->projectPath.toUtf8();
    engineType = projectSettings->engineType;

    if (engineType == EngineType::New) {
        ui->iniTitleWidget->hide();
    } else {
        ui->iniTitleWidget->show();
    }

    ui->readModeSelect->setEnabled(true);
    ui->readModeSelect->setCurrentIndex(2);

    ui->ignoreCheckbox->setEnabled(true);
    ui->skipObsoleteCheckbox->setEnabled(true);
}

auto ReadMenu::exec(const QString& projectPath, const EngineType engineType)
    -> QDialog::DialogCode {
    this->projectPath = projectPath.toUtf8();
    this->engineType = engineType;

    if (engineType == EngineType::New) {
        ui->iniTitleWidget->hide();
    } else {
        ui->iniTitleWidget->show();
    }

    QEventLoop loop;
    QDialog::DialogCode code;

    connect(this, &ReadMenu::accepted, &loop, [this, &loop, &code] -> void {
        loop.quit();
        code = QDialog::DialogCode::Accepted;
    });

    connect(this, &ReadMenu::rejected, &loop, [this, &loop, &code] -> void {
        loop.quit();
        code = QDialog::DialogCode::Rejected;
    });

    loop.exec();

    this->projectPath = QByteArray();

    if (!ui->titleEncodingSelect->currentText().isEmpty()) {
        decodedTitle = QStringDecoder(ui->titleEncodingSelect->currentText())
                           .decode(QByteArrayView(title_.ptr, title_.len));
    }

    if (title_.ptr != nullptr) {
        rpgm_buffer_free(title_);
        title_ = ByteBuffer{};
    }

    return code;
};

auto ReadMenu::parseMapEvents() const -> bool {
    return ui->mapEventsCheckbox->isChecked();
};

auto ReadMenu::readMode() const -> ReadMode {
    return ReadMode(ui->readModeSelect->currentIndex());
};

auto ReadMenu::duplicateMode() const -> DuplicateMode {
    return DuplicateMode(ui->duplicateModeSelect->currentIndex());
};

auto ReadMenu::flags() const -> BaseFlags {
    auto flags = BaseFlags(0);

    if (ui->romanizeCheckbox->isChecked()) {
        flags |= BaseFlags_Romanize;
    }

    if (ui->trimCheckbox->isChecked()) {
        flags |= BaseFlags_Trim;
    }

    if (ui->disableCustomProcessingCheckbox->isChecked()) {
        flags |= BaseFlags_DisableCustomProcessing;
    }

    if (ui->ignoreCheckbox->isChecked()) {
        flags |= BaseFlags_Ignore;
    }

    if (ui->skipObsoleteCheckbox->isChecked()) {
        flags |= BaseFlags_SkipObsolete;
    }

    return flags;
};

auto ReadMenu::selected(const bool skipped) const -> Selected {
    return fileSelectMenu->selected(skipped);
};

auto ReadMenu::title() -> QString {
    return ui->useIniTitleCheckbox->isChecked() ? std::move(decodedTitle)
                                                : QString();
}

void ReadMenu::setFiles(const vector<TabListItem>& files) {
    fileSelectMenu->setFiles(files);
}