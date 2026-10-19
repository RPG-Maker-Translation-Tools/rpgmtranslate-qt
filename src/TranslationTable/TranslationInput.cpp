#include "TranslationInput.hpp"

#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"

#include <QDesktopServices>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCursor>

namespace {
constexpr QColor LINE_LENGTH_HINT_COLOR = QColor(UINT8_MAX, 0, 0, 80);
constexpr QColor LINE_LENGTH_HINT_OVERFLOW_COLOR = QColor(UINT8_MAX, 0, 0, 200);

// RPG Maker MV/MZ message font size: default, step and clamp bounds of `\{` / `\}`.
constexpr f64 DEFAULT_GAME_FONT_SIZE = 28;
constexpr f64 FONT_SIZE_STEP = 12;
constexpr f64 FONT_SIZE_MIN = 24;
constexpr f64 FONT_SIZE_MAX = 96;

// Static estimate of one character's width. Font metadata (`QFontInfo::fixedPitch()`, `averageCharWidth()`) is
// unreliable across platforms, so this averages real advances: the midpoint of printable ASCII and a fixed CJK sample
// (translators often keep CJK punctuation). Strings go through the shaping overload, so a font without CJK glyphs is
// measured through its fallback, not as zero-width.
[[nodiscard]] auto hintCharWidth(const QFont& font) -> f64 {
    static vector<pair<QFont, f64>> cache;
    constexpr usize MAX_CACHED_FONTS = 8;

    for (const auto& [cachedFont, width] : cache) {
        if (cachedFont == font) {
            return width;
        }
    }

    constexpr QStringView CJK_PUNCTUATION = u"、。，．・：；？！「」『』（）【】〜…";
    constexpr QStringView KANJI =
        u"日本語人大年出中国上時見行思分生何者事今前後外手気力水火木金土学校先友心目口言話聞読書食飲";

    QString ascii;
    ascii.reserve(0x7E - 0x20 + 1);

    for (char16_t chr = u' '; chr <= u'~'; chr++) {
        ascii += QChar(chr);
    }

    QString cjk;
    cjk.reserve(CJK_PUNCTUATION.size() + (2 * (0x3093 - 0x3041 + 1)) + KANJI.size());
    cjk += CJK_PUNCTUATION;

    for (char16_t chr = 0x3041; chr <= 0x3093; chr++) {
        cjk += QChar(chr);
    }

    for (char16_t chr = 0x30A1; chr <= 0x30F3; chr++) {
        cjk += QChar(chr);
    }

    cjk += KANJI;

    const auto metrics = QFontMetricsF(font);
    const f64 width =
        (metrics.horizontalAdvance(ascii) / ascii.size() + metrics.horizontalAdvance(cjk) / cjk.size()) / 2;

    if (cache.size() >= MAX_CACHED_FONTS) {
        cache.clear();
    }

    cache.emplace_back(font, width);
    return width;
}

// Index of the `]` that closes the `[` at `open`, counting the brackets of nested codes (`\N[\V[3]]`); -1 when it is
// never closed.
[[nodiscard]] auto closingBracket(const QStringView text, const isize open) -> isize {
    i32 depth = 0;

    for (isize idx = open; idx < text.size(); idx++) {
        if (text[idx] == u'[') {
            depth++;
        } else if (text[idx] == u']' && --depth == 0) {
            return idx;
        }
    }

    return -1;
}

// Width of `text` as the game draws it, relative to `metrics` at the base size. `\{`, `\}`, `\fs[n]` rescale glyphs,
// `\_` is half a space, other codes draw nothing. Ignores kerning and surrogate pairs: an overflow estimate only.
[[nodiscard]] auto gameTextWidth(const QStringView text, const QFontMetricsF& metrics, const f64 baseSize) -> f64 {
    f64 width = 0;
    f64 size = baseSize;

    for (isize idx = 0; idx < text.size(); idx++) {
        const QChar chr = text[idx];

        if (chr != u'\\' || idx + 1 == text.size()) {
            width += metrics.horizontalAdvance(chr) * (size / baseSize);
            continue;
        }

        const QChar code = text[++idx];

        if (code == u'\\') {
            width += metrics.horizontalAdvance(u'\\') * (size / baseSize);
            continue;
        }

        // RPG Maker 2000/2003: a half-width space.
        if (code == u'_') {
            width += metrics.horizontalAdvance(u' ') * 0.5 * (size / baseSize);
            continue;
        }

        if (code == u'{') {
            if (size <= FONT_SIZE_MAX) {
                size += FONT_SIZE_STEP;
            }

            continue;
        }

        if (code == u'}') {
            if (size >= FONT_SIZE_MIN) {
                size -= FONT_SIZE_STEP;
            }

            continue;
        }

        const bool isFontSize = code == u'f' && idx + 1 < text.size() && text[idx + 1] == u's';

        if (isFontSize) {
            idx++;
        }

        if (idx + 1 < text.size() && text[idx + 1] == u'[') {
            const isize close = closingBracket(text, idx + 1);

            if (close != -1) {
                if (isFontSize) {
                    if (const optional<u32> value = stoa<u32>(text.sliced(idx + 2, close - idx - 2));
                        value && *value != 0) {
                        size = *value;
                    }
                }

                idx = close;
            }
        }
    }

    return width;
}
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
    connect(
        document(),
        &QTextDocument::contentsChange,
        this,
        [this](const i32 /* position */, const i32 /* removed */, const i32 added) -> void { lastCharsAdded = added; }
    );
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
    // The Enter of the numpad is a line break for `QPlainTextEdit`, but it must commit like Return
    const bool isReturn = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;

    if (event->key() == Qt::Key_Escape || isReturn) {
        if (isReturn && (event->modifiers() & Qt::ShiftModifier) != 0) {
            QTimer::singleShot(0, this, [this] -> void { verticalScrollBar()->setValue(0); });
        } else {
            if (isReturn && (event->modifiers() & Qt::ControlModifier) != 0) {
                emit focusNextInput();
            }

            clearFocus();
            event->accept();
            return;
        }
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

    // The positions of the lint under the cursor are those of the text before it changed
    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;
    shownRangeStart = -1;
    shownRangeEnd = -1;

    // Deleting text must not bring a replacement back, e.g. `...` left after erasing a fourth dot
    if (lastCharsAdded != 0) {
        performAutoReplacements();
    } else {
        lastReplacements.clear();
    }
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

void TranslationInput::replaceText(const QString& text) {
    QTextCursor cursor = textCursor();

    blockTextChanged = true;

    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();

    blockTextChanged = false;

    setTextCursor(cursor);
    updateContentHeight();
}

void TranslationInput::performAutoReplacements() {
    lastReplacements.clear();

    QTextCursor cursor = textCursor();
    const i32 originalPosition = cursor.position();
    // Copied when a replacement can match, most keystrokes have none enabled
    QString text;

    for (const auto& [source, replacement, enabled] : settings->translation.replacements) {
        if (!enabled) {
            continue;
        }

        if (originalPosition >= source.size()) {
            if (text.isNull()) {
                text = toPlainText();
            }

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

    const u16 hint = projectSettings->lineLengthHint;

    if (hint == 0) {
        return;
    }

    const i32 xPos = round(contentOffset().x() + document()->documentMargin() + (hintCharWidth(font()) * hint));

    auto painter = QPainter(viewport());
    painter.setPen(LINE_LENGTH_HINT_COLOR);
    painter.drawLine(xPos, 0, xPos, viewport()->height());

    // Overflow is a width *ratio* in the reference font (the game's font), so the editor font doesn't affect it.
    // `hintFontSize` is the game's base size, only needed to scale `\{` / `\fs[n]` relative to it.
    const QFont refFont = projectSettings->hintFont.isEmpty() ? font() : QFont(projectSettings->hintFont);
    const auto refMetrics = QFontMetricsF(refFont);
    const f64 limit = hintCharWidth(refFont) * hint;
    const f64 baseSize = projectSettings->hintFontSize == 0 ? DEFAULT_GAME_FONT_SIZE : projectSettings->hintFontSize;
    const i32 viewHeight = viewport()->height();

    painter.setPen(LINE_LENGTH_HINT_OVERFLOW_COLOR);

    for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next()) {
        const QRectF rect = blockBoundingGeometry(block).translated(contentOffset());

        if (rect.top() > viewHeight) {
            break;
        }

        if (gameTextWidth(block.text(), refMetrics, baseSize) > limit) {
            painter.drawLine(xPos, qRound(rect.top()), xPos, qRound(rect.bottom()));
        }
    }
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

    if (tooltip != nullptr) {
        tooltip->hide();
    }

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

    if (tooltip != nullptr) {
        tooltip->waitForEnter();
    }

    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;

    QPlainTextEdit::leaveEvent(event);
}

void TranslationInput::showPendingTooltip() {
    if (tooltip == nullptr) {
        tooltip = make_unique<LintTooltip>();

        // A tooltip shows text of the game and of custom lints as Markdown, guard against non-website links.
        connect(tooltip.get(), &LintTooltip::linkActivated, this, [](const QUrl& url) -> void {
            if (url.scheme() == u"http" || url.scheme() == u"https") {
                QDesktopServices::openUrl(url);
            }
        });

        connect(tooltip.get(), &LintTooltip::suggestionActivated, this, &TranslationInput::applySuggestion);
    }

    // The mouse leaves the text to reach the tooltip, which resets the hovered range, so the suggestions are applied
    // to the range copied here
    shownLints = pendingLints;
    shownRangeStart = hoveredRangeStart;
    shownRangeEnd = hoveredRangeEnd;

    tooltip->showAt(pendingCursorPos, shownLints, projectSettings);
}

void TranslationInput::applySuggestion(const LintType type, const i32 lintIndex, const i32 suggestionIndex) {
    if (shownRangeStart == -1) {
        return;
    }

    const auto entry = ranges::find_if(shownLints, [type, lintIndex](const LintEntry& lint) -> bool {
        return lint.type == type && lint.index == lintIndex;
    });

    if (entry == shownLints.end()) {
        return;
    }

    const QStringList suggestions = entry->suggestions.toStringList();

    if (suggestionIndex < 0 || suggestionIndex >= suggestions.size()) {
        return;
    }

    auto cursor = QTextCursor(document());
    cursor.setPosition(shownRangeStart);
    cursor.setPosition(shownRangeEnd, QTextCursor::KeepAnchor);
    cursor.insertText(suggestions[suggestionIndex]);
    setTextCursor(cursor);

    // The click is inside the tooltip, so it would wait for the mouse to leave
    tooltip->dismiss();
    hoveredRangeStart = -1;
    hoveredRangeEnd = -1;
    shownRangeStart = -1;
    shownRangeEnd = -1;
}
