#include "WhitespaceHighlighter.hpp"

#include <QColor>

void WhitespaceHighlighter::highlightBlock(const QString& text) {
    if (!**enabled) {
        setFormat(0, 0, format);
        return;
    }

    const i32 size = i32(text.size());
    if (size == 0) {
        return;
    }

    i32 lead = 0;
    while (lead < size && text.at(lead).isSpace()) {
        ++lead;
    }

    if (lead > 0) {
        setFormat(0, lead, format);
    }

    i32 lastNonSpace = size - 1;
    while (lastNonSpace >= 0 && text.at(lastNonSpace).isSpace()) {
        --lastNonSpace;
    }

    const i32 trailStart = lastNonSpace + 1;
    const i32 trailLen = size - trailStart;

    if (trailLen > 0 && trailStart >= lead) {
        setFormat(trailStart, trailLen, format);
    }
}

WhitespaceHighlighter::WhitespaceHighlighter(
    const bool* const* const enabled,
    QTextDocument* const doc
) :
    QSyntaxHighlighter(doc),
    enabled(enabled) {
    format.setBackground(QColor(255, 0, 0, 80));
}
