#pragma once

#include "Enums.hpp"

#include <nuspell/dictionary.hxx>

#include <QRegularExpression>
#include <QSyntaxHighlighter>

class SpellHighlighter final : public QSyntaxHighlighter {
   public:
    explicit SpellHighlighter(
        const nuspell::Dictionary* dict,
        const Algorithm* const* algorithm,
        QTextDocument* document
    );

   protected:
    void highlightBlock(const QString& text) override;

   private:
    [[nodiscard]] auto isMisspelled(const QString& word) const -> bool;

    QTextCharFormat misspelledFormat;

    const nuspell::Dictionary* const dictionary;
    const Algorithm* const* const algorithm;

    const QRegularExpression wordRegex;
};