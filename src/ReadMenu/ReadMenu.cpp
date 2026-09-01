#include "ReadMenu.hpp"

#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_ReadMenu.h"

ReadMenu::ReadMenu(QWidget* const parent) :
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    ui->iniTitleDisplayWidget->hide();

    hide();

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(u"ReadMenu { background-color: palette(window) }"_s);

    connect(ui->readModeSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
        switch (ReadMode(index)) {
            case ReadMode::Default:
                if (!fileSelectMenu->empty()) {
                    present(this, NOTICE("Default mode does nothing when files are already read.", Warning, Modal));

                    ui->readModeSelect->setCurrentIndex(2);
                    return;
                }

                ui->readModeLabel->setText(tr("Parses the game text."));
                break;
            case ReadMode::DefaultForce:
                ui->readModeLabel->setText(tr("Force rewrites existing translation files."));

                ui->duplicateModeSelect->setCurrentIndex(false);
                ui->duplicateModeSelect->setEnabled(true);
                // Unlike the append cases below, this can run before `init()`
                // ever assigns `projectSettings` (a brand-new project opened
                // through the standalone `exec()` overload), so it resets to
                // "guess" rather than reading `projectSettings->readEncoding`.
                ui->encodingSelect->setCurrentIndex(0);
                ui->encodingSelect->setEnabled(true);
                break;
            case ReadMode::AppendDefault:
                ui->readModeLabel->setText(tr(
                    "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event. Default mode does nothing, when the source files are unchanged since the last read - in this case use force append mode."
                ));

                ui->duplicateModeSelect->setCurrentIndex(scast<i32>(projectSettings->duplicateMode));
                ui->duplicateModeSelect->setEnabled(false);
                ui->encodingSelect->setCurrentText(projectSettings->readEncoding);
                ui->encodingSelect->setEnabled(false);
                break;
            case ReadMode::AppendForce:
                ui->readModeLabel->setText(tr(
                    "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event."
                ));

                ui->duplicateModeSelect->setCurrentIndex(scast<i32>(projectSettings->duplicateMode));
                ui->duplicateModeSelect->setEnabled(false);
                ui->encodingSelect->setCurrentText(projectSettings->readEncoding);
                ui->encodingSelect->setEnabled(false);
                break;
        }

        adjustSize();
    });

    connect(ui->duplicateModeSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
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
    });

    // Shared by the checkbox handler below and by `encodingSelect`'s own handler further down -
    // the ini title always decodes with whatever encoding "Text Encoding" currently specifies,
    // there is no separate encoding control for it.
    const auto updateIniTitleLabel = [this] -> void {
        const QString encoding = ui->encodingSelect->currentText();

        ui->iniTitleLabel->setText(u"[%1] %2"_qsv.arg(
            svtostr(iniFileName),
            QStringDecoder(encoding.isEmpty() ? u"UTF-8"_s : encoding).decode(QByteArrayView(title_.ptr, title_.len))
        ));
    };

    connect(
        ui->useIniTitleCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [this, updateIniTitleLabel](const Qt::CheckState state) -> void {
        if (state == Qt::CheckState::Checked) {
            rpgm_buffer_free(title_);
            title_ = ByteBuffer{};

            bool isRm2k = false;

            const bool success = rpgm_get_ini_title(
                FFIString{ .ptr = projectPath.data(), .len = scast<u32>(projectPath.size()) },
                &title_,
                &isRm2k
            );

            iniFileName = isRm2k ? u"RPG_RT.ini"_qsv : u"Game.ini"_qsv;

            if (!success) {
                const QUtf8SV error = ffitostr(rpgm_error());
                present(
                    this,
                    NOTICE(
                        "Failed to extract title from the Game.ini/RPG_RT.ini file: %1",
                        Critical,
                        Modal,
                        svtostr(error)
                    )
                );
                ui->useIniTitleCheckbox->setChecked(false);
                return;
            }

            if (title_.len == 0) {
                present(this, NOTICE("Title is empty in %1 file.", Warning, Modal, svtostr(iniFileName)));
                ui->useIniTitleCheckbox->setChecked(false);
                rpgm_buffer_free(title_);
                title_ = ByteBuffer{};
                return;
            }

            ui->iniTitleDisplayWidget->show();
            updateIniTitleLabel();
        } else {
            ui->iniTitleDisplayWidget->hide();
        }
    }
    );

    connect(
        ui->encodingSelect,
        &QComboBox::currentTextChanged,
        this,
        [this, updateIniTitleLabel](const QString& /* encoding */) -> void {
        if (title_.ptr != nullptr) {
            updateIniTitleLabel();
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
    ui->encodingSelect->setCurrentIndex(0);

    ui->readModeSelect->setDisabled(true);
    ui->duplicateModeSelect->setDisabled(false);
    ui->encodingSelect->setDisabled(false);

    ui->ignoreCheckbox->setChecked(false);
    ui->skipObsoleteCheckbox->setChecked(false);
    ui->mapEventsCheckbox->setChecked(false);
}

void ReadMenu::init(const shared_ptr<ProjectSettings>& settings) {
    projectSettings = settings;

    projectPath = projectSettings->projectPath.toUtf8();
    engineType = projectSettings->engineType;

    if (engineType == EngineType::MVMZ) {
        ui->iniTitleWidget->hide();
    } else {
        ui->iniTitleWidget->show();
    }

    ui->readModeSelect->setEnabled(true);
    // Triggers the AppendForce case above, which pre-fills `encodingSelect`
    // (and `duplicateModeSelect`) from `projectSettings` and locks them.
    ui->readModeSelect->setCurrentIndex(2);

    ui->ignoreCheckbox->setEnabled(true);
    ui->skipObsoleteCheckbox->setEnabled(true);
}

auto ReadMenu::exec(const QString& projectPath, const EngineType engineType) -> QDialog::DialogCode {
    this->projectPath = projectPath.toUtf8();
    this->engineType = engineType;

    if (engineType == EngineType::MVMZ) {
        ui->iniTitleWidget->hide();
    } else {
        ui->iniTitleWidget->show();
    }

    QEventLoop loop;
    QDialog::DialogCode code;

    connect(this, &ReadMenu::accepted, &loop, [&loop, &code] -> void {
        loop.quit();
        code = QDialog::DialogCode::Accepted;
    });

    connect(this, &ReadMenu::rejected, &loop, [&loop, &code] -> void {
        loop.quit();
        code = QDialog::DialogCode::Rejected;
    });

    loop.exec();

    this->projectPath = QByteArray();

    if (title_.ptr != nullptr) {
        const QString encoding = ui->encodingSelect->currentText();
        decodedTitle =
            QStringDecoder(encoding.isEmpty() ? u"UTF-8"_s : encoding).decode(QByteArrayView(title_.ptr, title_.len));

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

auto ReadMenu::readEncoding() const -> QString {
    return ui->encodingSelect->currentText();
};

auto ReadMenu::flags() const -> BaseFlags {
    auto flags = scast<BaseFlags>(0);

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
    return ui->useIniTitleCheckbox->isChecked() ? std::move(decodedTitle) : QString();
}

void ReadMenu::init(const vector<TabListItem>& files) {
    fileSelectMenu->init(files);
}