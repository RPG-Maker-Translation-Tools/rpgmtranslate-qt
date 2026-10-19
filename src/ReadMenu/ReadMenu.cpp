#include "ReadMenu.hpp"

#include "FileSelectMenu.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_ReadMenu.h"

namespace {
constexpr i32 MENU_WIDTH = 640;
constexpr i32 MENU_MIN_HEIGHT = 480;
}  // namespace

ReadMenu::ReadMenu(QWidget* const parent) :
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(setupUi()),
    fileSelectMenu(new FileSelectMenu(parent)) {
    ui->iniTitleDisplayWidget->hide();

    setFixedWidth(MENU_WIDTH);
    setMinimumHeight(MENU_MIN_HEIGHT);
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

                break;
            case ReadMode::DefaultForce:
                ui->duplicateModeSelect->setCurrentIndex(scast<i32>(DuplicateMode::Allow));
                ui->duplicateModeSelect->setEnabled(true);
                // Unlike the append cases, `projectSettings` may be unset here (a new project via `exec()`), so this
                // resets to UTF-8 instead of reading `projectSettings->readEncoding`.
                ui->encodingSelect->setCurrentIndex(0);
                ui->encodingSelect->setEnabled(true);
                break;
            case ReadMode::AppendDefault:
                ui->duplicateModeSelect->setCurrentIndex(scast<i32>(projectSettings->duplicateMode));
                ui->duplicateModeSelect->setEnabled(false);
                // A stored "" (older versions' UTF-8) matches no item: fall back to the first one, UTF-8
                ui->encodingSelect->setCurrentIndex(
                    max(0, ui->encodingSelect->findText(projectSettings->readEncoding))
                );
                ui->encodingSelect->setEnabled(false);
                break;
            case ReadMode::AppendForce:
                ui->duplicateModeSelect->setCurrentIndex(scast<i32>(projectSettings->duplicateMode));
                ui->duplicateModeSelect->setEnabled(false);
                // A stored "" (older versions' UTF-8) matches no item: fall back to the first one, UTF-8
                ui->encodingSelect->setCurrentIndex(
                    max(0, ui->encodingSelect->findText(projectSettings->readEncoding))
                );
                ui->encodingSelect->setEnabled(false);
                break;
        }

        updateModeLabels();
    });

    connect(ui->duplicateModeSelect, &QComboBox::currentIndexChanged, this, [this] -> void { updateModeLabels(); });

    // Shared by the checkbox handler below and by `iniEncodingSelect`'s own handler further down
    const auto updateIniTitleLabel = [this] -> void {
        ui->iniTitleLabel->setText(u"[%1] %2"_qsv.arg(
            svtostr(iniFileName),
            QStringDecoder(ui->iniEncodingSelect->currentText()).decode(QByteArrayView(title_.ptr, title_.len))
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
        ui->iniEncodingSelect,
        &QComboBox::currentTextChanged,
        this,
        [this, updateIniTitleLabel](const QString& /* encoding */) -> void {
        if (title_.ptr != nullptr) {
            updateIniTitleLabel();
        }
    }
    );

    connect(ui->applyButton, &QPushButton::clicked, this, [this] -> void {
        emit accepted();
        hide();
    });

    // `hideEvent` emits `rejected()`
    connect(ui->closeButton, &QPushButton::clicked, this, [this] -> void { hide(); });

    connect(ui->fileSelectButton, &QPushButton::clicked, this, [this] -> void {
        fileSelectMenu->setHidden(!fileSelectMenu->isHidden());
        fileSelectMenu->move(mapToGlobal(QPoint(width(), 0)));
    });

    ui->readModeSelect->setCurrentIndex(0);
    ui->duplicateModeSelect->setCurrentIndex(scast<i32>(DuplicateMode::Allow));
};

ReadMenu::~ReadMenu() {
    rpgm_buffer_free(title_);
    delete ui;
}

auto ReadMenu::setupUi() -> Ui::ReadMenu* {
    auto* const ui_ = new Ui::ReadMenu();
    ui_->setupUi(this);
    return ui_;
}

void ReadMenu::updateEngineWidgets() {
    const bool hasIni = engineType != EngineType::MVMZ;

    ui->iniTitleWidget->setVisible(hasIni);
    ui->encodingWidget->setVisible(
        engineType == EngineType::XP || engineType == EngineType::VX || engineType == EngineType::RM2K
    );
    ui->archiveEncodingWidget->setVisible(
        engineType == EngineType::XP || engineType == EngineType::VX || engineType == EngineType::VXAce
    );

    if (!hasIni) {
        ui->useIniTitleCheckbox->setChecked(false);
    }
}

// The descriptions of the selected modes. They are set in code, so `retranslateUi` can't translate them.
void ReadMenu::updateModeLabels() {
    switch (ReadMode(ui->readModeSelect->currentIndex())) {
        case ReadMode::Default:
            ui->readModeLabel->setText(tr("Parses the game text."));
            break;
        case ReadMode::DefaultForce:
            ui->readModeLabel->setText(tr("Force rewrites existing translation files."));
            break;
        case ReadMode::AppendDefault:
            ui->readModeLabel->setText(tr(
                "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event. Default mode does nothing, when the source files are unchanged since the last read - in this case use force append mode."
            ));
            break;
        case ReadMode::AppendForce:
            ui->readModeLabel->setText(tr(
                "Appends any new text from the game to the translation files, if the text is not already present. Lines order is sorted, unused lines go to the bottom of the map/event."
            ));
            break;
    }

    switch (DuplicateMode(ui->duplicateModeSelect->currentIndex())) {
        case DuplicateMode::Allow:
            ui->duplicateModeLabel->setText(tr(
                "Allow duplicates across maps and events. Recommended: every occurrence keeps its own translation. This mode is always set for system, scripts, and plugins files."
            ));
            break;
        case DuplicateMode::Remove:
            ui->duplicateModeLabel->setText(tr(
                "Remove duplicates across maps and events: a repeated line is translated once, which makes the translation smaller. In system, scripts and plugins files this mode is always overridden by allow mode."
            ));
            break;
    }
}

void ReadMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        updateModeLabels();
    }

    QWidget::changeEvent(event);
};

void ReadMenu::fitHeight() {
    const i32 height =
        max({ MENU_MIN_HEIGHT, layout()->totalHeightForWidth(MENU_WIDTH), layout()->totalMinimumSize().height() });

    if (height != minimumHeight()) {
        setMinimumHeight(height);
        resize(MENU_WIDTH, height);
    }
}

auto ReadMenu::event(QEvent* const event) -> bool {
    const bool handled = QWidget::event(event);

    if (event->type() == QEvent::LayoutRequest) {
        fitHeight();
    }

    return handled;
}

void ReadMenu::showEvent(QShowEvent* const event) {
    QWidget::showEvent(event);
    fitHeight();
}

void ReadMenu::hideEvent(QHideEvent* const event) {
    // Whoever hides the menu (the Close button, a closing project) answers the `exec()` waiting for it; after
    // `accepted()` that first answer is the one that counts
    emit rejected();
    fileSelectMenu->hide();
    QWidget::hideEvent(event);
};

void ReadMenu::clear() {
    fileSelectMenu->clear();

    rpgm_buffer_free(title_);
    title_ = ByteBuffer{};
    ui->useIniTitleCheckbox->setChecked(false);

    ui->readModeSelect->setCurrentIndex(0);
    ui->duplicateModeSelect->setCurrentIndex(scast<i32>(DuplicateMode::Allow));
    ui->encodingSelect->setCurrentIndex(0);
    ui->archiveEncodingSelect->setCurrentIndex(0);

    ui->readModeSelect->setDisabled(true);
    ui->duplicateModeSelect->setDisabled(false);
    ui->encodingSelect->setDisabled(false);

    // Both need a translation to exist, so they stay off until `init()` runs for a project that has one
    ui->ignoreCheckbox->setChecked(false);
    ui->ignoreCheckbox->setDisabled(true);
    ui->skipObsoleteCheckbox->setChecked(false);
    ui->skipObsoleteCheckbox->setDisabled(true);
    ui->mapEventsCheckbox->setChecked(false);
}

void ReadMenu::init(const ProjectSettings* const settings) {
    projectSettings = settings;

    projectPath = projectSettings->projectPath.toUtf8();
    engineType = projectSettings->engineType;

    updateEngineWidgets();
    // The archive is extracted only on first open (`exec()`), so an open project has nothing to apply it to
    ui->archiveEncodingWidget->hide();

    ui->readModeSelect->setEnabled(true);
    // Triggers the AppendDefault case above, which pre-fills `encodingSelect`
    // (and `duplicateModeSelect`) from `projectSettings` and locks them.
    ui->readModeSelect->setCurrentIndex(2);

    ui->ignoreCheckbox->setEnabled(true);
    ui->skipObsoleteCheckbox->setEnabled(true);
}

auto ReadMenu::exec(const QString& projectPath, const EngineType engineType, const bool hasArchive)
    -> QDialog::DialogCode {
    this->projectPath = projectPath.toUtf8();
    this->engineType = engineType;

    updateEngineWidgets();

    // Only an archive has file names to decode; a project that already has its data directory has nothing to extract
    if (!hasArchive) {
        ui->archiveEncodingWidget->hide();
    }

    QEventLoop loop;
    auto code = QDialog::DialogCode::Rejected;
    bool answered = false;

    const auto answer = [&loop, &code, &answered](const QDialog::DialogCode result) -> void {
        if (!answered) {
            answered = true;
            code = result;
            loop.quit();
        }
    };

    connect(this, &ReadMenu::accepted, &loop, [&answer] -> void { answer(QDialog::DialogCode::Accepted); });
    connect(this, &ReadMenu::rejected, &loop, [&answer] -> void { answer(QDialog::DialogCode::Rejected); });

    loop.exec();

    this->projectPath = QByteArray();

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

auto ReadMenu::archiveReadEncoding() const -> QString {
    return ui->archiveEncodingSelect->currentText();
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

auto ReadMenu::title() const -> QString {
    if (!ui->useIniTitleCheckbox->isChecked() || title_.ptr == nullptr) {
        return {};
    }

    return QStringDecoder(ui->iniEncodingSelect->currentText()).decode(QByteArrayView(title_.ptr, title_.len));
}

void ReadMenu::init(const vector<TabListItem>& files) {
    fileSelectMenu->init(files);
}