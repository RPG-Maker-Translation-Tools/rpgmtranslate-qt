#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class TranslationsMenu;
}  // namespace Ui

QT_END_NAMESPACE

class TranslationsMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit TranslationsMenu(QWidget* parent = nullptr);
    ~TranslationsMenu() override;

    void showTranslations(
        const array<QString, TRANSLATION_ENDPOINT_COUNT>& translations
    );

    void clear();

   signals:
    void translationClicked(const QString& translation);

   protected:
    void changeEvent(QEvent* event) override;
    auto eventFilter(QObject* obj, QEvent* event) -> bool override;

   private:
    inline auto setupUi() -> Ui::TranslationsMenu*;

    Ui::TranslationsMenu* const ui;
};