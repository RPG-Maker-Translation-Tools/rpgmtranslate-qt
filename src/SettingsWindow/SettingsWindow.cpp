#include "SettingsWindow.hpp"

#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "rpgmtranslate.h"
#include "ui_SettingsWindow.h"

#include <QDirListing>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStringListModel>
#include <QStyleFactory>
#include <QStyleHints>

[[nodiscard]] auto listSpellcheckDictionaries() -> QStringList {
    const auto dictionariesPath =
        qApp->property("data-location").toString() + u"/dictionaries";

    if (!QFile::exists(dictionariesPath)) {
        return {};
    }

    const auto listing = QDirListing(
        dictionariesPath,
        QDirListing::IteratorFlag::Recursive |
            QDirListing::IteratorFlag::FilesOnly
    );

    QStringList dictionaries;

    for (const auto& entry : listing) {
        QString filename = entry.filePath();

        if (!filename.endsWith("aff"_L1)) {
            continue;
        }

        filename.slice(
            dictionariesPath.size(),
            filename.size() - dictionariesPath.size()
        );
        dictionaries.append(std::move(filename));
    }

    dictionaries.sort(Qt::CaseInsensitive);
    return dictionaries;
}

SettingsWindow::SettingsWindow(
    const shared_ptr<Settings>& settings_,
    const shared_ptr<ProjectSettings>& projectSettings_,
    const QStringList& tabs,
    QWidget* const parent
) :
    QDialog(parent),
    ui(setupUi()),
    settings(settings_),
    projectSettings(projectSettings_) {
    ui->inputTokenLimitWidget->setRange(0, UINT16_MAX);
    ui->inputTokenLimitWidget->setCheckable(false);
    ui->inputTokenLimitWidget->setLabel(tr("Input token limit"));
    ui->outputTokenLimitWidget->setRange(0, UINT16_MAX);
    ui->outputTokenLimitWidget->setCheckable(false);
    ui->outputTokenLimitWidget->setLabel(tr("Output token limit"));
    ui->thinkingBudgetWidget->setRange(0, UINT16_MAX);
    ui->thinkingBudgetWidget->setCheckable(false);
    ui->thinkingBudgetWidget->setLabel(tr("Thinking budget limit"));

    ui->temperatureWidget->setRange(0.0F, 2.0F);
    ui->temperatureWidget->setLabel(tr("Temperature"));
    ui->frequencyWidget->setRange(0.0F, 2.0F);
    ui->frequencyWidget->setLabel(tr("Frequency penalty"));
    ui->precenseWidget->setRange(0.0F, 2.0F);
    ui->precenseWidget->setLabel(tr("Precense penalty"));
    ui->topPWidget->setRange(0.0F, 2.0F);
    ui->topPWidget->setLabel(tr("Top P"));

    ui->translationTableFontSizeWidget->setRange(8, 96);
    ui->translationTableFontSizeWidget->setLabel(
        tr("Translation table font size")
    );
    ui->translationTableFontSizeWidget->setCheckable(false);

    for (const auto [value, name] : magic_enum::enum_entries<Algorithm>()) {
        ui->sourceLanguageSelect->addItem(QString::fromLatin1(name));
        ui->translationLanguageSelect->addItem(QString::fromLatin1(name));
    }

    auto* const fileContextListModel =
        new QStringListModel(tabs, ui->fileContextList);
    ui->fileContextList->setModel(fileContextListModel);

    QStringList endpointNames;
    endpointNames.reserve(isize(settings->translation.endpoints.size()));

    for (const auto& endpoint : settings->translation.endpoints) {
        endpointNames.append(endpoint.name);
    }

    auto* const endpointListModel =
        new QStringListModel(endpointNames, ui->endpointList);
    ui->endpointList->setModel(endpointListModel);

    auto* const sectionsListModel = new QStringListModel(
        { tr("Core"),
          tr("Appearance"),
          tr("Controls"),
          tr("Translation"),
          tr("Project") },
        ui->sectionsList
    );
    ui->sectionsList->setModel(sectionsListModel);

    ui->spellcheckDictionarySelect->installEventFilter(this);
    refreshSpellcheckDictionarySelect();

    ui->stackedWidget->setCurrentIndex(0);

    connect(
        ui->addEndpointButton,
        &QPushButton::pressed,
        this,
        [this, endpointListModel] -> void {
        const u8 index = endpointListModel->rowCount();

        settings->translation.endpoints.emplace_back(
            EndpointSettings{ .name = tr("Endpoint %1").arg(index + 1) }
        );

        endpointListModel->insertRow(index);
        endpointListModel->setData(
            endpointListModel->index(index, 0),
            tr("Endpoint %1").arg(index + 1)
        );

        onTypeChange(TranslationEndpoint::Google);
    }
    );

    connect(
        ui->removeEndpointButton,
        &QPushButton::pressed,
        this,
        [this, endpointListModel] -> void {
        const QModelIndex index = ui->endpointList->currentIndex();

        if (!index.isValid()) {
            return;
        }

        endpointListModel->removeRow(index.row());
        settings->translation.endpoints.erase(
            settings->translation.endpoints.begin() + index.row()
        );
    }
    );

    connect(
        endpointListModel,
        &QStringListModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& bottomRight)
            -> void {
        settings->translation.endpoints[topLeft.row()].name =
            topLeft.data().toString();
    }
    );

    connect(
        ui->defaultBaseURLButton,
        &QPushButton::pressed,
        this,
        [this] -> void {
        const auto endpoint =
            TranslationEndpoint(ui->typeSelect->currentIndex());
        if (endpoint > TranslationEndpoint::DeepL) {
            setDefaultBaseURL(endpoint);
        }
    }
    );

    connect(
        ui->typeSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u8 index) -> void {
        onTypeChange(TranslationEndpoint(index));
    }
    );

    connect(
        ui->sectionsList->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& /* previous */)
            -> void { ui->stackedWidget->setCurrentIndex(current.row()); }
    );

    connect(
        ui->styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString& style) -> void {
        qApp->setStyle(QStyleFactory::create(style));
    }
    );

    connect(
        ui->themeSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u8 index) -> void {
        qApp->styleHints()->setColorScheme(Qt::ColorScheme(index));
    }
    );

    connect(
        ui->endpointList->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& previous)
            -> void {
        if (previous.isValid()) {
            saveCurrentEndpoint(
                settings->translation.endpoints[previous.row()]
            );
        }

        auto& endpointSettings = settings->translation.endpoints[current.row()];

        ui->apiKeyInput->setText(endpointSettings.apiKey);
        ui->yandexFolderIDInput->setText(endpointSettings.yandexFolderID);
        ui->baseURLInput->setText(endpointSettings.baseUrl);
        ui->systemPromptInput->setPlainText(endpointSettings.systemPrompt);
        ui->singleSystemPromptInput->setPlainText(
            endpointSettings.singleTranslateSystemPrompt
        );

        if (endpointSettings.temperature) {
            ui->temperatureWidget->setChecked(true);
            ui->temperatureWidget->setValue(
                endpointSettings.temperature.value()
            );
        } else {
            ui->temperatureWidget->setChecked(false);
        }

        if (endpointSettings.frequencyPenalty) {
            ui->frequencyWidget->setChecked(true);
            ui->frequencyWidget->setValue(
                endpointSettings.frequencyPenalty.value()
            );
        } else {
            ui->frequencyWidget->setChecked(false);
        }

        if (endpointSettings.precensePenalty) {
            ui->precenseWidget->setChecked(true);
            ui->precenseWidget->setValue(
                endpointSettings.precensePenalty.value()
            );
        } else {
            ui->precenseWidget->setChecked(false);
        }

        if (endpointSettings.topP) {
            ui->topPWidget->setChecked(true);
            ui->topPWidget->setValue(endpointSettings.topP.value());
        } else {
            ui->topPWidget->setChecked(false);
        }

        ui->inputTokenLimitWidget->setValue(endpointSettings.tokenLimit);
        ui->outputTokenLimitWidget->setValue(endpointSettings.outputTokenLimit);

        ui->thinkingBudgetWidget->setValue(endpointSettings.thinkingBudget);

        ui->glossaryCheckbox->setChecked(endpointSettings.useGlossary);
        ui->thinkingCheckbox->setChecked(endpointSettings.thinking);

        ui->singleTranslationCheckbox->setChecked(
            endpointSettings.singleTranslation
        );
        ui->typeSelect->setCurrentIndex(u8(endpointSettings.type));

        if (!endpointSettings.model.isEmpty()) {
            checkKey();

            bool modelFound = false;

            for (const auto idx : range(0, ui->modelSelect->count())) {
                if (ui->modelSelect->itemText(idx) == endpointSettings.model) {
                    ui->modelSelect->setCurrentIndex(idx);
                    modelFound = true;
                }
            }

            if (!modelFound) {
                QMessageBox::warning(
                    this,
                    tr("Couldn't select model"),
                    tr(
                        "Model that was previously selected for translation: %1 is not longer in the list of models provided by the endpoint."
                    )
                        .arg(endpointSettings.model)
                );
            }
        }
    }
    );

    connect(
        ui->fileContextList->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& previous)
            -> void {
        if (previous.isValid()) {
            const QString prevTab = previous.data().toString();
            const QString context = ui->fileContextInput->toPlainText();

            if (context.isEmpty()) {
                projectSettings->fileContexts.erase(prevTab);
            } else {
                projectSettings->fileContexts.insert({ prevTab, context });
            }
        }

        const QString currentTab = current.data().toString();

        if (projectSettings->fileContexts.contains(currentTab)) {
            ui->fileContextInput->setPlainText(
                projectSettings->fileContexts[currentTab]
            );
        } else {
            ui->fileContextInput->setPlainText(QString());
        }

        ui->fileContextList->setCurrentIndex(current);
    }
    );

    connect(
        ui->defaultSystemPromptButton,
        &QPushButton::pressed,
        this,
        [this] -> void {
        ui->systemPromptInput->setPlainText(DEFAULT_SYSTEM_PROMPT);
    }
    );

    connect(
        ui->singleSystemPromptDefaultButton,
        &QPushButton::pressed,
        this,
        [this] -> void {
        ui->singleSystemPromptInput->setPlainText(
            DEFAULT_SINGLE_TRANSLATE_SYSTEM_PROMPT
        );
    }
    );

    connect(
        ui->baseURLInput,
        &UnfocusLineEdit::editingFinished,
        this,
        [this] -> void {
        QString apiKey = ui->baseURLInput->text();

        if (apiKey.isEmpty()) {
            return;
        }

        QUrl url = QUrl::fromUserInput(apiKey);

        if (url.scheme().isEmpty() || url.host().isEmpty()) {
            QMessageBox::warning(
                this,
                tr("URL is invalid"),
                tr(
                    "Given URL is invalid. Please check the validity of submitted URL."
                )
            );

            ui->baseURLInput->clear();
            return;
        }
    }
    );

    connect(ui->checkKeyButton, &QPushButton::pressed, this, [this] -> void {
        checkKey();
    });

    connect(
        ui->spellcheckDictionarySelect,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString& text) -> void {
        if (text.isEmpty()) {
            return;
        }

        QString dicPath = qApp->property("data-location").toString() +
                          u"/dictionaries" + text;
        dicPath.slice(0, dicPath.size() - 3);
        dicPath += u"dic";

        if (!QFile::exists(dicPath)) {
            QMessageBox::warning(
                this,
                tr("No .dic file"),
                tr(
                    "`.dic` file corresponding to the `.aff` file does not exist. Dictionary won't work properly without the `.dic` file."
                )
            );
            ui->spellcheckDictionarySelect->setCurrentIndex(0);
        }
    }
    );

    for (const QString& style : QStyleFactory::keys()) {
        ui->styleSelect->addItem(style);
    }

    // Core
    ui->backupCheckbox->setChecked(settings->core.backup.enabled);
    ui->backupPeriodInput->setText(
        QL1SV(itos(settings->core.backup.period).data())
    );
    ui->maxBackupsInput->setText(QL1SV(itos(settings->core.backup.max).data()));

    ui->updatesCheckbox->setChecked(settings->core.checkForAppUpdates);

    // Appearance
    if (settings->appearance.translationTableFont.isEmpty()) {
        ui->fontSelect->setCurrentFont(font());
    } else {
        ui->fontSelect->setCurrentFont(
            settings->appearance.translationTableFont
        );
    }

    if (settings->appearance.translationTableFontSize == 0) {
        ui->translationTableFontSizeWidget->setValue(font().pointSize());
    } else {
        ui->translationTableFontSizeWidget->setValue(
            settings->appearance.translationTableFontSize
        );
    }

    ui->styleSelect->setCurrentText(settings->appearance.style);
    ui->themeSelect->setCurrentIndex(u8(settings->appearance.theme));

    // Controls
    ui->searchPanelInput->setKeySequence(settings->controls.searchPanel);
    ui->tabPanelInput->setKeySequence(settings->controls.tabPanel);
    ui->goToRowInput->setKeySequence(settings->controls.goToRow);
    ui->batchMenuInput->setKeySequence(settings->controls.batchMenu);
    ui->bookmarkMenuInput->setKeySequence(settings->controls.bookmarkMenu);

    ui->lineLengthHintInput->setText(
        QL1SV(itos(projectSettings->lineLengthHint).data())
    );
    ui->sourceLanguageSelect->setCurrentIndex(
        i8(projectSettings->sourceLang) + 1
    );
    ui->translationLanguageSelect->setCurrentIndex(
        i8(projectSettings->translationLang) + 1
    );
    ui->projectContextInput->setPlainText(projectSettings->projectContext);

    // Translation
    ui->leadingWhitespaceCheckbox->setChecked(
        (settings->translation.miscLints & MiscLints::LeadingWhitespace) != 0
    );
    ui->trailingWhitespaceCheckbox->setChecked(
        (settings->translation.miscLints & MiscLints::TrailingWhitespace) != 0
    );
    ui->contiguousWhitespaceCheckbox->setChecked(
        (settings->translation.miscLints & MiscLints::ContiguousWhitespace) != 0
    );
    ui->unclosedPunctuationCheckbox->setChecked(
        (settings->translation.miscLints & MiscLints::UnclosedPunctuation) != 0
    );

    ui->displayWordsCheckbox->setChecked(
        settings->translation.displayWordsAndCharacters
    );

    ui->tagMismatchCheckbox->setChecked(
        (settings->translation.miscLints & MiscLints::TagMismatch) != 0
    );
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

auto SettingsWindow::eventFilter(QObject* const watched, QEvent* const event)
    -> bool {
    if (watched == ui->spellcheckDictionarySelect) {
        if (event->type() == QEvent::MouseButtonPress) {
            refreshSpellcheckDictionarySelect();
        } else if (event->type() == QEvent::KeyPress) {
            const auto* const keyEvent = as<QKeyEvent*>(event);
            const bool popupPressed =
                keyEvent->key() == Qt::Key_F4 ||
                (keyEvent->key() == Qt::Key_Down &&
                 keyEvent->modifiers().testFlag(Qt::AltModifier));

            if (popupPressed) {
                refreshSpellcheckDictionarySelect();
            }
        }
    }

    return QDialog::eventFilter(watched, event);
}

void SettingsWindow::refreshSpellcheckDictionarySelect() {
    const QString selectedDictionary =
        ui->spellcheckDictionarySelect->currentText().isEmpty()
            ? projectSettings->spellcheckDictionary
            : ui->spellcheckDictionarySelect->currentText();

    const QStringList dictionaries = listSpellcheckDictionaries();

    ui->spellcheckDictionarySelect->blockSignals(true);

    for (const auto idx : range(1, ui->spellcheckDictionarySelect->count())) {
        ui->spellcheckDictionarySelect->removeItem(idx);
    }

    ui->spellcheckDictionarySelect->addItems(dictionaries);

    const i32 selectedIndex =
        ui->spellcheckDictionarySelect->findText(selectedDictionary);
    ui->spellcheckDictionarySelect->setCurrentIndex(selectedIndex);
    ui->spellcheckDictionarySelect->blockSignals(false);
}

void SettingsWindow::closeEvent(QCloseEvent* const event) {
    if (ui->backupPeriodInput->hasAcceptableInput()) {
        settings->core.backup.period = ui->backupPeriodInput->text().toUInt();
    } else {
        QMessageBox::warning(
            this,
            tr("Invalid backup period"),
            tr("Backup period is invalid. Unable to save.")
        );
        return;
    }

    if (ui->maxBackupsInput->hasAcceptableInput()) {
        settings->core.backup.max = ui->maxBackupsInput->text().toUInt();
    } else {
        QMessageBox::warning(
            this,
            tr("Invalid max backups"),
            tr("Max backups value is invalid. Unable to save.")
        );
        return;
    }

    settings->core.backup.enabled = ui->backupCheckbox->isChecked();
    settings->core.checkForAppUpdates = ui->updatesCheckbox->isChecked();

    settings->appearance.translationTableFont = ui->fontSelect->currentText();
    settings->appearance.translationTableFontSize =
        ui->translationTableFontSizeWidget->value();

    if (ui->leadingWhitespaceCheckbox->isChecked()) {
        settings->translation.miscLints |= MiscLints::LeadingWhitespace;
    } else {
        settings->translation.miscLints &= ~MiscLints::LeadingWhitespace;
    }

    if (ui->trailingWhitespaceCheckbox->isChecked()) {
        settings->translation.miscLints |= MiscLints::TrailingWhitespace;
    } else {
        settings->translation.miscLints &= ~MiscLints::TrailingWhitespace;
    }

    if (ui->contiguousWhitespaceCheckbox->isChecked()) {
        settings->translation.miscLints |= MiscLints::ContiguousWhitespace;
    } else {
        settings->translation.miscLints &= ~MiscLints::ContiguousWhitespace;
    }

    if (ui->unclosedPunctuationCheckbox->isChecked()) {
        settings->translation.miscLints |= MiscLints::UnclosedPunctuation;
    } else {
        settings->translation.miscLints &= ~MiscLints::UnclosedPunctuation;
    }

    if (ui->tagMismatchCheckbox->isChecked()) {
        settings->translation.miscLints |= MiscLints::TagMismatch;
    } else {
        settings->translation.miscLints &= ~MiscLints::TagMismatch;
    }

    settings->translation.displayWordsAndCharacters =
        ui->displayWordsCheckbox->isChecked();

    settings->controls.searchPanel =
        ui->searchPanelInput->keySequence().toString();
    settings->controls.tabPanel = ui->tabPanelInput->keySequence().toString();
    settings->controls.goToRow = ui->goToRowInput->keySequence().toString();
    settings->controls.batchMenu = ui->batchMenuInput->keySequence().toString();
    settings->controls.bookmarkMenu =
        ui->bookmarkMenuInput->keySequence().toString();
    settings->controls.matchMenu = ui->matchMenuInput->keySequence().toString();
    settings->controls.glossaryMenu =
        ui->glossaryMenuInput->keySequence().toString();
    settings->controls.translationsMenu =
        ui->translationsMenuInput->keySequence().toString();

    const i32 row = ui->endpointList->currentIndex().row();

    if (row != -1) {
        saveCurrentEndpoint(settings->translation.endpoints[row]);
    }

    if (ui->lineLengthHintInput->hasAcceptableInput()) {
        projectSettings->lineLengthHint =
            ui->lineLengthHintInput->text().toUInt();
    } else {
        QMessageBox::warning(
            this,
            tr("Invalid line length hint"),
            tr("Line length hint is invalid. Unable to save.")
        );
        return;
    }

    projectSettings->projectContext = ui->projectContextInput->toPlainText();
    const QString context = ui->fileContextInput->toPlainText();

    const QModelIndex currentIndex = ui->fileContextList->currentIndex();
    if (currentIndex.isValid()) {
        if (context.isEmpty()) {
            projectSettings->fileContexts.erase(currentIndex.data().toString());
        } else {
            projectSettings->fileContexts.insert(
                { currentIndex.data().toString(), context }
            );
        }
    }

    projectSettings->sourceLang =
        Algorithm(ui->sourceLanguageSelect->currentIndex() - 1);
    projectSettings->translationLang =
        Algorithm(ui->translationLanguageSelect->currentIndex() - 1);
    projectSettings->spellcheckDictionary =
        ui->spellcheckDictionarySelect->currentText();

    QDialog::closeEvent(event);
};

void SettingsWindow::saveCurrentEndpoint(EndpointSettings& settings) {
    settings.apiKey = ui->apiKeyInput->text();
    settings.yandexFolderID = ui->yandexFolderIDInput->text();
    settings.baseUrl = ui->baseURLInput->text();
    settings.model = ui->modelSelect->currentText();
    settings.systemPrompt = ui->systemPromptInput->toPlainText();
    settings.singleTranslateSystemPrompt =
        ui->singleSystemPromptInput->toPlainText();

    settings.temperature = ui->temperatureWidget->isChecked()
                               ? optional(ui->temperatureWidget->value<f32>())
                               : nullopt;
    settings.frequencyPenalty =
        ui->frequencyWidget->isChecked()
            ? optional(ui->frequencyWidget->value<f32>())
            : nullopt;
    settings.precensePenalty = ui->precenseWidget->isChecked()
                                   ? optional(ui->precenseWidget->value<f32>())
                                   : nullopt;
    settings.topP = ui->topPWidget->isChecked()
                        ? optional(ui->topPWidget->value<f32>())
                        : nullopt;

    settings.tokenLimit = ui->inputTokenLimitWidget->value();
    settings.outputTokenLimit = ui->outputTokenLimitWidget->value();

    settings.useGlossary = ui->glossaryCheckbox->isChecked();
    settings.thinking = ui->thinkingCheckbox->isChecked();

    settings.singleTranslation = ui->singleTranslationCheckbox->isChecked();
    settings.type = TranslationEndpoint(ui->typeSelect->currentIndex());
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
            ui->baseURLInput->setText(
                u"https://generativelanguage.googleapis.com/v1beta"_s
            );
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
            ui->baseURLInput->setText(
                u"https://ark.cn-beijing.volces.com/api/v3"_s
            );
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
        // TODO: Check
        return;
    }

    ByteBuffer out;

    const QByteArray apiKey = ui->apiKeyInput->text().toUtf8();
    const QByteArray baseUrl = ui->baseURLInput->text().toUtf8();

    const bool success =
        rpgm_get_models(endpoint, strtoffi(apiKey), strtoffi(baseUrl), &out);

    if (!success) {
        const QUtf8SV error = ffitostr(rpgm_error());
        qWarning() << "Getting available models failed with error: %1"_L1.arg(
            error
        );
        QMessageBox::warning(
            this,
            tr("Failed to validate key"),
            tr("Getting available models failed with error: %1").arg(error)
        );

        return;
    }

    ui->modelSelect->clear();

    const u32 len = *ras<const u32*>(out.ptr);
    u32 pos = 4;

    while (pos < out.len) {
        const u32 stringLen = *ras<const u32*>(out.ptr + pos);
        pos += 4;

        ui->modelSelect->addItem(
            QString::fromUtf8(ras<cstr>(out.ptr + pos), stringLen)
        );
        pos += stringLen;
    }

    rpgm_buffer_free(out);
}

void SettingsWindow::onTypeChange(const TranslationEndpoint endpoint) {
    ui->baseURLInput->setEnabled(true);

    switch (endpoint) {
        case TranslationEndpoint::Google:
            ui->typeDescriptionLabel->setText(tr(
                "Google Translate. Free and unlimited. Configured options don't work with this endpoint."
            ));
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
                    "OpenAI-compatible endpoint. This category fits many providers, including OpenAI itself, DeepSeek, Mistral, and local providers, such as llama.cpp and koboldcpp. Requires valid base URL, that should probably end with '/v1'. Configured options will affect this endpoint."
                ));
            } else if (
                endpoint == TranslationEndpoint::Ollama ||
                endpoint == TranslationEndpoint::Koboldcpp
            ) {
                ui->typeDescriptionLabel->setText(tr(
                    "Local endpoint. You need to set correct base url, that should probably end with '/v1'. Configured options will affect this endpoint."
                ));
            }
            break;
    }
}