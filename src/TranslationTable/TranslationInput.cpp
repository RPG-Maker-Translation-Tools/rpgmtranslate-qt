#include "TranslationInput.hpp"

#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"

#include <QAbstractItemView>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QPainter>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCursor>
#include <QUrlQuery>

namespace {
constexpr QColor LINE_LENGTH_HINT_COLOR = QColor(UINT8_MAX, 0, 0, 80);
}  // namespace

TranslationInput::TranslationInput(
    const ProjectSettings* const projectSettings,
    const Settings* const settings,
    QWidget* const parent
) :
    QPlainTextEdit(parent),

    highlighter(new TranslationHighlighter(document())),

    projectSettings(projectSettings),
    settings(settings) {
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    tooltipDelayTimer.setSingleShot(true);
    tooltipDelayTimer.setInterval(TOOLTIP_DELAY_MS);

    connect(&tooltipDelayTimer, &QTimer::timeout, this, &TranslationInput::showPendingTooltip);
    connect(this, &QPlainTextEdit::textChanged, this, &TranslationInput::onTextChanged);
    connect(document(), &QTextDocument::contentsChanged, this, &TranslationInput::updateContentHeight);

    connect(&tooltip, &LintTooltip::linkActivated, this, [this](const QUrl& url) -> void {
        if (url.scheme() == u"lint"_qsv && url.host() == u"apply-suggestion"_qsv) {
            const auto query = QUrlQuery(url);
            applySuggestion(
                scast<LintType>(stoa<u8>(query.queryItemValue(u"type"_s))),
                stoa<i32>(query.queryItemValue(u"lint"_s)),
                stoa<i32>(query.queryItemValue(u"index"_s))
            );
        } else {
            QDesktopServices::openUrl(url);
        }
    });
}

void TranslationInput::applyLintFormats(vector<LintCharState> charStates, const i32 size) {
    highlighter->setFormats(std::move(charStates), size);

    // rehighlight() reapplies formatting through the document, which would otherwise re-fire
    // textChanged() (Qt emits it for formatting-only changes too) and re-trigger linting.
    QMetaObject::invokeMethod(this, [this] -> void {
        const QSignalBlocker blocker(document());
        highlighter->rehighlight();
    }, Qt::QueuedConnection);
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
                const i32 replEndPos = replacement.position + scast<i32>(replacement.replacement.size());

                if (cursorPos == replEndPos) {
                    blockTextChanged = true;
                    cursor.setPosition(replacement.position);
                    cursor.setPosition(replEndPos, QTextCursor::KeepAnchor);
                    cursor.insertText(replacement.original);
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

    for (const auto& [source, replacement, enabled] : settings->translation.replacements) {
        if (!enabled) {
            continue;
        }

        if (originalPosition >= source.size()) {
            const i32 checkPos = originalPosition - scast<i32>(source.size());

            if (QStringView(text).mid(checkPos, source.size()) == source) {
                blockTextChanged = true;

                lastReplacements.emplace_back(source, replacement, checkPos);

                cursor.setPosition(checkPos);
                cursor.setPosition(originalPosition, QTextCursor::KeepAnchor);
                cursor.insertText(replacement);

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
    painter.setPen(LINE_LENGTH_HINT_COLOR);

    const i32 charWidth = fontMetrics().horizontalAdvance(u' ');

    const i32 xPos =
        scast<i32>(contentOffset().x() + document()->documentMargin() + (charWidth * projectSettings->lineLengthHint));

    painter.drawLine(xPos, 0, xPos, viewport()->height());
}

void TranslationInput::mouseMoveEvent(QMouseEvent* const event) {
    const QTextCursor cursor = cursorForPosition(event->pos());
    const QTextBlock block = cursor.block();
    const QTextLayout* const layout = block.layout();

    const i32 posInBlock = cursor.position() - block.position();
    const i32 absPos = cursor.position();

    if (hoveredRangeStart != -1 && absPos >= hoveredRangeStart && absPos < hoveredRangeEnd) {
        QPlainTextEdit::mouseMoveEvent(event);
        return;
    }

    tooltipDelayTimer.stop();
    tooltip.hide();

    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;

    for (const QTextLayout::FormatRange& range : layout->formats()) {
        if (posInBlock >= range.start && posInBlock < range.start + range.length) {
            const i32 blockStart = block.position();
            hoveredRangeStart = blockStart + range.start;
            hoveredRangeEnd = blockStart + range.start + range.length;

            auto rangeCursor = QTextCursor(document());
            rangeCursor.setPosition(hoveredRangeStart);
            const QRect charRect = cursorRect(rangeCursor);

            const auto localPos = QPoint(charRect.left(), charRect.top());

            pendingLints = range.format.property(LINT_ENTRIES_ID).value<QList<LintEntry>>();
            pendingCursorPos = viewport()->mapToGlobal(localPos);
            tooltipDelayTimer.start();
            break;
        }
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void TranslationInput::leaveEvent(QEvent* const event) {
    tooltipDelayTimer.stop();

    tooltip.waitForEnter();

    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;

    QPlainTextEdit::leaveEvent(event);
}

void TranslationInput::showPendingTooltip() {
    tooltip.showAt(pendingCursorPos, pendingLints, settings, projectSettings);
}

void TranslationInput::applySuggestion(const LintType type, const i32 lintIndex, const i32 suggestionIndex) {
    if (hoveredRangeStart == -1) {
        return;
    }

    const auto entry = ranges::find_if(pendingLints, [type, lintIndex](const LintEntry& lint) -> bool {
        return lint.type == type && lint.index == lintIndex;
    });

    if (entry == pendingLints.end()) {
        return;
    }

    const QStringList suggestions = entry->suggestions.toStringList();

    if (suggestionIndex < 0 || suggestionIndex >= suggestions.size()) {
        return;
    }

    auto cursor = QTextCursor(document());
    cursor.setPosition(hoveredRangeStart);
    cursor.setPosition(hoveredRangeEnd, QTextCursor::KeepAnchor);
    cursor.insertText(suggestions[suggestionIndex]);
    setTextCursor(cursor);

    tooltip.hide();
    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;
}