
#ifdef ENABLE_NUSPELL
#include "SpellHighlighter.hpp"

#include "Aliases.hpp"

#include <nuspell/finder.hxx>

#include <QRegularExpression>

SpellHighlighter::SpellHighlighter(
    const nuspell::Dictionary* const dict,
    const Algorithm* const* const algorithm_,
    QTextDocument* const document
) :
    QSyntaxHighlighter(document),
    wordRegex(QRegularExpression(uR"(\b[\p{L}']+\b)"_s)),
    dictionary(dict),
    algorithm(algorithm_) {
    misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    misspelledFormat.setUnderlineColor(Qt::red);
}

void SpellHighlighter::highlightBlock(const QString& text) {
    if (**algorithm == Algorithm::None) {
        setFormat(0, 0, misspelledFormat);
        return;
    }

    const auto matches = wordRegex.globalMatchView(text);

    for (const auto& match : matches) {
        const QString word = match.captured();

        if (isMisspelled(word)) {
            setFormat(
                i32(match.capturedStart()),
                i32(match.capturedLength()),
                misspelledFormat
            );
        }
    }
}

auto SpellHighlighter::isMisspelled(const QString& word) const -> bool {
    const QByteArray utf8Word = word.toUtf8();
    return !dictionary->spell(
        std::string_view(utf8Word.data(), utf8Word.size())
    );
}
#endif