#include "TranslationsMenu.hpp"

#include "ClickableLabel.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"

#include <QEvent>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
constexpr i32 LAYOUT_MARGIN = 8;
constexpr i32 LAYOUT_SPACING = 8;

constexpr i32 TRANSLATIONS_LAYOUT_MARGIN = 4;
constexpr i32 TRANSLATIONS_LAYOUT_SPACING = 4;

constexpr i32 MENU_WIDTH = 360;
constexpr i32 MENU_HEIGHT = 270;
}  // namespace

TranslationsMenu::TranslationsMenu(QWidget* const parent) :
    PersistentMenu(parent),
    layout(new QVBoxLayout(this)),
    titleLabel(new QLabel(tr("Translations Menu"), this)),
    translationsWidget(new QWidget(this)),
    translationsLayout(new QVBoxLayout(translationsWidget)) {
    setFixedSize(MENU_WIDTH, MENU_HEIGHT);
    setDragMoveEnabled(true);

    auto* const scrollArea = new QScrollArea(this);
    scrollArea->setWidget(translationsWidget);
    scrollArea->setWidgetResizable(true);

    layout->addWidget(titleLabel);
    layout->addWidget(scrollArea);

    layout->setContentsMargins(LAYOUT_MARGIN, LAYOUT_MARGIN, LAYOUT_MARGIN, LAYOUT_MARGIN);
    layout->setSpacing(LAYOUT_SPACING);

    translationsLayout->setContentsMargins(
        TRANSLATIONS_LAYOUT_MARGIN,
        TRANSLATIONS_LAYOUT_MARGIN,
        TRANSLATIONS_LAYOUT_MARGIN,
        TRANSLATIONS_LAYOUT_MARGIN
    );
    translationsLayout->setSpacing(TRANSLATIONS_LAYOUT_SPACING);
}

void TranslationsMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        titleLabel->setText(tr("Translations Menu"));
    }

    PersistentMenu::changeEvent(event);
}

void TranslationsMenu::showTranslations(const TranslateSingleResult& translations, const Settings* settings) {
    clear();

    for (const auto& [idx, answer] : views::enumerate(translations)) {
        // The endpoints can have been removed since the request was made
        if (idx >= scast<isize>(settings->translation.endpoints.size())) {
            break;
        }

        const QString& name = settings->translation.endpoints[idx].name;

        auto* const translationWidget = new QWidget(this);
        auto* const translationWidgetLayout = new QVBoxLayout(translationWidget);

        translationWidgetLayout->setContentsMargins(4, 4, 4, 4);
        translationWidgetLayout->setSpacing(4);

        auto* const headerLabel = new QLabel(name, translationWidget);
        headerLabel->setWordWrap(true);

        auto* const translationLabel = new ClickableLabel(answer.text, translationWidget);
        translationLabel->setWordWrap(true);

        auto* const scrollArea = new QScrollArea(translationWidget);
        scrollArea->setWidget(translationLabel);

        translationWidgetLayout->addWidget(headerLabel);
        translationWidgetLayout->addWidget(scrollArea);

        translationsLayout->addWidget(translationWidget);

        // The reason an endpoint failed is not a translation: it can be read, not inserted
        if (answer.failed) {
            translationLabel->setEnabled(false);
            continue;
        }

        translationLabel->setCursor(QCursor(Qt::PointingHandCursor));

        connect(translationLabel, &ClickableLabel::clicked, this, [this, translationLabel] -> void {
            emit translationClicked(translationLabel->text());
        });
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
