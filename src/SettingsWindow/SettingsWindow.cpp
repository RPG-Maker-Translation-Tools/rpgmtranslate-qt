#include "SettingsWindow.hpp"

#include "CBSLIWidget.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "ScriptHost.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_SettingsWindow.h"

#include <QBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDirListing>
#include <QFile>
#include <QFileDialog>
#include <QFontComboBox>
#include <QIntValidator>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStringListModel>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTableWidgetItem>
#include <QUuid>
#include <QtConcurrent>

namespace {
using Dir = SettingsWindow::Direction;

constexpr i32 TRANSLATION_TABLE_FONT_SIZE_MIN = 8;
constexpr i32 TRANSLATION_TABLE_FONT_SIZE_MAX = 96;
constexpr i32 MAX_LINE_LENGTH_HINT = 255;
constexpr i32 LANGUAGETOOL_CONNECTION_TIMEOUT_MS = 20000;

constexpr i32 SEQUENCE_USE_COLOR_COLUMN = 2;
constexpr i32 SEQUENCE_COLOR_COLUMN = 3;

constexpr i32 CUSTOM_LINT_PATTERN_COLUMN = 0;
constexpr i32 CUSTOM_LINT_TOOLTIP_COLUMN = 1;
constexpr i32 CUSTOM_LINT_CASE_COLUMN = 2;
constexpr i32 CUSTOM_LINT_USE_COLOR_COLUMN = 3;
constexpr i32 CUSTOM_LINT_COLOR_COLUMN = 4;

constexpr i32 REPLACEMENT_ENABLED_COLUMN = 2;

constexpr i32 SECTION_COUNT = 5;

// One overload per (widget, field) shape. Every setting travels through exactly one of these in both
// directions, which is what keeps a load from existing without its matching save.

void sync(const Dir dir, QCheckBox* const box, bool& value) {
    if (dir == Dir::Save) {
        value = box->isChecked();
    } else {
        box->setChecked(value);
    }
}

void sync(const Dir dir, QLineEdit* const edit, QString& value) {
    if (dir == Dir::Save) {
        value = edit->text();
    } else {
        edit->setText(value);
    }
}

template <typename T>
    requires std::is_integral_v<T>
void sync(const Dir dir, QLineEdit* const edit, T& value) {
    using Wide = std::conditional_t<std::is_signed_v<T>, i64, u64>;

    if (dir == Dir::Save) {
        value = scast<T>(stoa<Wide>(edit->text()).value_or(0));
    } else {
        edit->setText(QString(qsv(qitos(value))));
    }
}

void sync(const Dir dir, QKeySequenceEdit* const edit, QString& value) {
    if (dir == Dir::Save) {
        value = edit->keySequence().toString();
    } else {
        edit->setKeySequence(QKeySequence(value));
    }
}

void sync(const Dir dir, QPlainTextEdit* const edit, QString& value) {
    if (dir == Dir::Save) {
        value = edit->toPlainText();
    } else {
        edit->setPlainText(value);
    }
}

void sync(const Dir dir, QComboBox* const box, QString& value, bool useData = false) {
    if (dir == Dir::Save) {
        if (useData) {
            value = box->currentData().toString();
        } else {
            value = box->currentText();
        }
    } else {
        if (useData) {
            const i32 found = box->findData(value);

            if (found != -1) {
                box->setCurrentIndex(found);
            } else if (!value.isEmpty()) {
                // Not in the list (hand-edited or from another version): keep it instead of saving "" over it
                box->addItem(value, value);
                box->setCurrentIndex(box->count() - 1);
            }
        } else {
            box->setCurrentText(value);
        }
    }
}

template <typename E>
    requires std::is_enum_v<E>
void sync(const Dir dir, QComboBox* const box, E& value) {
    if (dir == Dir::Save) {
        value = scast<E>(box->currentIndex());
    } else {
        box->setCurrentIndex(scast<i32>(value));
    }
}

void sync(const Dir dir, QFontComboBox* const box, QString& value) {
    if (dir == Dir::Save) {
        value = box->currentFont().family();
    } else {
        box->setCurrentFont(QFont(value));
    }
}

template <typename T>
    requires std::is_integral_v<T>
void sync(const Dir dir, CBSLIWidget* const widget, T& value) {
    if (dir == Dir::Save) {
        value = scast<T>(widget->value());
    } else {
        widget->setValue(scast<i32>(value));
    }
}

void sync(const Dir dir, QLineEdit* const edit, vector<u32>& value) {
    if (dir == Dir::Save) {
        value.clear();

        const QString text = edit->text();

        for (const QStringView part : QStringView(text).split(u',', Qt::SkipEmptyParts)) {
            QStringView trimmed = part.trimmed();

            if (trimmed.startsWith(u"U+", Qt::CaseInsensitive) || trimmed.startsWith(u"0x", Qt::CaseInsensitive)) {
                trimmed = trimmed.sliced(2);
            }

            if (const optional<u32> codePoint = stoa<u32>(trimmed, 16)) {
                value.emplace_back(*codePoint);
            }
        }
    } else {
        QStringList parts;
        parts.reserve(scast<isize>(value.size()));

        for (const u32 codePoint : value) {
            parts.append(u"U+%1"_qsv.arg(qsv(qhtos(codePoint, 4, true))));
        }

        edit->setText(parts.join(u", "_qsv));
    }
}

void syncColorCells(
    const Dir dir,
    QTableWidgetItem* const useColor,
    QTableWidgetItem* const color,
    bool& useCustomColor,
    QRgb& customColor
) {
    if (dir == Dir::Save) {
        useCustomColor = useColor->checkState() == Qt::Checked;
        customColor = color->background().color().rgba();
    } else {
        useColor->setCheckState(useCustomColor ? Qt::Checked : Qt::Unchecked);
        color->setBackground(QBrush(QColor::fromRgba(customColor)));
    }
}

[[nodiscard]] auto listSpellcheckDictionaries() -> QStringList {
    const auto dictionariesPath = dataLocation() + u"/dictionaries";

    if (!QFile::exists(dictionariesPath)) {
        return {};
    }

    const auto listing =
        QDirListing(dictionariesPath, QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::FilesOnly);

    QStringList dictionaries;

    for (const auto& entry : listing) {
        QString filename = entry.filePath();

        if (!filename.endsWith(u".aff")) {
            continue;
        }

        filename.slice(dictionariesPath.size(), filename.size() - dictionariesPath.size());
        dictionaries.append(std::move(filename));
    }

    dictionaries.sort(Qt::CaseInsensitive);
    return dictionaries;
}

[[nodiscard]] auto listLanguageTags() -> QList<pair<QString, QString>> {
    static constexpr array<QLocale::Language, 6> TERRITORY_SENSITIVE_LANGUAGES = {
        QLocale::English, QLocale::Chinese, QLocale::Portuguese, QLocale::German, QLocale::French, QLocale::Spanish,
    };

    static constexpr array<pair<QLocale::Language, array<QLocale::Territory, 4>>, 6> LANGUAGE_RELEVANT_TERRITORIES = {
        pair{ QLocale::English,
              array{ QLocale::UnitedStates, QLocale::UnitedKingdom, QLocale::Australia, QLocale::Canada } },
        { QLocale::Chinese, { QLocale::China, QLocale::Taiwan, QLocale::HongKong } },
        { QLocale::Portuguese, { QLocale::Portugal, QLocale::Brazil } },
        { QLocale::German, { QLocale::Germany, QLocale::Austria, QLocale::Switzerland } },
        { QLocale::French, { QLocale::France, QLocale::Canada } },
        { QLocale::Spanish, { QLocale::Spain, QLocale::Mexico } },
    };

    const QList<QLocale> all =
        QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);

    QList<pair<QString, QString>> result = { {} };
    result.reserve(enchantum::values<QLocale::Language>.size() + 16);

    for (const QLocale& loc : all) {
        const QLocale::Language lang = loc.language();

        if (lang == QLocale::C || lang == QLocale::AnyLanguage) {
            continue;
        }

        // Only these territories of these languages get their own entry; every other locale is its bare language
        bool withTerritory = false;

        if (ranges::contains(TERRITORY_SENSITIVE_LANGUAGES, lang)) {
            const auto& relevant = ranges::find(
                                       LANGUAGE_RELEVANT_TERRITORIES,
                                       lang,
                                       &pair<QLocale::Language, array<QLocale::Territory, 4>>::first
            )
                                       ->second;
            withTerritory = ranges::contains(relevant, loc.territory());
        }

        QString code = withTerritory ? loc.bcp47Name() : QLocale::languageToCode(lang);

        if (ranges::any_of(result, [&code](const auto& entry) -> bool { return entry.second == code; })) {
            continue;
        }

        QString label =
            withTerritory
                ? QString(
                      QLocale::languageToString(lang) % u", "_qsv % QLocale::territoryToString(loc.territory()) %
                      u" ("_qsv % loc.nativeLanguageName() % u')'
                  )
                : QString(QLocale::languageToString(lang) % u" ("_qsv % QLocale(lang).nativeLanguageName() % u')');

        result.emplace_back(std::move(label), std::move(code));
    }

    return result;
}

}  // namespace

SettingsWindow::SettingsWindow(
    Settings* const settings_,
    ProjectSettings* const projectSettings_,
    QWidget* const parent
) :
    QDialog(parent),

    settings(settings_),
    projectSettings(projectSettings_),
    initialSettings(*settings_),

    ui(setupUi()) {
    for (const auto endpoint : enchantum::names<TranslationEndpoint>) {
        ui->typeSelect->addItem(QString::fromLatin1(endpoint));
    }

    ui->translationTableFontSizeWidget->setRange(TRANSLATION_TABLE_FONT_SIZE_MIN, TRANSLATION_TABLE_FONT_SIZE_MAX);

    // Without these, hasAcceptableInput() is unconditionally true and the range hints below are decoration.
    ui->backupPeriodInput->setValidator(new QIntValidator(MIN_BACKUP_PERIOD, MAX_BACKUP_PERIOD, this));
    ui->maxBackupsInput->setValidator(new QIntValidator(1, MAX_BACKUPS, this));
    ui->lineLengthHintInput->setValidator(new QIntValidator(0, MAX_LINE_LENGTH_HINT, this));
    ui->hintFontSizeInput->setValidator(new QIntValidator(0, UINT8_MAX, this));

    for (const QString& style : QStyleFactory::keys()) {
        ui->styleSelect->addItem(style);
    }

    const QList<pair<QString, QString>> languageTags = listLanguageTags();

    for (const auto& [iidx, pair] : views::enumerate(languageTags)) {
        const i32 idx = scast<i32>(iidx);
        ui->sourceLanguageSelect->addItem(pair.first);
        ui->sourceLanguageSelect->setItemData(idx, pair.second);
        ui->translationLanguageSelect->addItem(pair.first);
        ui->translationLanguageSelect->setItemData(idx, pair.second);
        ui->ltMotherTongueSelect->addItem(pair.first);
        ui->ltMotherTongueSelect->setItemData(idx, pair.second);
    }

    QStringList endpointNames;
    endpointNames.reserve(scast<isize>(settings->translation.endpoints.size()));

    for (const auto& endpoint : settings->translation.endpoints) {
        endpointNames.append(endpoint.name);
    }

    auto* const endpointListModel = new QStringListModel(endpointNames, ui->endpointList);
    ui->endpointList->setModel(endpointListModel);

    // The names are set by `retranslateDynamic()`
    auto* const sectionsListModel = new QStringListModel(QStringList(SECTION_COUNT), ui->sectionsList);
    ui->sectionsList->setModel(sectionsListModel);
    retranslateDynamic();

    ui->spellcheckDictionarySelect->installEventFilter(this);
    refreshSpellcheckDictionarySelect();

    buildSequenceTables();

#ifndef ENABLE_UPDATER
    ui->updatesCheckbox->setEnabled(false);
#endif

    syncAll(Direction::Load);

    ui->stackedWidget->setCurrentIndex(0);
    setEndpointFormEnabled(false);

    // The window has no buttons of its own: Save applies the changes, Cancel (also Esc and the close button) drops them
    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto* const contentColumn = new QVBoxLayout();
    layout()->removeWidget(ui->stackedWidget);
    contentColumn->setContentsMargins(0, 0, 0, 0);
    contentColumn->addWidget(ui->stackedWidget, 1);
    contentColumn->addWidget(buttons);
    scast<QBoxLayout*>(layout())->addLayout(contentColumn, 1);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->addEndpointButton, &QPushButton::clicked, this, [this, endpointListModel] -> void {
        const i32 index = endpointListModel->rowCount();
        const QString name = tr("Endpoint %1").arg(qsv(qitos(index + 1)));

        settings->translation.endpoints.emplace_back(
            EndpointSettings{ .id = QUuid::createUuid().toString(QUuid::WithoutBraces), .name = name }
        );

        endpointListModel->insertRow(index);
        endpointListModel->setData(endpointListModel->index(index, 0), name);

        ui->endpointList->setCurrentIndex(endpointListModel->index(index, 0));
    });

    connect(ui->removeEndpointButton, &QPushButton::clicked, this, [this, endpointListModel] -> void {
        const QModelIndex index = ui->endpointList->currentIndex();

        if (!index.isValid()) {
            return;
        }

        const i32 row = index.row();

        const QSignalBlocker blocker(ui->endpointList->selectionModel());

        ui->endpointList->setCurrentIndex(QModelIndex());
        endpointListModel->removeRow(row);

        const auto endpointIt = settings->translation.endpoints.begin() + row;
        removedEndpointIds.emplace_back(std::move(endpointIt->id));
        settings->translation.endpoints.erase(endpointIt);

        setEndpointFormEnabled(false);
    });

    connect(
        endpointListModel,
        &QStringListModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& /* bottomRight */) -> void {
        settings->translation.endpoints[topLeft.row()].name = topLeft.data().toString();
    }
    );

    connect(ui->browseScriptButton, &QPushButton::clicked, this, [this] -> void {
        const QString path = QFileDialog::getOpenFileName(
            this,
            tr("Select Script"),
            ui->scriptPathInput->text(),
            tr("JavaScript (*.js)")
        );

        if (!path.isEmpty()) {
            ui->scriptPathInput->setText(path);
        }
    });

    connect(ui->typeSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
        onTypeChange(TranslationEndpoint(index));
    });

    connect(
        ui->sectionsList->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& /* previous */) -> void {
        ui->stackedWidget->setCurrentIndex(current.row());
    }
    );

    connect(ui->styleSelect, &QComboBox::currentTextChanged, this, [](const QString& style) -> void {
        qApp->setStyle(QStyleFactory::create(style));
    });

    connect(ui->themeSelect, &QComboBox::currentIndexChanged, this, [](const i32 index) -> void {
        qApp->styleHints()->setColorScheme(Qt::ColorScheme(index));
    });

    connect(
        ui->endpointList->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& previous) -> void {
        auto& endpoints = settings->translation.endpoints;
        const auto inRange = [&endpoints](const QModelIndex& index) -> bool {
            return index.isValid() && index.row() < scast<i32>(endpoints.size());
        };

        if (inRange(previous)) {
            syncEndpoint(Direction::Save, endpoints[previous.row()]);
        }

        if (!inRange(current)) {
            return;
        }

        auto& endpointSettings = endpoints[current.row()];
        setEndpointFormEnabled(true);
        syncEndpoint(Direction::Load, endpointSettings);

        // The select signals only when the type differs from the one shown before, and the form depends on it
        onTypeChange(endpointSettings.endpoint);
    }
    );

    connect(ui->checkKeyButton, &QPushButton::clicked, this, [this] -> void { checkKey(); });

    connect(ui->ltCheckConnectionButton, &QPushButton::clicked, this, [this] -> void {
        checkLanguageToolConnection();
    });

    connect(ui->spellcheckDictionarySelect, &QComboBox::currentTextChanged, this, [this](const QString& text) -> void {
        if (text.isEmpty()) {
            return;
        }

        QString dicPath = dataLocation() % u"/dictionaries"_qsv % text;
        dicPath.slice(0, dicPath.size() - 3);
        dicPath += u"dic";

        if (!QFile::exists(dicPath)) {
            present(
                this,
                NOTICE(
                    "`.dic` file corresponding to the `.aff` file does not exist. Dictionary won't work properly without the `.dic` file.",
                    Warning,
                    Modal
                )
            );
            ui->spellcheckDictionarySelect->setCurrentIndex(0);
        }
    });

    connect(ui->addReplacementButton, &QPushButton::clicked, this, [this] -> void {
        const i32 row = ui->replacementsTable->rowCount();
        ui->replacementsTable->insertRow(row);

        auto* const checkboxItem = new QTableWidgetItem();
        checkboxItem->setCheckState(Qt::Unchecked);

        ui->replacementsTable->setItem(row, 0, new QTableWidgetItem());
        ui->replacementsTable->setItem(row, 1, new QTableWidgetItem());
        ui->replacementsTable->setItem(row, REPLACEMENT_ENABLED_COLUMN, checkboxItem);
    });

    connect(ui->removeReplacementButton, &QPushButton::clicked, this, [this] -> void {
        const i32 row = ui->replacementsTable->currentRow();

        if (row != -1) {
            ui->replacementsTable->removeRow(row);
        }
    });

    connect(ui->addLintButton, &QPushButton::clicked, this, [this] -> void { addCustomLintRow(CustomLint()); });

    connect(ui->removeLintButton, &QPushButton::clicked, this, [this] -> void {
        const i32 row = ui->customLintsTable->currentRow();

        if (row != -1) {
            ui->customLintsTable->removeRow(row);
        }
    });

    connect(ui->exportLintsButton, &QPushButton::clicked, this, [this] -> void { exportCustomLints(); });
    connect(ui->importLintsButton, &QPushButton::clicked, this, [this] -> void { importCustomLints(); });

    connect(ui->customLintsTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* const item) -> void {
        if (item->column() != CUSTOM_LINT_COLOR_COLUMN) {
            return;
        }

        const QColor color = QColorDialog::getColor(item->background().color(), this, tr("Choose Color"));

        if (color.isValid()) {
            item->setBackground(color);
        }
    });

    connect(ui->customLintsTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* const item) -> void {
        if (item->column() != CUSTOM_LINT_PATTERN_COLUMN) {
            return;
        }

        const auto test = QRegularExpression(item->text());

        if (!test.isValid()) {
            present(
                this,
                NOTICE(
                    "Regular expression is invalid: %1. Offset: %2",
                    Warning,
                    Modal,
                    test.errorString(),
                    qsv(qitos(test.patternErrorOffset()))
                )
            );
            ui->customLintsTable->editItem(item);
        }
    });
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* const ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
}

// The texts that are set in code, which `retranslateUi` doesn't know (or puts the `.ui` ones back over)
void SettingsWindow::retranslateDynamic() {
    ui->translationTableFontSizeWidget->setLabel(tr("Translation table font size"));

    auto* const sectionsModel = ui->sectionsList->model();
    const QStringList sections = { tr("Core"), tr("Appearance"), tr("Controls"), tr("Translation"), tr("Project") };

    for (i32 idx = 0; idx < sections.size(); idx++) {
        sectionsModel->setData(sectionsModel->index(idx, 0), sections[idx]);
    }

#ifndef ENABLE_UPDATER
    ui->updatesCheckbox->setToolTip(tr("This build of RPGMTranslate has no self-updater."));
#endif

    // Only once an endpoint is selected: the form is meaningless, and its widgets hidden, before that
    if (ui->endpointList->currentIndex().isValid()) {
        onTypeChange(TranslationEndpoint(ui->typeSelect->currentIndex()));
    }
}

void SettingsWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        retranslateDynamic();
    }

    QDialog::changeEvent(event);
};

auto SettingsWindow::eventFilter(QObject* const watched, QEvent* const event) -> bool {
    if (watched == ui->spellcheckDictionarySelect) {
        if (event->type() == QEvent::MouseButtonPress) {
            refreshSpellcheckDictionarySelect();
        } else if (event->type() == QEvent::KeyPress) {
            const auto* const keyEvent = scast<QKeyEvent*>(event);
            const bool popupPressed =
                keyEvent->key() == Qt::Key_F4 ||
                (keyEvent->key() == Qt::Key_Down && keyEvent->modifiers().testFlag(Qt::AltModifier));

            if (popupPressed) {
                refreshSpellcheckDictionarySelect();
            }
        }
    }

    return QDialog::eventFilter(watched, event);
}

void SettingsWindow::refreshSpellcheckDictionarySelect() {
    if (projectSettings == nullptr) {
        return;
    }

    const QString selected = ui->spellcheckDictionarySelect->currentText();

    const QSignalBlocker blocker(ui->spellcheckDictionarySelect);

    ui->spellcheckDictionarySelect->clear();
    ui->spellcheckDictionarySelect->addItem(QString());
    ui->spellcheckDictionarySelect->addItems(listSpellcheckDictionaries());
    ui->spellcheckDictionarySelect->setCurrentIndex(max(0, ui->spellcheckDictionarySelect->findText(selected)));
}

void SettingsWindow::setEndpointFormEnabled(const bool enabled) {
    // Nothing selected means nothing to save the form into
    const array<QWidget*, 4> widgets = { ui->typeSelect,
                                         ui->apiKeyWidget,
                                         ui->yandexFolderWidget,
                                         ui->scriptPathWidget };

    for (QWidget* const widget : widgets) {
        widget->setEnabled(enabled);
    }

    ui->glossaryCheckbox->setEnabled(enabled);
    ui->checkKeyButton->setEnabled(enabled);
}

void SettingsWindow::done(const i32 result) {
    if (result != Accepted) {
        // Style and theme were applied live
        *settings = std::move(initialSettings);

        if (!settings->appearance.style.isEmpty()) {
            qApp->setStyle(QStyleFactory::create(settings->appearance.style));
        }

        qApp->styleHints()->setColorScheme(Qt::ColorScheme(settings->appearance.theme));

        QDialog::done(result);
        return;
    }

    if (!validate()) {
        return;
    }

    syncAll(Direction::Save);

    for (const QString& endpointId : removedEndpointIds) {
        if (!rpgm_delete_credential(strtoffi(endpointId.toUtf8()))) {
            qWarning().noquote() << u"Failed to delete the API key of a removed endpoint: %1"_qsv.arg(
                svtostr(ffitostr(rpgm_error()))
            );
        }
    }

    for (const auto& endpoint : settings->translation.endpoints) {
        if (endpoint.endpoint != TranslationEndpoint::Script || endpoint.scriptPath.isEmpty()) {
            continue;
        }

        auto file = QFile(endpoint.scriptPath);

        if (!file.open(QFile::ReadOnly)) {
            present(this, NOTICE("Cannot read script %1: %2", Warning, Modal, endpoint.scriptPath, file.errorString()));
            continue;
        }

        // Declining just leaves the script unapproved; translating with it then reports that.
        confirmScriptAccess(this, settings->core.approvedScripts, endpoint.name, QString::fromUtf8(file.readAll()));
    }

    QDialog::done(result);
}

auto SettingsWindow::validate() -> bool {
    if (!ui->backupPeriodInput->hasAcceptableInput()) {
        present(
            this,
            NOTICE(
                "Backup period must be between %1 and %2 seconds.",
                Warning,
                Modal,
                qsv(qitos(MIN_BACKUP_PERIOD)),
                qsv(qitos(MAX_BACKUP_PERIOD))
            )
        );
        ui->backupPeriodInput->setFocus();
        return false;
    }

    if (!ui->maxBackupsInput->hasAcceptableInput()) {
        present(this, NOTICE("Max backups must be between 1 and %1.", Warning, Modal, qsv(qitos(MAX_BACKUPS))));
        ui->maxBackupsInput->setFocus();
        return false;
    }

    if (projectSettings != nullptr && !ui->lineLengthHintInput->hasAcceptableInput()) {
        present(
            this,
            NOTICE("Line length hint must be between 0 and %1.", Warning, Modal, qsv(qitos(MAX_LINE_LENGTH_HINT)))
        );
        ui->lineLengthHintInput->setFocus();
        return false;
    }

    if (projectSettings != nullptr && !ui->hintFontSizeInput->hasAcceptableInput()) {
        present(this, NOTICE("Hint font size must be between 0 and %1.", Warning, Modal, qsv(qitos(UINT8_MAX))));
        ui->hintFontSizeInput->setFocus();
        return false;
    }

    // All custom lints are joined into one regex, so a single invalid pattern would silence every one of them
    for (i32 row = 0; row < ui->customLintsTable->rowCount(); row++) {
        const QString pattern = ui->customLintsTable->item(row, CUSTOM_LINT_PATTERN_COLUMN)->text();
        const auto regexp = QRegularExpression(pattern);

        if (!regexp.isValid()) {
            present(
                this,
                NOTICE(
                    "Custom lint \"%1\" is not a valid regular expression: %2",
                    Warning,
                    Modal,
                    pattern,
                    regexp.errorString()
                )
            );
            ui->customLintsTable->setCurrentCell(row, CUSTOM_LINT_PATTERN_COLUMN);
            ui->customLintsTable->setFocus();
            return false;
        }
    }

    // Startup removes a Script endpoint with no path, together with its stored key
    const QModelIndex shownEndpoint = ui->endpointList->currentIndex();

    for (const auto [idx, endpoint] : views::enumerate(settings->translation.endpoints)) {
        // The selected endpoint is still in the form, not yet in `settings`
        const bool isShown = shownEndpoint.isValid() && shownEndpoint.row() == scast<i32>(idx);
        const bool isScript = isShown ? ui->typeSelect->currentIndex() == scast<i32>(TranslationEndpoint::Script)
                                      : endpoint.endpoint == TranslationEndpoint::Script;
        const bool hasPath = isShown ? !ui->scriptPathInput->text().isEmpty() : !endpoint.scriptPath.isEmpty();

        if (isScript && !hasPath) {
            present(this, NOTICE("The Script endpoint \"%1\" needs a script file.", Warning, Modal, endpoint.name));
            ui->endpointList->setCurrentIndex(ui->endpointList->model()->index(scast<i32>(idx), 0));
            ui->scriptPathInput->setFocus();
            return false;
        }
    }

    // The names are shown to the user, so they are translated
    const array<pair<QString, QKeySequenceEdit*>, 11> hotkeyInputs{
        { { tr("Search Panel"), ui->searchPanelInput },
          { tr("Tab Panel"), ui->tabPanelInput },
          { tr("Go To Row"), ui->goToRowInput },
          { tr("Batch Menu"), ui->batchMenuInput },
          { tr("Bookmark Menu"), ui->bookmarkMenuInput },
          { tr("Lint Menu"), ui->lintMenuInput },
          { tr("Glossary Menu"), ui->glossaryMenuInput },
          { tr("Translations Menu"), ui->translationsMenuInput },
          { tr("Trim Cell"), ui->singleTrimInput },
          { tr("Wrap Cell"), ui->singleWrapInput },
          { tr("Latinize Cell"), ui->singleLatinizeInput } }
    };

    const auto reservedHotkeys =
        array<pair<QString, QKeySequence>, 9>{ { { tr("Save"), QKeySequence(QKeySequence::Save) },
                                                 { tr("Write"), QKeySequence(WRITE_SHORTCUT.toString()) },
                                                 { tr("Search"), QKeySequence(QKeySequence::Find) },
                                                 { tr("Close Tab"), QKeySequence(CLOSE_TAB_SHORTCUT.toString()) },
                                                 { tr("Copy"), QKeySequence(QKeySequence::Copy) },
                                                 { tr("Cut"), QKeySequence(QKeySequence::Cut) },
                                                 { tr("Paste"), QKeySequence(QKeySequence::Paste) },
                                                 { tr("Undo"), QKeySequence(QKeySequence::Undo) },
                                                 { tr("Redo"), QKeySequence(QKeySequence::Redo) } } };

    for (const auto [idx, first] : views::enumerate(hotkeyInputs)) {
        const QKeySequence sequence = first.second->keySequence();

        if (sequence.isEmpty()) {
            continue;
        }

        const auto reserved = ranges::find(reservedHotkeys, sequence, &pair<QString, QKeySequence>::second);

        if (reserved != reservedHotkeys.end()) {
            present(
                this,
                NOTICE(
                    "Hotkey %1 of \"%2\" is reserved for \"%3\". Please assign a different hotkey.",
                    Warning,
                    Modal,
                    sequence.toString(),
                    first.first,
                    reserved->first
                )
            );
            first.second->setFocus();
            return false;
        }

        for (const auto& second : views::drop(hotkeyInputs, idx + 1)) {
            if (second.second->keySequence() == sequence) {
                present(
                    this,
                    NOTICE(
                        "Hotkey %1 is already assigned to both \"%2\" and \"%3\". Please assign a different hotkey.",
                        Warning,
                        Modal,
                        sequence.toString(),
                        first.first,
                        second.first
                    )
                );
                first.second->setFocus();
                return false;
            }
        }
    }

    return true;
}

void SettingsWindow::syncAll(const Direction direction) {
    auto& core = settings->core;
    auto& appearance = settings->appearance;
    auto& controls = settings->controls;
    auto& translation = settings->translation;

    if (direction == Direction::Load) {
        core.backup.period = scast<u16>(clamp<i32>(core.backup.period, MIN_BACKUP_PERIOD, MAX_BACKUP_PERIOD));
        core.backup.max = scast<u8>(clamp<i32>(core.backup.max, 1, MAX_BACKUPS));
    }

    // Core
    sync(direction, ui->backupCheckbox, core.backup.enabled);
    sync(direction, ui->backupPeriodInput, core.backup.period);
    sync(direction, ui->maxBackupsInput, core.backup.max);
    sync(direction, ui->updatesCheckbox, core.checkForAppUpdates);
    sync(direction, ui->sourceUpdatesCheckbox, core.checkForSourceUpdates);
    sync(direction, ui->preferGitConfigCheckbox, core.git.preferGitConfig);
    sync(direction, ui->gitUsernameInput, core.git.username);
    sync(direction, ui->gitUserEmailInput, core.git.useremail);

    // Appearance
    if (direction == Direction::Load) {
        if (appearance.translationTableFont.isEmpty()) {
            appearance.translationTableFont = qApp->font().family();
        }

        // `pointSize()` is -1 for a font given in pixels, `QFontInfo` always has a point size
        if (appearance.translationTableFontSize == 0) {
            appearance.translationTableFontSize = scast<u8>(QFontInfo(qApp->font()).pointSize());
        }

        // With no style set the application runs in its default one. The select would show its first, and saving
        // would make that one the style.
        if (appearance.style.isEmpty()) {
            const i32 styleIndex = ui->styleSelect->findText(qApp->style()->name(), Qt::MatchFixedString);

            if (styleIndex != -1) {
                appearance.style = ui->styleSelect->itemText(styleIndex);
            }
        }
    }

    sync(direction, ui->fontSelect, appearance.translationTableFont);
    sync(direction, ui->translationTableFontSizeWidget, appearance.translationTableFontSize);
    sync(direction, ui->styleSelect, appearance.style);
    sync(direction, ui->themeSelect, appearance.theme);
    sync(direction, ui->displayPercentsCheckbox, appearance.displayPercents);
    sync(direction, ui->displayEntryNamesCheckbox, appearance.displayEntryNames);

    // Controls
    sync(direction, ui->searchPanelInput, controls.searchPanel);
    sync(direction, ui->tabPanelInput, controls.tabPanel);
    sync(direction, ui->goToRowInput, controls.goToRow);
    sync(direction, ui->batchMenuInput, controls.batchMenu);
    sync(direction, ui->bookmarkMenuInput, controls.bookmarkMenu);
    sync(direction, ui->lintMenuInput, controls.lintMenu);
    sync(direction, ui->glossaryMenuInput, controls.glossaryMenu);
    sync(direction, ui->translationsMenuInput, controls.translationsMenu);
    sync(direction, ui->singleTrimInput, controls.singleTrim);
    sync(direction, ui->singleWrapInput, controls.singleWrap);
    sync(direction, ui->singleLatinizeInput, controls.singleLatinize);

    // Translation
    sync(direction, ui->displayWordsCheckbox, translation.displayWordsAndCharacters);
    sync(direction, ui->whitespaceCharactersInput, translation.whitespaceCharacters);

    auto& languageTool = translation.languageTool;
    sync(direction, ui->ltBaseUrlInput, languageTool.baseURL);
    sync(direction, ui->ltApiKeyInput, languageTool.apiKey);
    sync(direction, ui->ltUsernameInput, languageTool.username);
    sync(direction, ui->ltMotherTongueSelect, languageTool.motherTongue, true);
    sync(direction, ui->ltPreferredVariantsInput, languageTool.preferredVariants);
    sync(direction, ui->ltDictsInput, languageTool.dicts);
    sync(direction, ui->ltEnabledRulesInput, languageTool.enabledRules);
    sync(direction, ui->ltDisabledRulesInput, languageTool.disabledRules);
    sync(direction, ui->ltEnabledCategoriesInput, languageTool.enabledCategories);
    sync(direction, ui->ltDisabledCategoriesInput, languageTool.disabledCategories);
    sync(direction, ui->ltPickyCheckbox, languageTool.picky);
    sync(direction, ui->ltEnableOnlyCheckbox, languageTool.enabledOnly);

    syncSequenceTable(direction, ui->textCodesTable, translation.textCodes);
    syncSequenceTable(direction, ui->noteTagsTable, translation.notetags);
    syncSequenceTable(direction, ui->pluginCommandsTable, translation.pluginCommands);
    syncSequenceTable(direction, ui->commentTagsTable, translation.commentTags);

    syncCustomLints(direction);
    syncReplacements(direction);

    if (direction == Direction::Save) {
        const QModelIndex endpointIndex = ui->endpointList->currentIndex();

        if (endpointIndex.isValid() && endpointIndex.row() < scast<i32>(translation.endpoints.size())) {
            syncEndpoint(direction, translation.endpoints[endpointIndex.row()]);
        }
    }

    // Project
    if (projectSettings == nullptr) {
        return;
    }

    sync(direction, ui->lineLengthHintInput, projectSettings->lineLengthHint);
    // Empty means "the editor font", which the select can't show. Saving what it shows by default would turn it
    // into that family, so an untouched select leaves the setting empty.
    if (direction == Direction::Load) {
        sync(direction, ui->hintFontSelect, projectSettings->hintFont);
        defaultHintFont = projectSettings->hintFont.isEmpty() ? ui->hintFontSelect->currentFont().family() : QString();
    } else if (!projectSettings->hintFont.isEmpty() || ui->hintFontSelect->currentFont().family() != defaultHintFont) {
        sync(direction, ui->hintFontSelect, projectSettings->hintFont);
    }

    sync(direction, ui->hintFontSizeInput, projectSettings->hintFontSize);

    sync(direction, ui->sourceLanguageSelect, projectSettings->sourceLang, true);
    sync(direction, ui->translationLanguageSelect, projectSettings->translationLang, true);

    sync(direction, ui->spellcheckDictionarySelect, projectSettings->spellcheckDictionary);
    sync(direction, ui->writeEncodingSelect, projectSettings->writeEncoding);

    // A stored "" (older versions' "always UTF-8") matches no item and would keep the previous project's choice
    if (direction == Direction::Load && ui->writeEncodingSelect->findText(projectSettings->writeEncoding) == -1) {
        ui->writeEncodingSelect->setCurrentIndex(0);
    }
}

void SettingsWindow::syncEndpoint(const Direction direction, EndpointSettings& endpoint) {
    sync(direction, ui->typeSelect, endpoint.endpoint);

    sync(direction, ui->apiKeyInput, endpoint.apiKey);
    sync(direction, ui->yandexFolderIdInput, endpoint.yandexFolderId);
    sync(direction, ui->scriptPathInput, endpoint.scriptPath);

    sync(direction, ui->glossaryCheckbox, endpoint.useGlossary);
}

void SettingsWindow::buildSequenceTables() {
    const array<pair<span<const QStringView>, QTableWidget*>, 4> tables = {
        pair<span<const QStringView>, QTableWidget*>{ TextCodes::PATTERN_PARTS, ui->textCodesTable },
        { Notetags::PATTERN_PARTS, ui->noteTagsTable },
        { PluginCommands::PATTERN_PARTS, ui->pluginCommandsTable },
        { CommentTags::PATTERN_PARTS, ui->commentTagsTable },
    };

    for (const auto& [tableIdx, entry] : views::enumerate(tables)) {
        const auto& [patterns, table] = entry;

        table->setRowCount(scast<i32>(patterns.size()));

        for (const auto& [idx, pattern] : views::enumerate(patterns)) {
            const i32 row = scast<i32>(idx);

            QStringView category;

            switch (tableIdx) {
                case 0:
                    category = TextCodes::category(scast<u32>(idx));
                    break;
                case 1:
                    category = Notetags::category(scast<u32>(idx));
                    break;
                case 2:
                    category = PluginCommands::category(scast<u32>(idx));
                    break;
                case 3:
                    category = CommentTags::category(scast<u32>(idx));
                    break;
                default:
                    std::unreachable();
            }

            table->setItem(row, 0, new QTableWidgetItem(pattern.toString()));
            table->setItem(row, 1, new QTableWidgetItem(category.toString()));
            table->setItem(row, SEQUENCE_USE_COLOR_COLUMN, new QTableWidgetItem());
            table->setItem(row, SEQUENCE_COLOR_COLUMN, new QTableWidgetItem());
        }

        connect(table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* const item) -> void {
            if (item->column() != SEQUENCE_COLOR_COLUMN) {
                return;
            }

            const QColor color = QColorDialog::getColor(item->background().color(), this, tr("Choose Color"));

            if (color.isValid()) {
                item->setBackground(color);
            }
        });
    }
}

void SettingsWindow::syncSequenceTable(
    const Direction direction,
    QTableWidget* const table,
    const span<SequenceSettings> sequences
) {
    for (const auto& [idx, sequence] : views::enumerate(sequences)) {
        const i32 row = scast<i32>(idx);

        syncColorCells(
            direction,
            table->item(row, SEQUENCE_USE_COLOR_COLUMN),
            table->item(row, SEQUENCE_COLOR_COLUMN),
            sequence.useCustomColor,
            sequence.customColor
        );
    }
}

void SettingsWindow::addCustomLintRow(const CustomLint& lint) {
    const i32 row = ui->customLintsTable->rowCount();
    ui->customLintsTable->insertRow(row);

    auto* const caseItem = new QTableWidgetItem();
    caseItem->setCheckState(lint.caseInsensitive ? Qt::Checked : Qt::Unchecked);

    auto* const useColorItem = new QTableWidgetItem();
    useColorItem->setCheckState(lint.useCustomColor ? Qt::Checked : Qt::Unchecked);

    auto* const colorItem = new QTableWidgetItem();
    colorItem->setBackground(QBrush(QColor::fromRgba(lint.customColor)));
    colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable);

    ui->customLintsTable->setItem(row, 0, new QTableWidgetItem(lint.sequence));
    ui->customLintsTable->setItem(row, 1, new QTableWidgetItem(lint.tooltip));
    ui->customLintsTable->setItem(row, CUSTOM_LINT_CASE_COLUMN, caseItem);
    ui->customLintsTable->setItem(row, CUSTOM_LINT_USE_COLOR_COLUMN, useColorItem);
    ui->customLintsTable->setItem(row, CUSTOM_LINT_COLOR_COLUMN, colorItem);
}

void SettingsWindow::syncCustomLints(const Direction direction) {
    if (direction == Direction::Load) {
        ui->customLintsTable->setRowCount(0);

        for (const auto& lint : settings->translation.customLints) {
            addCustomLintRow(lint);
        }

        return;
    }

    vector<CustomLint> lints;
    lints.reserve(scast<usize>(ui->customLintsTable->rowCount()));

    for (i32 row = 0; row < ui->customLintsTable->rowCount(); row++) {
        const QString sequence = ui->customLintsTable->item(row, 0)->text();

        if (sequence.isEmpty()) {
            continue;
        }

        CustomLint lint{
            .sequence = sequence,
            .tooltip = ui->customLintsTable->item(row, 1)->text(),
            .caseInsensitive = ui->customLintsTable->item(row, CUSTOM_LINT_CASE_COLUMN)->checkState() == Qt::Checked,
        };

        syncColorCells(
            direction,
            ui->customLintsTable->item(row, CUSTOM_LINT_USE_COLOR_COLUMN),
            ui->customLintsTable->item(row, CUSTOM_LINT_COLOR_COLUMN),
            lint.useCustomColor,
            lint.customColor
        );

        lints.emplace_back(std::move(lint));
    }

    // `disabled` lives in LintSelectMenu, not here, so carry the old flags across by pattern.
    for (auto& lint : lints) {
        const auto previous =
            ranges::find_if(settings->translation.customLints, [&lint](const CustomLint& old) -> bool {
            return old.sequence == lint.sequence;
        });

        if (previous != settings->translation.customLints.end()) {
            lint.disabled = previous->disabled;
        }
    }

    settings->translation.customLints = std::move(lints);
}

void SettingsWindow::syncReplacements(const Direction direction) {
    if (direction == Direction::Load) {
        ui->replacementsTable->setRowCount(scast<i32>(settings->translation.replacements.size()));

        for (const auto& [idx, entry] : views::enumerate(settings->translation.replacements)) {
            const auto& [source, replacement, enabled] = entry;
            const i32 row = scast<i32>(idx);

            auto* const checkboxItem = new QTableWidgetItem();
            checkboxItem->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);

            ui->replacementsTable->setItem(row, 0, new QTableWidgetItem(source));
            ui->replacementsTable->setItem(row, 1, new QTableWidgetItem(replacement));
            ui->replacementsTable->setItem(row, REPLACEMENT_ENABLED_COLUMN, checkboxItem);
        }

        return;
    }

    vector<Replacement> replacements;
    replacements.reserve(scast<usize>(ui->replacementsTable->rowCount()));

    for (i32 row = 0; row < ui->replacementsTable->rowCount(); row++) {
        QString source = ui->replacementsTable->item(row, 0)->text();

        if (source.isEmpty()) {
            continue;
        }

        replacements.emplace_back(
            std::move(source),
            ui->replacementsTable->item(row, 1)->text(),
            ui->replacementsTable->item(row, REPLACEMENT_ENABLED_COLUMN)->checkState() == Qt::Checked
        );
    }

    settings->translation.replacements = std::move(replacements);
}

void SettingsWindow::exportCustomLints() {
    const QString path =
        QFileDialog::getSaveFileName(this, tr("Export custom lints"), QString(), tr("JSON files (*.json)"));

    if (path.isEmpty()) {
        return;
    }

    syncCustomLints(Direction::Save);

    const auto json = glz::write_json(settings->translation.customLints);

    if (!json) {
        present(
            this,
            NOTICE(
                "Serializing custom lints failed: %1",
                Warning,
                Modal,
                QString::fromStdString(glz::format_error(json.error()))
            )
        );
        return;
    }

    // The target may be an existing file the user chose to overwrite; keep it intact unless the write succeeds
    auto file = QSaveFile(path);
    file.setDirectWriteFallback(true);

    const i64 size = scast<i64>(json.value().size());

    if (!file.open(QFile::WriteOnly) || file.write(json.value().data(), size) != size || !file.commit()) {
        present(this, NOTICE("Writing %1 failed: %2", Warning, Modal, path, file.errorString()));
    }
}

void SettingsWindow::importCustomLints() {
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Import custom lints"), QString(), tr("JSON files (*.json)"));

    if (path.isEmpty()) {
        return;
    }

    auto file = QFile(path);

    if (!file.open(QFile::ReadOnly)) {
        present(this, NOTICE("Reading %1 failed: %2", Warning, Modal, path, file.errorString()));
        return;
    }

    const QByteArray json = file.readAll();
    const auto lints = readLenient<vector<CustomLint>>(string_view(json.data(), json.size()));

    if (!lints) {
        present(
            this,
            NOTICE(
                "Parsing %1 failed: %2",
                Warning,
                Modal,
                path,
                QString::fromStdString(glz::format_error(lints.error(), string_view(json.data(), json.size())))
            )
        );
        return;
    }

    usize skipped = 0;

    for (const auto& lint : lints.value()) {
        if (QRegularExpression(lint.sequence).isValid()) {
            addCustomLintRow(lint);
        } else {
            skipped++;
        }
    }

    if (skipped != 0) {
        present(
            this,
            NOTICE(
                "%1 imported custom lint(s) were skipped: their patterns are not valid regular expressions.",
                Warning,
                Modal,
                qsv(qitos(skipped))
            )
        );
    }
}

void SettingsWindow::checkKey() {
    EndpointSettings endpoint;
    syncEndpoint(Direction::Save, endpoint);

    const auto endpointJson = glz::write_json(endpoint);
    const QByteArray sourceLang = QByteArrayLiteral("en");
    const QByteArray translationLang = QByteArrayLiteral("de");
    const QByteArray text = QByteArrayLiteral("Hello");

    ui->checkKeyButton->setEnabled(false);

    QtConcurrent::run([=] -> ::Result<void, QString> {
        FFIString out;

        if (!rpgm_translate_single(
                strtoffi(endpointJson.value().data()),
                strtoffi(sourceLang),
                strtoffi(translationLang),
                strtoffi(text),
                &out
            )) {
            return Err(ffitostr(rpgm_error()).toString());
        }

        rpgm_string_free(out);
        return {};
    }).then(this, [this](const ::Result<void, QString>& checked) -> void {
        ui->checkKeyButton->setEnabled(true);

        if (checked) {
            present(this, NOTICE("The API key works.", Info, Status));
        } else {
            present(this, NOTICE("API key check failed: %1", Warning, Modal, checked.error()));
        }
    });
}

void SettingsWindow::checkLanguageToolConnection() {
    QString baseUrl = ui->ltBaseUrlInput->text();

    while (baseUrl.endsWith(u'/')) {
        baseUrl.chop(1);
    }

    auto request = QNetworkRequest(QUrl(baseUrl + u"/v2"_qsv));
    request.setTransferTimeout(LANGUAGETOOL_CONNECTION_TIMEOUT_MS);

    // One connection per reply: a check started while another is running must not take the other's `finished`
    QNetworkReply* const pending = languageToolNetworkManager.get(request);

    connect(pending, &QNetworkReply::finished, this, [this, reply = pending] -> void {
        if (reply->error() == QNetworkReply::NoError) {
            present(this, NOTICE("Successfully connected to the LanguageTool server.", Info, Status));
        } else {
            const QString error = reply->errorString();

            const bool isTimeout = reply->error() == QNetworkReply::TimeoutError ||
                                   reply->error() == QNetworkReply::OperationCanceledError;

            if (isTimeout && reply->url().scheme() == u"https"_s) {
                present(
                    this,
                    NOTICE(
                        "LanguageTool connection check failed with error: %1\nIf the server doesn't actually support HTTPS (e.g. local server, started with org.languagetool.server.HTTPServer), try an http:// URL instead.",
                        Warning,
                        Modal,
                        error
                    )
                );
            } else {
                present(this, NOTICE("LanguageTool connection check failed with error: %1", Warning, Modal, error));
            }
        }

        reply->deleteLater();
    });
}

void SettingsWindow::onTypeChange(const TranslationEndpoint endpoint) {
    const bool usesKey = endpoint == TranslationEndpoint::Yandex || endpoint == TranslationEndpoint::DeepL;
    const bool isScript = endpoint == TranslationEndpoint::Script;

    ui->apiKeyWidget->setVisible(usesKey);
    ui->checkKeyButton->setVisible(usesKey);
    ui->yandexFolderWidget->setVisible(endpoint == TranslationEndpoint::Yandex);
    ui->scriptPathWidget->setVisible(isScript);
    ui->glossaryCheckbox->setVisible(isScript);

    switch (endpoint) {
        case TranslationEndpoint::Google:
            ui->typeDescriptionLabel->setText(tr("Google Translate. Free and unlimited."));
            break;
        case TranslationEndpoint::Yandex:
            ui->typeDescriptionLabel->setText(tr("Yandex Translate. Requires API key and folder ID."));
            break;
        case TranslationEndpoint::DeepL:
            ui->typeDescriptionLabel->setText(tr("DeepL. Requires API key."));
            break;
        case TranslationEndpoint::Script:
            ui->typeDescriptionLabel->setText(tr(
                "Script. Runs the rpgmTranslateCallback function of your JavaScript file, so any translation service or AI model can be connected."
            ));
            break;
        default:
            std::unreachable();
    }

    QString unavailableBackend;

    switch (endpoint) {
        case TranslationEndpoint::Google:
#ifndef ENABLE_GOOGLE_TRANSLATE
            unavailableBackend = u"Google Translate"_s;
#endif
            break;
        case TranslationEndpoint::Yandex:
#ifndef ENABLE_YANDEX_TRANSLATE
            unavailableBackend = u"Yandex Translate"_s;
#endif
            break;
        case TranslationEndpoint::DeepL:
#ifndef ENABLE_DEEPL
            unavailableBackend = u"DeepL"_s;
#endif
            break;
        case TranslationEndpoint::Script:
            break;
        default:
            std::unreachable();
    }

    if (!unavailableBackend.isEmpty()) {
        ui->typeDescriptionLabel->setText(
            ui->typeDescriptionLabel->text() % u"\n\n"_qsv %
            tr("Warning: %1 support was not compiled into this build. Selecting this endpoint will fail when used.")
                .arg(unavailableBackend)
        );
    }
}
