#pragma once

#include "Aliases.hpp"
#include "Settings.hpp"

#ifdef ENABLE_NUSPELL
#include <nuspell/dictionary.hxx>

#include <QRegularExpression>
#endif

#include <QSyntaxHighlighter>

class TranslationHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit TranslationHighlighter(
        const TranslationSettings* translationSettings,
#ifdef ENABLE_NUSPELL
        const nuspell::Dictionary* dictionary,
        const bool* dictionaryReady,
#endif
        QTextDocument* document
    );

   protected:
    void highlightBlock(const QString& text) override;

   private:
#ifdef ENABLE_NUSPELL
    const nuspell::Dictionary* const dictionary;
    const bool* const isDictionaryReady;

    static inline const QRegularExpression wordRegex = QRegularExpression(
        uR"(\b[\p{L}']+\b)"_s,
        QRegularExpression::UseUnicodePropertiesOption
    );
#endif

    const TranslationSettings* translationSettings;
};
