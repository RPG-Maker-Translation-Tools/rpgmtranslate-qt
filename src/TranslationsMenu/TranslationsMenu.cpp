#include "TranslationsMenu.hpp"

#include "Enums.hpp"
#include "PersistentMenu.hpp"
#include "ui_TranslationsMenu.h"

#include <QMouseEvent>

TranslationsMenu::TranslationsMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    ui(setupUi()) {
    setDragMoveEnabled(true);

    ui->googleTranslation->installEventFilter(this);
    ui->yandexTranslation->installEventFilter(this);
    ui->deeplTranslation->installEventFilter(this);
    ui->deepseekTranslation->installEventFilter(this);
    ui->openaiTranslation->installEventFilter(this);
    ui->anthropicTranslation->installEventFilter(this);
    ui->geminiTranslation->installEventFilter(this);
    ui->openaiCompatibleTranslation->installEventFilter(this);
    ui->ollamaTranslation->installEventFilter(this);
}

TranslationsMenu::~TranslationsMenu() {
    delete ui;
}

auto TranslationsMenu::setupUi() -> Ui::TranslationsMenu* {
    auto* const ui_ = new Ui::TranslationsMenu();
    ui_->setupUi(this);
    return ui_;
}

void TranslationsMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
};

void TranslationsMenu::showTranslations(
    const array<QString, TRANSLATION_ENDPOINT_COUNT>& translations
) {
    clear();

    for (const auto& [idx, translation] : views::enumerate(translations)) {
        switch (TranslationEndpoint(idx)) {
            case TranslationEndpoint::Google:
                ui->googleTranslation->setText(translation);
                break;
            case TranslationEndpoint::Yandex:
                ui->yandexTranslation->setText(translation);
                break;
            case TranslationEndpoint::DeepL:
                ui->deeplTranslation->setText(translation);
                break;
            case TranslationEndpoint::OpenAI:
                ui->openaiTranslation->setText(translation);
                break;
            case TranslationEndpoint::Anthropic:
                ui->anthropicTranslation->setText(translation);
                break;
            case TranslationEndpoint::Gemini:
                ui->geminiTranslation->setText(translation);
                break;
            case TranslationEndpoint::DeepSeek:
                ui->deepseekTranslation->setText(translation);
                break;
            case TranslationEndpoint::OpenAICompatible:
                ui->openaiCompatibleTranslation->setText(translation);
                break;
            case TranslationEndpoint::Ollama:
                ui->ollamaTranslation->setText(translation);
                break;
        }
    }
};

void TranslationsMenu::clear() {
    ui->googleTranslation->clear();
    ui->yandexTranslation->clear();
    ui->deeplTranslation->clear();
    ui->openaiTranslation->clear();
    ui->anthropicTranslation->clear();
    ui->geminiTranslation->clear();
    ui->deepseekTranslation->clear();
    ui->openaiCompatibleTranslation->clear();
    ui->ollamaTranslation->clear();
}

auto TranslationsMenu::eventFilter(QObject* const obj, QEvent* const event)
    -> bool {
    if (event->type() != QEvent::MouseButtonPress ||
        !obj->objectName().endsWith("Translation"_L1)) {
        return PersistentMenu::eventFilter(obj, event);
    }

    emit translationClicked(as<QLabel*>(obj)->text());
    return true;
}