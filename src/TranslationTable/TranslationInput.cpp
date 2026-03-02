#include "TranslationInput.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>

TranslationInput::TranslationInput(
    const u16* const* const hint,
    QWidget* const parent
) :
    QPlainTextEdit(parent),
    lengthHint(hint) {
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(
        this,
        &QPlainTextEdit::textChanged,
        this,
        &TranslationInput::onTextChanged
    );
    connect(
        document(),
        &QTextDocument::contentsChanged,
        this,
        &TranslationInput::updateContentHeight
    );
}

void TranslationInput::keyPressEvent(QKeyEvent* const event) {
    if (event->key() == Qt::Key_Escape) {
        emit editingFinished();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Backspace) {
        QTextCursor cursor = textCursor();
        const i32 cursorPos = cursor.position();

        if (!lastReplacements.empty()) {
            for (const auto& replacement : lastReplacements) {
                const i32 replEndPos =
                    replacement.position + replacement.replacement.size();

                if (cursorPos == replEndPos) {
                    blockTextChanged = true;
                    cursor.setPosition(replacement.position);
                    cursor.setPosition(replEndPos, QTextCursor::KeepAnchor);
                    cursor.insertText(replacement.original.toString());
                    blockTextChanged = false;

                    lastReplacements.clear();
                    event->accept();
                    return;
                }
            }
        }

        lastReplacements.clear();
    }

    QPlainTextEdit::keyPressEvent(event);
}

void TranslationInput::onTextChanged() {
    if (blockTextChanged) {
        return;
    }

    performAutoReplacements();
}

void TranslationInput::updateContentHeight() {
    if (blockTextChanged) {
        return;
    }

    const auto fontMetrics = QFontMetrics(font());
    const i32 lineHeight = fontMetrics.lineSpacing();
    const i32 lineCount = document()->lineCount();
    const i32 contentHeight = (lineCount * lineHeight) + 10;

    if (contentHeight != lastContentHeight) {
        lastContentHeight = contentHeight;
        emit contentHeightChanged(contentHeight);
    }
}

void TranslationInput::performAutoReplacements() {
    lastReplacements.clear();

    QTextCursor cursor = textCursor();
    const i32 originalPosition = cursor.position();
    const QString text = toPlainText();

    constexpr array<std::pair<QLatin1StringView, QStringView>, 5>
        replacements = { std::pair{ "<<"_L1, u"«" },
                         { ">>"_L1, u"»" },
                         { "--"_L1, u"—" },
                         { ",,"_L1, u"„" },
                         { "''"_L1, u"“" } };

    for (const auto& pair : replacements) {
        const QLatin1StringView source = pair.first;
        const QStringView replacement = pair.second;

        if (originalPosition >= source.size()) {
            const i32 checkPos = originalPosition - source.size();

            if (QStringView(text).mid(checkPos, source.size()) == source) {
                blockTextChanged = true;

                lastReplacements.emplace_back(source, replacement, checkPos);

                cursor.setPosition(checkPos);
                cursor.setPosition(originalPosition, QTextCursor::KeepAnchor);
                cursor.insertText(replacement.toString());

                blockTextChanged = false;
                break;
            }
        }
    }
}

void TranslationInput::paintEvent(QPaintEvent* const event) {
    QPlainTextEdit::paintEvent(event);

    const u16 hint = **lengthHint;
    if (hint == 0) {
        return;
    }

    auto painter = QPainter(viewport());
    painter.setPen(QColor(255, 0, 0, 80));

    const int charWidth = fontMetrics().horizontalAdvance(' ');

    const int xPos =
        contentOffset().x() + document()->documentMargin() + (charWidth * hint);

    painter.drawLine(xPos, 0, xPos, viewport()->height());
};