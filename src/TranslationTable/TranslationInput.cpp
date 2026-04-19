#include "TranslationInput.hpp"

#include "ProjectSettings.hpp"

#include <QAbstractItemView>
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>

static constexpr u16 TOOLTIP_DELAY_MS = 500;

TranslationInput::TranslationInput(
    const ProjectSettings* projectSettings,
    QWidget* const parent
) :
    QPlainTextEdit(parent),
    projectSettings(projectSettings) {
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    tooltipDelayTimer.setSingleShot(true);
    tooltipDelayTimer.setInterval(TOOLTIP_DELAY_MS);

    connect(
        &tooltipDelayTimer,
        &QTimer::timeout,
        this,
        &TranslationInput::showPendingTooltip
    );

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
    const u16 lineHeight = fontMetrics.lineSpacing();
    const u32 lineCount = document()->lineCount();
    const u32 contentHeight = (lineCount * lineHeight) + 10;

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

    // TODO: Make those optional
    constexpr array<std::pair<QL1SV, QStringView>, 5> replacements = {
        std::pair{ "<<"_L1, u"«" },
        { ">>"_L1, u"»" },
        { "--"_L1, u"—" },
        { ",,"_L1, u"„" },
        { "''"_L1, u"\u201C" }
    };

    for (const auto& pair : replacements) {
        const QL1SV source = pair.first;
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

    if (projectSettings->lineLengthHint == 0) {
        return;
    }

    auto painter = QPainter(viewport());
    painter.setPen(QColor(255, 0, 0, 80));

    const i32 charWidth = fontMetrics().horizontalAdvance(u' ');

    const i32 xPos = contentOffset().x() + document()->documentMargin() +
                     (charWidth * projectSettings->lineLengthHint);

    painter.drawLine(xPos, 0, xPos, viewport()->height());
}

void TranslationInput::mouseMoveEvent(QMouseEvent* const event) {
    const QTextCursor cursor = cursorForPosition(event->pos());
    const QTextBlock block = cursor.block();
    const QTextLayout* const layout = block.layout();

    const u32 posInBlock = cursor.position() - block.position();
    const u32 absPos = cursor.position();

    if (hoveredRangeStart != -1 && absPos >= hoveredRangeStart &&
        absPos < hoveredRangeEnd) {
        QPlainTextEdit::mouseMoveEvent(event);
        return;
    }

    tooltipDelayTimer.stop();
    tooltip.hide();
    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;
    pendingTooltipText = QString();
    pendingTooltipCaptured = QString();

    for (const QTextLayout::FormatRange& range : layout->formats()) {
        if (posInBlock >= range.start &&
            posInBlock < range.start + range.length) {
            const QString tip = range.format.toolTip();
            const QString captured = range.format.property(69).toString();

            if (!tip.isEmpty()) {
                const i32 blockStart = block.position();
                hoveredRangeStart = blockStart + range.start;
                hoveredRangeEnd = blockStart + range.start + range.length;

                auto rangeCursor = QTextCursor(document());
                rangeCursor.setPosition(hoveredRangeStart);
                const QRect charRect = cursorRect(rangeCursor);

                const auto localPos = QPoint(
                    charRect.left(),
                    charRect.top() - (fontMetrics().lineSpacing() * 2)
                );
                pendingTooltipPos = viewport()->mapToGlobal(localPos);
                pendingTooltipText = tip;
                pendingTooltipCaptured = captured;

                tooltipDelayTimer.start();
                break;
            }
        }
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void TranslationInput::leaveEvent(QEvent* const event) {
    tooltipDelayTimer.stop();
    tooltip.hide();
    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;
    pendingTooltipText.clear();

    QPlainTextEdit::leaveEvent(event);
}

void TranslationInput::showPendingTooltip() {
    if (!pendingTooltipText.isEmpty()) {
        tooltip.showAt(
            pendingTooltipPos,
            pendingTooltipText,
            pendingTooltipCaptured,
            projectSettings
        );
    }
}