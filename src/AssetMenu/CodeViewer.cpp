#include "CodeViewer.hpp"

#include "rpgmtranslate.h"

#include <QPainter>
#include <QTextCharFormat>

LineNumberArea::LineNumberArea(CodeViewer* const editor) :
    QWidget(editor),
    editor(editor) {};

[[nodiscard]] auto LineNumberArea::sizeHint() const -> QSize {
    return { editor->lineNumberAreaWidth(), 0 };
}

void LineNumberArea::paintEvent(QPaintEvent* const event) {
    editor->lineNumberAreaPaintEvent(event);
}

CodeViewer::CodeViewer(QWidget* const parent) :
    QPlainTextEdit(parent),
    lineNumberArea(new LineNumberArea(this)) {
    connect(
        this,
        &QPlainTextEdit::blockCountChanged,
        this,
        &CodeViewer::updateLineNumberAreaWidth
    );

    connect(
        this,
        &QPlainTextEdit::updateRequest,
        this,
        &CodeViewer::updateLineNumberArea
    );

    updateLineNumberAreaWidth(0);
}

[[nodiscard]] auto CodeViewer::lineNumberAreaWidth() const -> i32 {
    i32 digits = 1;
    i32 maxBlocks = max(1, blockCount());

    while (maxBlocks >= 10) {
        maxBlocks /= 10;
        digits++;
    }

    return (fontMetrics().horizontalAdvance(u'9') * digits) + 8;
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
    u32 blockNumber = block.blockNumber();
    u32 top =
        qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    u32 bottom = top + qRound(blockBoundingRect(block).height());

    painter.setFont(font());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QL1SV(itos(blockNumber + 1).data());
            painter.setPen(palette().color(QPalette::PlaceholderText));
            painter.drawText(
                0,
                i32(top),
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
    lineNumberArea->setGeometry(
        QRect(rect.left(), rect.top(), lineNumberAreaWidth(), rect.height())
    );
}

void CodeViewer::updateLineNumberAreaWidth(const i32 /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeViewer::updateLineNumberArea(const QRect& rect, const i32 dy) {
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea
            ->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
TreeSitterHighlighter::~TreeSitterHighlighter() {
    if (highlights.data() != nullptr) {
        rpgm_buffer_free(
            ByteBuffer{ .ptr = ras<const u8*>(highlights.data()),
                        .len = u32(highlights.size()) }
        );
    }
}

void TreeSitterHighlighter::setHighlights(
    const span<const HighlightToken> newHighlights
) {
    if (highlights.data() != nullptr) {
        rpgm_buffer_free(
            ByteBuffer{ .ptr = ras<const u8*>(highlights.data()),
                        .len = u32(highlights.size()) }
        );
    }

    highlights = newHighlights;
    rehighlight();
}

void TreeSitterHighlighter::highlightBlock(const QString& text) {
    if (highlights.empty()) {
        return;
    }

    const u32 blockStart = currentBlock().position();
    const u32 blockEnd = blockStart + text.size();

    QTextCharFormat fmt;

    auto iter = std::lower_bound(
        highlights.begin(),
        highlights.end(),
        blockStart,
        [](const HighlightToken& token, const u32 pos) -> bool {
        return token.end_utf16 < pos;
    }
    );

    for (; iter != highlights.end(); iter++) {
        const HighlightToken token = *iter;

        if (token.start_utf16 >= blockEnd) {
            break;  // no more relevant tokens
        }

        const u32 relStart = max(token.start_utf16, blockStart) - blockStart;
        const u32 relEnd = min(token.end_utf16, blockEnd) - blockStart;

        fmt.setForeground(highlightColor(token.highlight_type));

        if (relEnd > relStart) {
            setFormat(i32(relStart), i32(relEnd - relStart), fmt);
        }
    }
}

auto TreeSitterHighlighter::highlightColor(const HighlightType type) -> QColor {
    // TODO
#if false
    const bool dark =
        qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;

    const auto keyword = dark ? qRgb(0xff, 0x7b, 0x72) : qRgb(0xcf, 0x22, 0x2e);
    const auto string_ = dark ? qRgb(0xa5, 0xd6, 0xff) : qRgb(0x0a, 0x30, 0x69);
    const auto jsonKey = dark ? qRgb(0x7e, 0xe7, 0x87) : qRgb(0x11, 0x63, 0x29);
    const auto constant =
        dark ? qRgb(0x79, 0xc0, 0xff) : qRgb(0x05, 0x50, 0xae);
    const auto entityName =
        dark ? qRgb(0xff, 0xa6, 0x57) : qRgb(0x95, 0x38, 0x00);
    const auto funcName =
        dark ? qRgb(0xd2, 0xa8, 0xff) : qRgb(0x82, 0x50, 0xdf);
    const auto comment = dark ? qRgb(0x8b, 0x94, 0x9e) : qRgb(0x6e, 0x77, 0x81);
    const auto foreground =
        dark ? qRgb(0xe6, 0xed, 0xf3) : qRgb(0x1f, 0x23, 0x28);

    switch (name) {
        case HighlightType::Keyword:
            return keyword;
        case HighlightType::String:
        case HighlightType::StringSpecial:
            return string_;
        case HighlightType::StringSpecialKey:
            return jsonKey;
        case HighlightType::Number:
        case HighlightType::ConstantBuiltin:
        case HighlightType::Constant:
        case HighlightType::VariableBuiltin:
        case HighlightType::Property:
        case HighlightType::FunctionBuiltin:
            return constant;
        case HighlightType::Function:
        case HighlightType::FunctionMethod:
            return funcName;
        case HighlightType::Constructor:
            return entityName;
        case HighlightType::Comment:
            return comment;
        case HighlightType::Variable:
        case HighlightType::Operator:
        case HighlightType::PunctuationBracket:
        case HighlightType::PunctuationDelimiter:
            return foreground;
    }
#endif

    // One Dark
    switch (type) {
        case HighlightType::Keyword:
            return qRgb(0xC6, 0x78, 0xDD);
        case HighlightType::String:
        case HighlightType::StringSpecial:
            return qRgb(0x98, 0xC3, 0x79);
        case HighlightType::StringSpecialKey:
            return qRgb(0xE0, 0x6C, 0x75);
        case HighlightType::Number:
            return qRgb(0xD1, 0x9A, 0x66);
        case HighlightType::Comment:
            return qRgb(0x5C, 0x63, 0x70);
        case HighlightType::Variable:
            return qRgb(0xAB, 0xB2, 0xBF);
        case HighlightType::VariableBuiltin:
            return qRgb(0xE5, 0xC0, 0x7B);
        case HighlightType::Property:
            return qRgb(0xE0, 0x6C, 0x75);
        case HighlightType::Function:
        case HighlightType::FunctionMethod:
            return qRgb(0x61, 0xAF, 0xEF);
        case HighlightType::FunctionBuiltin:
            return qRgb(0x56, 0xB6, 0xC2);
        case HighlightType::Constructor:
            return qRgb(0xE5, 0xC0, 0x7B);
        case HighlightType::Constant:
        case HighlightType::ConstantBuiltin:
            return qRgb(0xD1, 0x9A, 0x66);
        case HighlightType::Operator:
        case HighlightType::PunctuationBracket:
        case HighlightType::PunctuationDelimiter:
            return qRgb(0xAB, 0xB2, 0xBF);
    }
}
#endif