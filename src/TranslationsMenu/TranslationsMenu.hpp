#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"

class TranslationsMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit TranslationsMenu(QWidget* parent = nullptr);

    void showTranslations(const TranslateSingleResult& translations, const Settings* projectSettings);

    void showError(const QString& error);

    void clear();

   signals:
    void translationClicked(const QString& translation);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    QVBoxLayout* layout;
    QLabel* titleLabel;

    QWidget* translationsWidget;
    QVBoxLayout* translationsLayout;
};
