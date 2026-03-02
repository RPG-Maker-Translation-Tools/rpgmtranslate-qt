#include "SettingsWindow.hpp"

#include "Enums.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "rpgmtranslate.h"
#include "ui_SettingsWindow.h"

#include <QDoubleValidator>
#include <QMessageBox>
#include <QStringListModel>
#include <QStyleFactory>
#include <QStyleHints>

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
    for (const auto [value, name] : magic_enum::enum_entries<Algorithm>()) {
        ui->sourceLanguageSelect->addItem(QString::fromLatin1(name));
        ui->translationLanguageSelect->addItem(QString::fromLatin1(name));
    }

    auto* const fileContextListModel =
        new QStringListModel(tabs, ui->fileContextList);
    ui->fileContextList->setModel(fileContextListModel);

    auto* const endpointListModel = new QStringListModel(
        { u"Google"_s,
          u"Yandex"_s,
          u"DeepL"_s,
          u"OpenAI"_s,
          u"Claude"_s,
          u"Gemini"_s,
          u"DeepSeek"_s,
          u"OpenAI-compatible"_s,
          u"Ollama"_s },
        ui->endpointList
    );
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

    ui->stackedWidget->setCurrentIndex(0);

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
        if (!saveCurrentEndpoint(previous.row())) {
            ui->endpointList->selectionModel()->blockSignals(true);
            ui->endpointList->selectionModel()->setCurrentIndex(
                previous,
                QItemSelectionModel::NoUpdate
            );
            ui->endpointList->selectionModel()->blockSignals(false);
            return;
        }

        const u8 currentRow = current.row();

        ui->apiKeyWidget->hide();
        ui->yandexFolderWidget->hide();
        ui->baseURLWidget->hide();
        ui->checkKeyButton->hide();
        ui->modelWidget->hide();
        ui->tokenLimitWidget->hide();
        ui->outputLimitWidget->hide();
        ui->temperatureWidget->hide();
        ui->glossaryCheckbox->hide();
        ui->thinkingCheckbox->hide();
        ui->systemPromptWidget->hide();
        ui->singleSystemPromptWidget->hide();

        auto applyLLM = [this](
                            const QString& description,
                            const LLMSettings& settings
                        ) -> void {
            ui->endpointDescriptionLabel->setText(description);
            ui->singleTranslationCheckbox->setChecked(
                settings.singleTranslation
            );

            ui->apiKeyInput->setText(settings.apiKey);
            ui->baseURLInput->setText(settings.baseUrl);
            ui->modelSelect->setCurrentText(settings.model);
            ui->tokenLimitInput->setText(QString::number(settings.tokenLimit));
            ui->outputLimitInput->setText(
                QString::number(settings.outputTokenLimit)
            );
            ui->temperatureInput->setText(
                QString::number(settings.temperature)
            );
            ui->glossaryCheckbox->setChecked(settings.useGlossary);
            ui->thinkingCheckbox->setChecked(settings.thinking);
            ui->systemPromptInput->setPlainText(settings.systemPrompt);
            ui->singleSystemPromptInput->setPlainText(
                settings.singleTranslateSystemPrompt
            );

            ui->apiKeyWidget->show();
            ui->baseURLWidget->show();
            ui->checkKeyButton->show();
            ui->modelWidget->show();
            ui->tokenLimitWidget->show();
            ui->outputLimitWidget->show();
            ui->temperatureWidget->show();
            ui->glossaryCheckbox->show();
            ui->thinkingCheckbox->show();
            ui->systemPromptWidget->show();
            ui->singleSystemPromptWidget->show();
        };

        switch (TranslationEndpoint(currentRow)) {
            case TranslationEndpoint::Google:
                ui->endpointDescriptionLabel->setText(
                    tr("Google Translate API. Free and unlimited.")
                );
                ui->singleTranslationCheckbox->setChecked(
                    settings->translation.google.singleTranslation
                );
                break;

            case TranslationEndpoint::Yandex:
                ui->endpointDescriptionLabel->setText(
                    tr("Yandex Translate API. Requires API key and folder ID.")
                );
                ui->singleTranslationCheckbox->setChecked(
                    settings->translation.yandex.singleTranslation
                );

                ui->apiKeyInput->setText(settings->translation.yandex.apiKey);
                ui->yandexFolderIDInput->setText(
                    settings->translation.yandex.folderId
                );

                ui->apiKeyWidget->show();
                ui->yandexFolderWidget->show();
                break;

            case TranslationEndpoint::DeepL:
                ui->endpointDescriptionLabel->setText(
                    tr("DeepL API. Requires API key and allows using glossary.")
                );
                ui->singleTranslationCheckbox->setChecked(
                    settings->translation.deepl.singleTranslation
                );

                ui->apiKeyInput->setText(settings->translation.deepl.apiKey);
                ui->glossaryCheckbox->setChecked(
                    settings->translation.deepl.useGlossary
                );

                ui->apiKeyWidget->show();
                ui->glossaryCheckbox->show();
                break;
            case TranslationEndpoint::OpenAI:
                applyLLM(
                    tr("OpenAI ChatGPT API. Allows fine-tuning."),
                    settings->translation.chatgpt
                );
                break;
            case TranslationEndpoint::Anthropic:
                applyLLM(
                    tr("Anthropic Claude API. Allows fine-tuning."),
                    settings->translation.claude
                );
                break;
            case TranslationEndpoint::DeepSeek:
                applyLLM(
                    tr("DeepSeek API. Allows fine-tuning."),
                    settings->translation.deepseek
                );
                break;
            case TranslationEndpoint::Gemini:
                applyLLM(
                    tr("Google Gemini API. Allows fine-tuning."),
                    settings->translation.gemini
                );
                break;
            case TranslationEndpoint::OpenAICompatible:
                applyLLM(
                    tr(
                        "OpenAI-compatible API (e.g. koboldcpp). Requires base URL. Allows fine-tuning."
                    ),
                    settings->translation.openaiCompatible
                );
                break;

            case TranslationEndpoint::Ollama:
                applyLLM(
                    tr("Ollama API. Requires base URL. Allows fine-tuning."),
                    settings->translation.ollama
                );
                break;
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
                projectSettings->fileContexts.remove(prevTab);
            } else {
                projectSettings->fileContexts.insert(prevTab, context);
            }
        }

        const QString currentTab = current.data().toString();

        if (projectSettings->fileContexts.contains(currentTab)) {
            ui->fileContextInput->setPlainText(
                projectSettings->fileContexts[currentTab].toString()
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

    connect(ui->checkKeyButton, &QPushButton::pressed, this, [this] -> void {
        const TranslationEndpoint endpoint = TranslationEndpoint(
            ui->endpointList->selectionModel()->selectedRows().first().row()
        );

        ByteBuffer out;
        const QByteArray apiKey = ui->apiKeyInput->text().toUtf8();
        const QByteArray baseUrl = ui->baseURLInput->text().toUtf8();

        const FFIString error = rpgm_get_models(
            endpoint,
            { .ptr = apiKey.data(), .len = usize(apiKey.size()) },
            { .ptr = baseUrl.data(), .len = usize(baseUrl.size()) },
            &out
        );

        if (error.ptr != nullptr) {
            QMessageBox::warning(
                this,
                tr("Failed to validate key"),
                tr("Getting available models failed with error: %1")
                    .arg(QString::fromUtf8(error.ptr, isize(error.len)))
            );
            rpgm_string_free(error);
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
    });

    ui->fontSizeInput->setValidator(
        new QIntValidator(8, 96, ui->fontSizeInput)
    );

    ui->tokenLimitInput->setValidator(
        new QIntValidator(1000, UINT16_MAX, ui->tokenLimitInput)
    );

    ui->outputLimitInput->setValidator(
        new QIntValidator(1000, UINT16_MAX, ui->tokenLimitInput)
    );

    ui->temperatureInput->setValidator(
        new QDoubleValidator(0.0, 2.0, 3, ui->temperatureInput)
    );

    for (const QString& style : QStyleFactory::keys()) {
        ui->styleSelect->addItem(style);
    }

    // Core
    ui->backupCheckbox->setChecked(settings->core.backup.enabled);
    ui->backupPeriodInput->setText(
        QString::number(settings->core.backup.period)
    );
    ui->maxBackupsInput->setText(QString::number(settings->core.backup.max));

    ui->updatesCheckbox->setChecked(settings->core.checkForUpdates);

    // Appearance
    if (settings->appearance.translationTableFont.isEmpty()) {
        ui->fontSelect->setCurrentFont(font());
    } else {
        ui->fontSelect->setCurrentFont(
            settings->appearance.translationTableFont
        );
    }

    if (settings->appearance.translationTableFontSize == 0) {
        ui->fontSizeInput->setText(QString::number(font().pointSize()));
    } else {
        ui->fontSizeInput->setText(
            QString::number(settings->appearance.translationTableFontSize)
        );
    }

    ui->trailingWhitespaceCheckbox->setChecked(
        settings->appearance.displayTrailingWhitespace
    );
    ui->displayWordsCheckbox->setChecked(
        settings->appearance.displayWordsAndCharacters
    );

    ui->styleSelect->setCurrentText(settings->appearance.style);
    ui->themeSelect->setCurrentIndex(u8(settings->appearance.theme));

    // Controls
    ui->searchPanelInput->setKeySequence(settings->controls.searchPanel);
    ui->tabPanelInput->setKeySequence(settings->controls.tabPanel);
    ui->goToRowInput->setKeySequence(settings->controls.goToRow);
    ui->batchMenuInput->setKeySequence(settings->controls.batchMenu);
    ui->bookmarkMenuInput->setKeySequence(settings->controls.bookmarkMenu);

    // Translation
    ui->apiKeyWidget->hide();
    ui->yandexFolderWidget->hide();
    ui->baseURLWidget->hide();
    ui->checkKeyButton->hide();
    ui->modelWidget->hide();
    ui->tokenLimitWidget->hide();
    ui->temperatureWidget->hide();
    ui->glossaryCheckbox->hide();
    ui->thinkingCheckbox->hide();
    ui->systemPromptWidget->hide();
    ui->singleSystemPromptWidget->hide();
    ui->endpointList->setCurrentIndex(endpointListModel->index(0));

    ui->lineLengthHintInput->setText(
        QString::number(projectSettings->lineLengthHint)
    );
    ui->sourceLanguageSelect->setCurrentIndex(
        i8(projectSettings->sourceLang) + 1
    );
    ui->translationLanguageSelect->setCurrentIndex(
        i8(projectSettings->translationLang) + 1
    );
    ui->projectContextInput->setPlainText(projectSettings->projectContext);
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
    settings->core.checkForUpdates = ui->updatesCheckbox->isChecked();

    settings->appearance.translationTableFont = ui->fontSelect->currentText();
    settings->appearance.displayTrailingWhitespace =
        ui->trailingWhitespaceCheckbox->isChecked();
    settings->appearance.displayWordsAndCharacters =
        ui->displayWordsCheckbox->isChecked();

    if (ui->fontSizeInput->hasAcceptableInput()) {
        settings->appearance.translationTableFontSize =
            ui->fontSizeInput->text().toUInt();
    } else {
        QMessageBox::warning(
            this,
            tr("Invalid font size"),
            tr("Font size is invalid. Unable to save.")
        );
        return;
    }

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

    if (!saveCurrentEndpoint(ui->endpointList->currentIndex().row())) {
        return;
    };

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
            projectSettings->fileContexts.remove(
                currentIndex.data().toString()
            );
        } else {
            projectSettings->fileContexts.insert(
                currentIndex.data().toString(),
                context
            );
        }
    }

    projectSettings->sourceLang =
        Algorithm(ui->sourceLanguageSelect->currentIndex() - 1);
    projectSettings->translationLang =
        Algorithm(ui->translationLanguageSelect->currentIndex() - 1);

    QDialog::closeEvent(event);
};

auto SettingsWindow::saveCurrentEndpoint(const u8 row) -> bool {
    if (row == u8(-1)) {
        return true;
    }

    if (TranslationEndpoint(row) > TranslationEndpoint::DeepL) {
        if (!ui->tokenLimitInput->hasAcceptableInput()) {
            QMessageBox::warning(
                this,
                tr("Invalid token limit"),
                tr(
                    "Token input contains invalid value. Valid input is range from 1000 to 65536."
                )
            );
            return false;
        }

        if (!ui->temperatureInput->hasAcceptableInput()) {
            QMessageBox::warning(
                this,
                tr("Invalid temperature"),
                tr(
                    "Temperature input contains invalid value. Valid input is range from 0.0 to 2.0."
                )
            );
            return false;
        }
    }

    auto applyYandex = [this](auto& settings) -> void {
        settings.singleTranslation = ui->singleTranslationCheckbox->isChecked();
        settings.apiKey = ui->apiKeyInput->text();
        settings.folderId = ui->yandexFolderIDInput->text();
    };

    auto applyDeepL = [this](auto& settings) -> void {
        settings.singleTranslation = ui->singleTranslationCheckbox->isChecked();
        settings.apiKey = ui->apiKeyInput->text();
        settings.useGlossary = ui->glossaryCheckbox->isChecked();
    };

    auto applyLLM = [this](LLMSettings& settings) -> void {
        settings.singleTranslation = ui->singleTranslationCheckbox->isChecked();
        settings.apiKey = ui->apiKeyInput->text();
        settings.baseUrl = ui->baseURLInput->text();
        settings.model = ui->modelSelect->currentText();
        settings.tokenLimit = ui->tokenLimitInput->text().toUInt();
        settings.outputTokenLimit = ui->outputLimitInput->text().toUInt();
        settings.temperature = ui->temperatureInput->text().toFloat();
        settings.useGlossary = ui->glossaryCheckbox->isChecked();
        settings.thinking = ui->thinkingCheckbox->isChecked();
        settings.systemPrompt = ui->systemPromptInput->toPlainText();
        settings.singleTranslateSystemPrompt =
            ui->singleSystemPromptInput->toPlainText();
    };

    switch (TranslationEndpoint(row)) {
        case TranslationEndpoint::Google:
            settings->translation.google.singleTranslation =
                ui->singleTranslationCheckbox->isChecked();
            break;
        case TranslationEndpoint::Yandex:
            applyYandex(settings->translation.yandex);
            break;
        case TranslationEndpoint::DeepL:
            applyDeepL(settings->translation.deepl);
            break;
        case TranslationEndpoint::OpenAI:
            applyLLM(settings->translation.chatgpt);
            break;
        case TranslationEndpoint::Anthropic:
            applyLLM(settings->translation.claude);
            break;
        case TranslationEndpoint::DeepSeek:
            applyLLM(settings->translation.deepseek);
            break;
        case TranslationEndpoint::Gemini:
            applyLLM(settings->translation.gemini);
            break;
        case TranslationEndpoint::OpenAICompatible:
            applyLLM(settings->translation.openaiCompatible);
            break;
        case TranslationEndpoint::Ollama:
            applyLLM(settings->translation.ollama);
            break;
    }

    return true;
}