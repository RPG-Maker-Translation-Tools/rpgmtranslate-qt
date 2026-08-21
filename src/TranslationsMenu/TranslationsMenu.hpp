#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"

class TranslationsMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit TranslationsMenu(QWidget* parent = nullptr);

    void showTranslations(const vector<QString>& translations, const shared_ptr<Settings>& projectSettings);

    void showError(const QString& error);

    void clear();

   signals:
    void translationClicked(const QString& translation);

   private:
    QVBoxLayout* layout;

    QWidget* translationsWidget;
    QVBoxLayout* translationsLayout;
};
