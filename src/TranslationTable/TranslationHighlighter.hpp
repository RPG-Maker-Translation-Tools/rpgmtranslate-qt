#pragma once

#include "Aliases.hpp"
#include "Types.hpp"

#include <QSyntaxHighlighter>

class TranslationHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

   public:
    explicit TranslationHighlighter(QTextDocument* document);

    void setFormats(vector<LintCharState> perChar, i32 size) {
        this->perChar = std::move(perChar);
        this->size = size;
    };

   protected:
    void highlightBlock(const QString& text) override;

    vector<LintCharState> perChar;
    i32 size = 0;
};
