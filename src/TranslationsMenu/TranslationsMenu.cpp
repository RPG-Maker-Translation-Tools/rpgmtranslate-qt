#include "TranslationsMenu.hpp"

#include "ClickableLabel.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

TranslationsMenu::TranslationsMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    layout(new QVBoxLayout(this)),
    translationsWidget(new QWidget(this)),
    translationsLayout(new QVBoxLayout(translationsWidget)) {
    setFixedSize(360, 270);
    setDragMoveEnabled(true);

    auto* const scrollArea = new QScrollArea(this);
    scrollArea->setWidget(translationsWidget);
    scrollArea->setWidgetResizable(true);

    layout->addWidget(new QLabel(tr("Translations Menu"), this));
    layout->addWidget(scrollArea);

    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    translationsLayout->setContentsMargins(4, 4, 4, 4);
    translationsLayout->setSpacing(4);
}

void TranslationsMenu::showTranslations(
    const vector<QString>& translations,
    const shared_ptr<Settings>& settings
) {
    clear();

    for (const auto& [idx, translation] : views::enumerate(translations)) {
        const QString& name = settings->translation.endpoints[idx].name;

        auto* const translationWidget = new QWidget(this);
        auto* const translationWidgetLayout =
            new QVBoxLayout(translationWidget);

        translationWidgetLayout->setContentsMargins(4, 4, 4, 4);
        translationWidgetLayout->setSpacing(4);

        auto* const headerLabel = new QLabel(name, translationWidget);
        headerLabel->setWordWrap(true);

        auto* const translationLabel =
            new ClickableLabel(translation, translationWidget);
        translationLabel->setCursor(QCursor(Qt::PointingHandCursor));
        translationLabel->setWordWrap(true);

        auto* const scrollArea = new QScrollArea(translationWidget);
        scrollArea->setWidget(translationLabel);

        translationWidgetLayout->addWidget(headerLabel);
        translationWidgetLayout->addWidget(scrollArea);

        translationsLayout->addWidget(translationWidget);

        connect(
            translationLabel,
            &ClickableLabel::clicked,
            this,
            [this, translationLabel] -> void {
            emit translationClicked(translationLabel->text());
        }
        );
    }
};

void TranslationsMenu::showError(const QString& error) {
    clear();
    translationsLayout->addWidget(new QLabel(error, this));
};

void TranslationsMenu::clear() {
    while (const auto* const item = translationsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
};
