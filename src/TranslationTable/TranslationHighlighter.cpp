#include "TranslationHighlighter.hpp"

#include "Constants.hpp"
#include "Types.hpp"

#include <QColor>
#include <QFile>
#include <QTextCharFormat>

TranslationHighlighter::TranslationHighlighter(QTextDocument* const document) : QSyntaxHighlighter(document) {
    document->disconnect(this);
}

void TranslationHighlighter::highlightBlock(const QString& text) {
    const auto sameState = [](const LintCharState& lhs, const LintCharState& rhs) -> bool {
        if (lhs.background != rhs.background || lhs.underlineColor != rhs.underlineColor ||
            lhs.underlineStyle != rhs.underlineStyle || lhs.foreground != rhs.foreground) {
            return false;
        }
        if (lhs.entries.size() != rhs.entries.size()) {
            return false;
        }
        for (i32 i = 0; i < lhs.entries.size(); i++) {
            if (lhs.entries[i].translationId != rhs.entries[i].translationId ||
                lhs.entries[i].captured != rhs.entries[i].captured) {
                return false;
            }
        }
        return true;
    };

    const i32 blockStart = currentBlock().position();
    const i32 blockLen = scast<i32>(text.length());

    if (blockStart + blockLen > size) {
        return;
    }

    i32 runStart = 0;
    while (runStart < blockLen) {
        i32 runEnd = runStart + 1;
        while (runEnd < blockLen && sameState(perChar[blockStart + runStart], perChar[blockStart + runEnd])) {
            runEnd++;
        }

        const LintCharState& state = perChar[blockStart + runStart];
        if (state.background || state.underlineStyle != QTextCharFormat::NoUnderline || state.foreground ||
            !state.entries.isEmpty()) {
            QTextCharFormat fmt;
            if (state.background) {
                fmt.setBackground(*state.background);
            }
            if (state.underlineStyle != QTextCharFormat::NoUnderline) {
                fmt.setUnderlineStyle(state.underlineStyle);
                fmt.setUnderlineColor(*state.underlineColor);
            }
            if (state.foreground) {
                fmt.setForeground(*state.foreground);
            }
            if (!state.entries.isEmpty()) {
                fmt.setProperty(LINT_ENTRIES_ID, QVariant::fromValue(state.entries));
            }

            setFormat(runStart, runEnd - runStart, fmt);
        }
        runStart = runEnd;
    }
}