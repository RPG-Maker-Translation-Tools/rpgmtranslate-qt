#include "SettingsWindow.hpp"

#include "CBSLIWidget.hpp"
#include "Notice.hpp"
#include "PluginSequences.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_SettingsWindow.h"

#include <QColorDialog>
#include <QDirListing>
#include <QFileDialog>
#include <QFontComboBox>
#include <QIntValidator>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringListModel>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTableWidgetItem>
#include <QUuid>

namespace {
using Dir = SettingsWindow::Direction;

constexpr f32 SAMPLING_PARAM_MAX = 2.0F;
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

// itos is decimal-only by design, and hex is what "U+XXXX" means to anyone reading the field.
[[nodiscard]] auto itox(u32 value) -> QString {
    static constexpr const char16_t* DIGITS = u"0123456789ABCDEF";
    static constexpr u8 MIN_DIGITS = 4;

    array<char16_t, 8> buf{};
    u8 len = 0;

    do {
        buf[buf.size() - ++len] = DIGITS[value & 0xF];
        value >>= 4;
    } while (value != 0);

    while (len < MIN_DIGITS) {
        buf[buf.size() - ++len] = u'0';
    }

    return QString(QStringView(buf.data() + buf.size() - len, len));
}

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
        value = scast<T>(stoa<Wide>(edit->text()));
    } else {
        edit->setText(QString(itos(value).qsv()));
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
            box->setItemData(box->currentIndex(), value);
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

void sync(const Dir dir, CBSLIWidget* const widget, optional<f32>& value) {
    if (dir == Dir::Save) {
        value = widget->isChecked() ? optional(widget->value<f32>()) : nullopt;
    } else if (value) {
        widget->setChecked(true);
        widget->setValue(value.value());
    } else {
        widget->setChecked(false);
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

            bool parsed = false;
            const u32 codePoint = stoa<u32>(trimmed, 16, &parsed);

            if (parsed) {
                value.emplace_back(codePoint);
            }
        }
    } else {
        QStringList parts;
        parts.reserve(scast<isize>(value.size()));

        for (const u32 codePoint : value) {
            parts.append(u"U+%1"_qsv.arg(itox(codePoint)));
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
    const auto dictionariesPath = qApp->property("data-location").toString() + u"/dictionaries";

    if (!QFile::exists(dictionariesPath)) {
        return {};
    }

    const auto listing =
        QDirListing(dictionariesPath, QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::FilesOnly);

    QStringList dictionaries;

    for (const auto& entry : listing) {
        QString filename = entry.filePath();

        if (!filename.endsWith(u"aff")) {
            continue;
        }

        filename.slice(dictionariesPath.size(), filename.size() - dictionariesPath.size());
        dictionaries.append(std::move(filename));
    }

    dictionaries.sort(Qt::CaseInsensitive);
    return dictionaries;
}

[[nodiscard]] auto listLanguageTags() -> QList<std::pair<QString, QString>> {
    static constexpr array<QLocale::Language, 6> TERRITORY_SENSITIVE_LANGUAGES = {
        QLocale::English, QLocale::Chinese, QLocale::Portuguese, QLocale::German, QLocale::French, QLocale::Spanish,
    };

    static constexpr array<std::pair<QLocale::Language, array<QLocale::Territory, 4>>, 6>
        LANGUAGE_RELEVANT_TERRITORIES = {
            std::pair{ QLocale::English,
                       array{ QLocale::UnitedStates, QLocale::UnitedKingdom, QLocale::Australia, QLocale::Canada } },
            { QLocale::Chinese, { QLocale::China, QLocale::Taiwan, QLocale::HongKong } },
            { QLocale::Portuguese, { QLocale::Portugal, QLocale::Brazil } },
            { QLocale::German, { QLocale::Germany, QLocale::Austria, QLocale::Switzerland } },
            { QLocale::French, { QLocale::France, QLocale::Canada } },
            { QLocale::Spanish, { QLocale::Spain, QLocale::Mexico } },
        };

    const QList<QLocale> all =
        QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);

    QList<std::pair<QString, QString>> result = { {} };
    result.reserve(magic_enum::enum_values<QLocale::Language>().size() + 16);

    for (const QLocale& loc : all) {
        const QLocale::Language lang = loc.language();

        if (lang == QLocale::C || lang == QLocale::AnyLanguage) {
            continue;
        }

        if (ranges::contains(TERRITORY_SENSITIVE_LANGUAGES, lang)) {
            const auto& relevant = ranges::find_if(LANGUAGE_RELEVANT_TERRITORIES, [lang](const auto kv_) -> bool {
                return kv_.first == lang;
            })->second;

            if (ranges::contains(relevant, loc.territory())) {
                std::pair<QString, QString> val = { QString(
                                                        QLocale::languageToString(loc.language()) % u", "_qsv %
                                                        QLocale::territoryToString(loc.territory()) % u" ("_qsv %
                                                        loc.nativeLanguageName() % u')'
                                                    ),
                                                    loc.bcp47Name() };

                if (!ranges::any_of(result, [&](const auto& pair) -> bool { return pair.second == val.second; })) {
                    result.push_back(std::move(val));
                }

            } else {
                std::pair<QString, QString> val = {
                    QString(QLocale::languageToString(lang) % u" ("_qsv % QLocale(lang).nativeLanguageName() % u')'),
                    QLocale::languageToCode(lang)
                };

                if (!ranges::any_of(result, [&](const auto& pair) -> bool { return pair.second == val.second; })) {
                    result.push_back(std::move(val));
                }
            }
        } else {
            std::pair<QString, QString> val = {
                QString(QLocale::languageToString(lang) % u" ("_qsv % QLocale(lang).nativeLanguageName() % u')'),
                QLocale::languageToCode(lang)
            };

            if (!ranges::any_of(result, [&](const auto& pair) -> bool { return pair.second == val.second; })) {
                result.push_back(std::move(val));
            }
        }
    }

    return result;
}

QString DEFAULT_SYSTEM_PROMPT =
    uR"(Role:
You are a professional videogame localization specialist for JRPG/RPG/Visual Novel titles (RPG Maker engine). Produce player-facing translations with high fidelity, cultural awareness, and strict terminology consistency.

Task:
Translate all provided text from sourceLanguage to translationLanguage using all available context.

Input:
You receive a JSON object with:

- sourceLanguage - BCP-47 tag of the source text
- translationLanguage - target BCP-47 tag
- projectContext - global tone, lore, and style guidance
- localContext - situational context for this batch
- glossary - mandatory terminology with notes (always override default choices)
- files - object:
  - keys = filename / asset type (map, system, items, etc.) - indicates gameplay function
  - values = array of lines to process

Line semantics:

- Comments starting with <!> are context only - DO NOT translate.
- `<!>ID` starts a new independent entry.
- `<!>NAME` is an internal identifier.
- `<!>IN-GAME DISPLAY NAME` is the visible map name.
- `<!>EVENT NAME` starts a new event block.

Translation rules:

- Preserve meaning, intent, tone, and gameplay function.
- Prefer concise, idiomatic, UI-safe phrasing.
- Maintain cross-file consistency within the batch.
- Use projectContext and localContext aggressively.
- Enforce glossary terminology exactly.

Non-translatable elements (must remain byte-exact and in place):

- Control codes and escape sequences:
  `\V[n] \N[n] \P[n] \G \C[n] \I[n] \{ \} \\ \$ \. \| \! \> \< \^ \w[x]
 \n<x> \nc<x> \nr<x> <br> \px[x] \py[x] \oc[x] \ow[x] \fr \fs[x] \fn<x>
 \fb \fi \af[x] \ac[x] \an[x] \pf[x] \pc[x] \pn[x]
 \nc[x] \ni[x] \nw[x] \na[x] \ns[x] \nt[x]
 \ii[x] \iw[x] \ia[x] \is[x] \it[x]`
- Printf-style substitutions (`%s`, `%d`, etc.).
- Inline conditional constructs (e.g., `text if(\V[250])`).
- Engine command tags and HTML-like functional elements
  (e.g., `<Picture: icon_jumpsuit>`).
- Any variables, placeholders, markup, or structural tokens.

Do not:

- Translate comments or identifiers.
- Reorder, split, merge, or drop lines.
- Add explanations or metadata.

Output:
Return ONLY a JSON object with the same keys and array structure as the `files` key:

```json
{
  "name": [...]
}
```
)"_s;

QString DEFAULT_SINGLE_TRANSLATE_SYSTEM_PROMPT =
    uR"(Role:
You are a professional videogame localization specialist for JRPG/RPG/Visual Novel titles (RPG Maker engine). Produce player-facing translations with high fidelity, cultural awareness, and strict terminology consistency.

Task:
Translate all provided text from sourceLanguage to translationLanguage using all available context.

Input:
You receive a JSON object containing:

- sourceLanguage - BCP-47 tag of the source text
- translationLanguage - target BCP-47 tag
- projectContext - global tone, lore, and style guidance
- localContext - situational context for this batch
- glossary - mandatory terminology with notes (always override default choices)
- string - the text to translate
- filename - filename / asset type (map, system, items, etc.) - indicates gameplay function

Translation rules:

- Preserve meaning, intent, tone, and gameplay function.
- Prefer concise, idiomatic, UI-safe phrasing.
- Maintain cross-file consistency within the batch.
- Use projectContext and localContext aggressively.
- Enforce glossary terminology exactly.

Non-translatable elements (must remain byte-exact and in place):

- Control codes and escape sequences:
  `\V[n] \N[n] \P[n] \G \C[n] \I[n] \{ \} \\ \$ \. \| \! \> \< \^ \w[x]
\n<x> \nc<x> \nr<x> <br> \px[x] \py[x] \oc[x] \ow[x] \fr \fs[x] \fn<x>
\fb \fi \af[x] \ac[x] \an[x] \pf[x] \pc[x] \pn[x]
\nc[x] \ni[x] \nw[x] \na[x] \ns[x] \nt[x]
\ii[x] \iw[x] \ia[x] \is[x] \it[x]`
- Printf-style substitutions (`%s`, `%d`, etc.).
- Inline conditional constructs (e.g., `text if(\V[250])`).
- Engine command tags and HTML-like functional elements
  (e.g., `<Picture: icon_jumpsuit>`).
- Any variables, placeholders, markup, or structural tokens.

Do not:

- Translate comments or identifiers.
- Reorder, split, merge, or drop lines.
- Add explanations or metadata.

Output:
Return ONLY a JSON string with the translation.
)"_s;
}  // namespace

SettingsWindow::SettingsWindow(
    const shared_ptr<Settings>& settings_,
    const shared_ptr<ProjectSettings>& projectSettings_,
    const QStringList& tabs,
    QWidget* const parent
) :
    QDialog(parent),

    settings(settings_),
    projectSettings(projectSettings_),

    ui(setupUi()) {
    ui->inputTokenLimitWidget->setRange(0, UINT16_MAX);
    ui->inputTokenLimitWidget->setCheckable(false);
    ui->inputTokenLimitWidget->setLabel(tr("Input token limit"));
    ui->outputTokenLimitWidget->setRange(0, UINT16_MAX);
    ui->outputTokenLimitWidget->setCheckable(false);
    ui->outputTokenLimitWidget->setLabel(tr("Output token limit"));
    ui->thinkingBudgetWidget->setRange(0, UINT16_MAX);
    ui->thinkingBudgetWidget->setCheckable(false);
    ui->thinkingBudgetWidget->setLabel(tr("Thinking budget limit"));

    ui->temperatureWidget->setRange(0.0F, SAMPLING_PARAM_MAX);
    ui->temperatureWidget->setLabel(tr("Temperature"));
    ui->frequencyWidget->setRange(0.0F, SAMPLING_PARAM_MAX);
    ui->frequencyWidget->setLabel(tr("Frequency penalty"));
    ui->precenseWidget->setRange(0.0F, SAMPLING_PARAM_MAX);
    ui->precenseWidget->setLabel(tr("Precense penalty"));
    ui->topPWidget->setRange(0.0F, SAMPLING_PARAM_MAX);
    ui->topPWidget->setLabel(tr("Top P"));

    ui->translationTableFontSizeWidget->setRange(TRANSLATION_TABLE_FONT_SIZE_MIN, TRANSLATION_TABLE_FONT_SIZE_MAX);
    ui->translationTableFontSizeWidget->setLabel(tr("Translation table font size"));
    ui->translationTableFontSizeWidget->setCheckable(false);

    // Without these, hasAcceptableInput() is unconditionally true and the range hints below are decoration.
    ui->backupPeriodInput->setValidator(new QIntValidator(MIN_BACKUP_PERIOD, MAX_BACKUP_PERIOD, this));
    ui->maxBackupsInput->setValidator(new QIntValidator(1, MAX_BACKUPS, this));
    ui->lineLengthHintInput->setValidator(new QIntValidator(0, MAX_LINE_LENGTH_HINT, this));

    for (const QString& style : QStyleFactory::keys()) {
        ui->styleSelect->addItem(style);
    }


    const QList<std::pair<QString, QString>> languageTags = listLanguageTags();

    for (const auto& [idx, pair] : views::enumerate(languageTags)) {
        ui->sourceLanguageSelect->addItem(pair.first);
        ui->sourceLanguageSelect->setItemData(idx, pair.second);
        ui->translationLanguageSelect->addItem(pair.first);
        ui->translationLanguageSelect->setItemData(idx, pair.second);
        ui->ltMotherTongueSelect->addItem(pair.first);
        ui->ltMotherTongueSelect->setItemData(idx, pair.second);
    }

    auto* const fileContextListModel = new QStringListModel(tabs, ui->fileContextList);
    ui->fileContextList->setModel(fileContextListModel);

    QStringList endpointNames;
    endpointNames.reserve(scast<isize>(settings->translation.endpoints.size()));

    for (const auto& endpoint : settings->translation.endpoints) {
        endpointNames.append(endpoint.name);
    }

    auto* const endpointListModel = new QStringListModel(endpointNames, ui->endpointList);
    ui->endpointList->setModel(endpointListModel);

    auto* const sectionsListModel = new QStringListModel(
        { tr("Core"), tr("Appearance"), tr("Controls"), tr("Translation"), tr("Project") },
        ui->sectionsList
    );
    ui->sectionsList->setModel(sectionsListModel);

    ui->spellcheckDictionarySelect->installEventFilter(this);
    refreshSpellcheckDictionarySelect();

    buildSequenceTables();

    syncAll(Direction::Load);

    ui->stackedWidget->setCurrentIndex(0);

    connect(ui->addEndpointButton, &QPushButton::pressed, this, [this, endpointListModel] -> void {
        const i32 index = endpointListModel->rowCount();
        const QString name = tr("Endpoint %1").arg(itos(index + 1).qsv());

        settings->translation.endpoints.emplace_back(
            EndpointSettings{ .id = QUuid::createUuid().toString(QUuid::WithoutBraces), .name = name }
        );

        endpointListModel->insertRow(index);
        endpointListModel->setData(endpointListModel->index(index, 0), name);

        ui->endpointList->setCurrentIndex(endpointListModel->index(index, 0));
    });

    connect(ui->removeEndpointButton, &QPushButton::pressed, this, [this, endpointListModel] -> void {
        const QModelIndex index = ui->endpointList->currentIndex();

        if (!index.isValid()) {
            return;
        }

        const i32 row = index.row();

        const QSignalBlocker blocker(ui->endpointList->selectionModel());

        ui->endpointList->setCurrentIndex(QModelIndex());
        endpointListModel->removeRow(row);

        const auto endpointIt = settings->translation.endpoints.begin() + row;
        rpgm_delete_credential(strtoffi(endpointIt->id.toUtf8()));
        settings->translation.endpoints.erase(endpointIt);
    });

    connect(
        endpointListModel,
        &QStringListModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& /* bottomRight */) -> void {
        settings->translation.endpoints[topLeft.row()].name = topLeft.data().toString();
    }
    );

    connect(ui->defaultBaseURLButton, &QPushButton::pressed, this, [this] -> void {
        const auto endpoint = TranslationEndpoint(ui->typeSelect->currentIndex());
        if (endpoint > TranslationEndpoint::DeepL) {
            setDefaultBaseURL(endpoint);
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

        // Removing an endpoint fires this with a `previous` that no longer exists.
        if (inRange(previous)) {
            syncEndpoint(Direction::Save, endpoints[previous.row()]);
        }

        if (!inRange(current)) {
            return;
        }

        auto& endpointSettings = endpoints[current.row()];
        syncEndpoint(Direction::Load, endpointSettings);

        if (endpointSettings.model.isEmpty()) {
            return;
        }

        checkKey();

        const i32 modelIndex = ui->modelSelect->findText(endpointSettings.model);

        if (modelIndex != -1) {
            ui->modelSelect->setCurrentIndex(modelIndex);
        } else if (ui->modelSelect->count() > 0) {
            // An empty list means checkKey() already reported why it couldn't fetch anything.
            present(
                this,
                NOTICE(
                    "Model that was previously selected for translation: %1 is not longer in the list of models provided by the endpoint.",
                    Warning,
                    Modal,
                    endpointSettings.model
                )
            );
        }
    }
    );

    connect(ui->defaultSystemPromptButton, &QPushButton::pressed, this, [this] -> void {
        ui->systemPromptInput->setPlainText(DEFAULT_SYSTEM_PROMPT);
    });

    connect(ui->singleSystemPromptDefaultButton, &QPushButton::pressed, this, [this] -> void {
        ui->singleSystemPromptInput->setPlainText(DEFAULT_SINGLE_TRANSLATE_SYSTEM_PROMPT);
    });

    connect(ui->baseURLInput, &UnfocusLineEdit::editingFinished, this, [this] -> void {
        const QString baseUrl = ui->baseURLInput->text();

        if (baseUrl.isEmpty()) {
            return;
        }

        const QUrl url = QUrl::fromUserInput(baseUrl);

        if (url.scheme().isEmpty() || url.host().isEmpty()) {
            present(this, NOTICE("Given URL is invalid. Please check the validity of submitted URL.", Warning, Modal));

            ui->baseURLInput->clear();
        }
    });

    connect(ui->checkKeyButton, &QPushButton::pressed, this, [this] -> void { checkKey(); });

    connect(ui->ltCheckConnectionButton, &QPushButton::pressed, this, [this] -> void {
        checkLanguageToolConnection();
    });

    connect(ui->spellcheckDictionarySelect, &QComboBox::currentTextChanged, this, [this](const QString& text) -> void {
        if (text.isEmpty()) {
            return;
        }

        QString dicPath = qApp->property("data-location").toString() % u"/dictionaries"_qsv % text;
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

    connect(ui->addReplacementButton, &QPushButton::pressed, this, [this] -> void {
        const i32 row = ui->replacementsTable->rowCount();
        ui->replacementsTable->insertRow(row);

        auto* const checkboxItem = new QTableWidgetItem();
        checkboxItem->setCheckState(Qt::Unchecked);

        ui->replacementsTable->setItem(row, 0, new QTableWidgetItem());
        ui->replacementsTable->setItem(row, 1, new QTableWidgetItem());
        ui->replacementsTable->setItem(row, REPLACEMENT_ENABLED_COLUMN, checkboxItem);
    });

    connect(ui->removeReplacementButton, &QPushButton::pressed, this, [this] -> void {
        const i32 row = ui->replacementsTable->currentRow();

        if (row != -1) {
            ui->replacementsTable->removeRow(row);
        }
    });

    connect(ui->addLintButton, &QPushButton::pressed, this, [this] -> void { addCustomLintRow(CustomLint{}); });

    connect(ui->removeLintButton, &QPushButton::pressed, this, [this] -> void {
        const i32 row = ui->customLintsTable->currentRow();

        if (row != -1) {
            ui->customLintsTable->removeRow(row);
        }
    });

    connect(ui->exportLintsButton, &QPushButton::pressed, this, [this] -> void { exportCustomLints(); });
    connect(ui->importLintsButton, &QPushButton::pressed, this, [this] -> void { importCustomLints(); });

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
                    itos(test.patternErrorOffset()).qsv()
                )
            );
            ui->customLintsTable->editItem(item);
        }
    });

    if (projectSettings != nullptr) {
        connect(
            ui->fileContextList->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            [this](const QModelIndex& current, const QModelIndex& previous) -> void {
            syncFileContext(Direction::Save, previous);
            syncFileContext(Direction::Load, current);
        }
        );
    }
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* const ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
};

void SettingsWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
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

// Overriding done() rather than closeEvent() catches every way out of the dialog - Esc and reject() reach
// done() without ever producing a QCloseEvent.
void SettingsWindow::done(const i32 result) {
    if (!validate()) {
        return;
    }

    syncAll(Direction::Save);
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
                itos(MIN_BACKUP_PERIOD).qsv(),
                itos(MAX_BACKUP_PERIOD).qsv()
            )
        );
        ui->backupPeriodInput->setFocus();
        return false;
    }

    if (!ui->maxBackupsInput->hasAcceptableInput()) {
        present(this, NOTICE("Max backups must be between 1 and %1.", Warning, Modal, itos(MAX_BACKUPS).qsv()));
        ui->maxBackupsInput->setFocus();
        return false;
    }

    if (projectSettings != nullptr && !ui->lineLengthHintInput->hasAcceptableInput()) {
        present(
            this,
            NOTICE("Line length hint must be between 0 and %1.", Warning, Modal, itos(MAX_LINE_LENGTH_HINT).qsv())
        );
        ui->lineLengthHintInput->setFocus();
        return false;
    }

    const array<std::pair<QStringView, QKeySequenceEdit*>, 8> hotkeyInputs{
        { { u"Search Panel"_qsv, ui->searchPanelInput },
         { u"Tab Panel"_qsv, ui->tabPanelInput },
         { u"Go To Row"_qsv, ui->goToRowInput },
         { u"Batch Menu"_qsv, ui->batchMenuInput },
         { u"Bookmark Menu"_qsv, ui->bookmarkMenuInput },
         { u"Lint Menu"_qsv, ui->lintMenuInput },
         { u"Glossary Menu"_qsv, ui->glossaryMenuInput },
         { u"Translations Menu"_qsv, ui->translationsMenuInput } }
    };

    for (const auto [i, first] : views::enumerate(hotkeyInputs)) {
        const QKeySequence sequence = first.second->keySequence();

        if (sequence.isEmpty()) {
            continue;
        }

        for (const auto& second : views::drop(hotkeyInputs, i + 1)) {
            if (second.second->keySequence() == sequence) {
                present(
                    this,
                    NOTICE(
                        "Hotkey %1 is already assigned to both \"%2\" and \"%3\". Please assign a different hotkey.",
                        Warning,
                        Modal,
                        sequence.toString(),
                        first.first.toString(),
                        second.first.toString()
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

        if (appearance.translationTableFontSize == 0) {
            appearance.translationTableFontSize = scast<u8>(qApp->font().pointSize());
        }
    }

    sync(direction, ui->fontSelect, appearance.translationTableFont);
    sync(direction, ui->translationTableFontSizeWidget, appearance.translationTableFontSize);
    sync(direction, ui->styleSelect, appearance.style);
    sync(direction, ui->themeSelect, appearance.theme);
    sync(direction, ui->displayPercentsCheckbox, appearance.displayPercents);

    // Controls
    sync(direction, ui->searchPanelInput, controls.searchPanel);
    sync(direction, ui->tabPanelInput, controls.tabPanel);
    sync(direction, ui->goToRowInput, controls.goToRow);
    sync(direction, ui->batchMenuInput, controls.batchMenu);
    sync(direction, ui->bookmarkMenuInput, controls.bookmarkMenu);
    sync(direction, ui->lintMenuInput, controls.lintMenu);
    sync(direction, ui->glossaryMenuInput, controls.glossaryMenu);
    sync(direction, ui->translationsMenuInput, controls.translationsMenu);

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

    sync(direction, ui->sourceLanguageSelect, projectSettings->sourceLang, true);
    sync(direction, ui->translationLanguageSelect, projectSettings->translationLang, true);

    sync(direction, ui->spellcheckDictionarySelect, projectSettings->spellcheckDictionary);
    sync(direction, ui->writeEncodingSelect, projectSettings->writeEncoding);
    sync(direction, ui->projectContextInput, projectSettings->projectContext);

    if (direction == Direction::Save) {
        syncFileContext(direction, ui->fileContextList->currentIndex());
    }
}

void SettingsWindow::syncEndpoint(const Direction direction, EndpointSettings& endpoint) {
    sync(direction, ui->typeSelect, endpoint.type);

    if (direction == Direction::Save || !endpoint.baseUrl.isEmpty()) {
        sync(direction, ui->baseURLInput, endpoint.baseUrl);
    }

    sync(direction, ui->apiKeyInput, endpoint.apiKey);
    sync(direction, ui->yandexFolderIDInput, endpoint.yandexFolderID);
    sync(direction, ui->systemPromptInput, endpoint.systemPrompt);
    sync(direction, ui->singleSystemPromptInput, endpoint.singleTranslateSystemPrompt);

    sync(direction, ui->temperatureWidget, endpoint.temperature);
    sync(direction, ui->frequencyWidget, endpoint.frequencyPenalty);
    sync(direction, ui->precenseWidget, endpoint.precensePenalty);
    sync(direction, ui->topPWidget, endpoint.topP);

    sync(direction, ui->inputTokenLimitWidget, endpoint.tokenLimit);
    sync(direction, ui->outputTokenLimitWidget, endpoint.outputTokenLimit);
    sync(direction, ui->thinkingBudgetWidget, endpoint.thinkingBudget);

    sync(direction, ui->reasoningEffortSelect, endpoint.reasoningEffort);
    sync(direction, ui->glossaryCheckbox, endpoint.useGlossary);
    sync(direction, ui->thinkingCheckbox, endpoint.thinking);
    sync(direction, ui->singleTranslationCheckbox, endpoint.singleTranslation);

    if (direction == Direction::Save) {
        if (ui->modelSelect->count() > 0) {
            endpoint.model = ui->modelSelect->currentText();
        }
    } else {
        ui->modelSelect->clear();
    }
}

void SettingsWindow::buildSequenceTables() {
    const array<std::pair<span<const QStringView>, QTableWidget*>, 4> tables = {
        std::pair<span<const QStringView>, QTableWidget*>{ TextCodes::PATTERN_PARTS, ui->textCodesTable },
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

void SettingsWindow::syncFileContext(const Direction direction, const QModelIndex& index) {
    if (!index.isValid()) {
        if (direction == Direction::Load) {
            ui->fileContextInput->setPlainText(QString());
        }

        return;
    }

    const QString tab = index.data().toString();

    if (direction == Direction::Save) {
        const QString context = ui->fileContextInput->toPlainText();

        if (context.isEmpty()) {
            projectSettings->fileContexts.erase(tab);
        } else {
            projectSettings->fileContexts.insert({ tab, context });
        }
    } else if (projectSettings->fileContexts.contains(tab)) {
        ui->fileContextInput->setPlainText(projectSettings->fileContexts[tab]);
    } else {
        ui->fileContextInput->setPlainText(QString());
    }
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

    auto file = QFile(path);

    if (!file.open(QFile::WriteOnly)) {
        present(this, NOTICE("Writing %1 failed: %2", Warning, Modal, path, file.errorString()));
        return;
    }

    file.write(json.value().data(), scast<i64>(json.value().size()));
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
    const auto lints = glz::read_json<vector<CustomLint>>(string_view(json.data(), json.size()));

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

    for (const auto& lint : lints.value()) {
        addCustomLintRow(lint);
    }
}

void SettingsWindow::setDefaultBaseURL(const TranslationEndpoint endpoint) {
    switch (endpoint) {
        case TranslationEndpoint::Google:
        case TranslationEndpoint::Yandex:
        case TranslationEndpoint::DeepL:
            std::unreachable();
            break;
        case TranslationEndpoint::Aliyun:
            ui->baseURLInput->setText(u"https://dashscope.aliyuncs.com"_s);
            break;
        case TranslationEndpoint::Anthropic:
            ui->baseURLInput->setText(u"https://api.anthropic.com"_s);
            break;
        case TranslationEndpoint::DeepSeek:
            ui->baseURLInput->setText(u"https://api.deepseek.com/v1"_s);
            break;
        case TranslationEndpoint::Gemini:
            ui->baseURLInput->setText(u"https://generativelanguage.googleapis.com/v1beta"_s);
            break;
        case TranslationEndpoint::Longcat:
            ui->baseURLInput->setText(u"https://api.longcat.chat/openai/v1"_s);
            break;
        case TranslationEndpoint::Moonshot:
            ui->baseURLInput->setText(u"https://api.moonshot.cn/v1"_s);
            break;
        case TranslationEndpoint::Mistral:
            ui->baseURLInput->setText(u"https://api.mistral.ai/v1"_s);
            break;
        case TranslationEndpoint::OpenAI:
            ui->baseURLInput->setText(u"https://api.openai.com/v1"_s);
            break;
        case TranslationEndpoint::Volcengine:
            ui->baseURLInput->setText(u"https://ark.cn-beijing.volces.com/api/v3"_s);
            break;
        case TranslationEndpoint::Xiaomi:
            ui->baseURLInput->setText(u"https://api.xiaomimimo.com/v1"_s);
            break;
        case TranslationEndpoint::Koboldcpp:
        case TranslationEndpoint::Ollama:
        case TranslationEndpoint::OpenAICompatible:
        case TranslationEndpoint::Xinference:
            ui->baseURLInput->clear();
            break;
        case TranslationEndpoint::Zhipu:
            ui->baseURLInput->setText(u"https://open.bigmodel.cn"_s);
            break;
    }
}

void SettingsWindow::checkKey() {
    const auto endpoint = TranslationEndpoint(ui->typeSelect->currentIndex());

    if (endpoint <= TranslationEndpoint::DeepL) {
        // TODO(v1.2): Check
        return;
    }

    ByteBuffer out;

    const QByteArray apiKey = ui->apiKeyInput->text().toUtf8();
    const QByteArray baseUrl = ui->baseURLInput->text().toUtf8();

    const bool success = rpgm_get_models(endpoint, strtoffi(apiKey), strtoffi(baseUrl), &out);

    if (!success) {
        const QUtf8SV error = ffitostr(rpgm_error());
        present(this, NOTICE("Getting available models failed with error: %1", Warning, Modal, svtostr(error)));

        return;
    }

    ui->modelSelect->clear();

    u32 pos = 4;

    while (pos < out.len) {
        const u32 stringLen = *rcast<const u32*>(out.ptr + pos);
        pos += 4;

        ui->modelSelect->addItem(QString::fromUtf8(rcast<const char*>(out.ptr + pos), stringLen));
        pos += stringLen;
    }

    rpgm_buffer_free(out);
}

void SettingsWindow::checkLanguageToolConnection() {
    auto request = QNetworkRequest(QUrl(ui->ltBaseUrlInput->text() + u"/v2"_qsv));
    request.setTransferTimeout(LANGUAGETOOL_CONNECTION_TIMEOUT_MS);

    connect(
        &languageToolNetworkManager,
        &QNetworkAccessManager::finished,
        this,
        [this](QNetworkReply* const reply) -> void {
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
    },
        Qt::SingleShotConnection
    );

    languageToolNetworkManager.get(request);
}

void SettingsWindow::onTypeChange(const TranslationEndpoint endpoint) {
    ui->baseURLInput->setEnabled(true);

    switch (endpoint) {
        case TranslationEndpoint::Google:
            ui->typeDescriptionLabel->setText(
                tr("Google Translate. Free and unlimited. Configured options don't work with this endpoint.")
            );
            break;
        case TranslationEndpoint::Yandex:
            ui->typeDescriptionLabel->setText(tr(
                "Yandex Translate. Requires API key and folder ID. Configured options don't work with this endpoint."
            ));
            break;
        case TranslationEndpoint::DeepL:
            ui->typeDescriptionLabel->setText(tr(
                "DeepL. Requires API key and folder ID. Configured options don't work with this endpoint, except glossary usage."
            ));
            ui->baseURLInput->setEnabled(false);
            break;
        default:
            setDefaultBaseURL(endpoint);

            ui->typeDescriptionLabel->setText(tr(
                "LLM endpoint with pre-defined base URL. Don't change the base URL, unless you know what you're doing. Configured options will affect this endpoint."
            ));

            if (endpoint == TranslationEndpoint::OpenAICompatible) {
                ui->typeDescriptionLabel->setText(tr(
                    "OpenAI-compatible endpoint. This category fits many providers, including OpenAI itself, DeepSeek, Mistral, OpenRouter and local providers, such as llama.cpp and koboldcpp. Requires valid base URL, that should probably end with '/v1'. Configured options will affect this endpoint."
                ));
            } else if (endpoint == TranslationEndpoint::Ollama || endpoint == TranslationEndpoint::Koboldcpp) {
                ui->typeDescriptionLabel->setText(tr(
                    "Local endpoint. You need to set correct base url, that should probably end with '/v1'. Configured options will affect this endpoint."
                ));
            }
            break;
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
        default:
#ifndef ENABLE_LLM_CONNECTOR
            unavailableBackend = tr("LLM connector");
#endif
            break;
    }

    if (!unavailableBackend.isEmpty()) {
        ui->typeDescriptionLabel->setText(
            ui->typeDescriptionLabel->text() % u"\n\n"_qsv %
            tr("Warning: %1 support was not compiled into this build. Selecting this endpoint will fail when used.")
                .arg(unavailableBackend)
        );
    }
}
