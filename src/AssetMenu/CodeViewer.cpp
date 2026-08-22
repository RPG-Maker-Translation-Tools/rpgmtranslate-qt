#include "CodeViewer.hpp"

#include "Utils.hpp"

#include <QPainter>
#include <QTextBlock>

namespace {
constexpr i32 DECIMAL_BASE = 10;
constexpr i32 LINE_NUMBER_AREA_PADDING = 8;
}  // namespace

LineNumberArea::LineNumberArea(CodeViewer* const editor) : QWidget(editor), editor(editor) {};

[[nodiscard]] auto LineNumberArea::sizeHint() const -> QSize {
    return { editor->lineNumberAreaWidth(), 0 };
}

void LineNumberArea::paintEvent(QPaintEvent* const event) {
    editor->lineNumberAreaPaintEvent(event);
}

CodeViewer::CodeViewer(QWidget* const parent) : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeViewer::updateLineNumberAreaWidth);

    connect(this, &QPlainTextEdit::updateRequest, this, &CodeViewer::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

void CodeViewer::setHighlightedHtml(const QString& html) {
    clear();
    appendHtml(html);
}

[[nodiscard]] auto CodeViewer::lineNumberAreaWidth() const -> i32 {
    i32 digits = 1;
    i32 maxBlocks = max(1, blockCount());

    while (maxBlocks >= DECIMAL_BASE) {
        maxBlocks /= DECIMAL_BASE;
        digits++;
    }

    return (fontMetrics().horizontalAdvance(u'9') * digits) + LINE_NUMBER_AREA_PADDING;
}

void CodeViewer::lineNumberAreaPaintEvent(QPaintEvent* const event) {
    auto painter = QPainter(lineNumberArea);

    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

    painter.setPen(palette().color(QPalette::Mid));
    painter.drawLine(
        lineNumberArea->width() - 1,
        event->rect().top(),
        lineNumberArea->width() - 1,
        event->rect().bottom()
    );

    QTextBlock block = firstVisibleBlock();
    i32 blockNumber = block.blockNumber();
    i32 top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    i32 bottom = top + qRound(blockBoundingRect(block).height());

    painter.setFont(font());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString(itos(blockNumber + 1).qsv());
            painter.setPen(palette().color(QPalette::PlaceholderText));
            painter.drawText(
                0,
                scast<i32>(top),
                lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight | Qt::AlignVCenter,
                number
            );
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}

void CodeViewer::resizeEvent(QResizeEvent* const event) {
    QPlainTextEdit::resizeEvent(event);

    const QRect rect = contentsRect();
    lineNumberArea->setGeometry(QRect(rect.left(), rect.top(), lineNumberAreaWidth(), rect.height()));
}

void CodeViewer::updateLineNumberAreaWidth(const i32 /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeViewer::updateLineNumberArea(const QRect& rect, const i32 yPos) {
    if (yPos != 0) {
        lineNumberArea->scroll(0, yPos);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}
