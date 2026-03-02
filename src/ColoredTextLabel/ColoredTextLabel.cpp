#include "ColoredTextLabel.hpp"

#include "Utils.hpp"

#include <QColor>
#include <QPainter>
#include <QResizeEvent>
#include <QTextOption>

ColoredTextLabel::ColoredTextLabel(QWidget* const parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    setAttribute(Qt::WA_Hover, false);
}

void ColoredTextLabel::setData(
    const QStringView text,
    const vector<Span>& spans,
    const QColor& color
) {
    text_ = qsvReplace(text, '\n', QChar(0x2028));
    spans_ = spans;
    spanColor_ = color;

    dirtyLayout = true;
    updateGeometry();
    update();
}

void ColoredTextLabel::setText(
    const QStringView text,
    const vector<Span>& spans
) {
    text_ = qsvReplace(text, '\n', QChar(0x2028));
    spans_ = spans;

    dirtyLayout = true;
    updateGeometry();
    update();
}

auto ColoredTextLabel::text() const -> const QString& {
    return text_;
}

auto ColoredTextLabel::hasHeightForWidth() const -> bool {
    return true;
}

auto ColoredTextLabel::heightForWidth(const i32 width) const -> i32 {
    const QMargins margins = contentsMargins();
    const i32 availableWidth = width - margins.left() - margins.right();

    ensureLayout(std::max(0, availableWidth));
    return i32(layoutSize_.height()) + margins.top() + margins.bottom();
}

auto ColoredTextLabel::sizeHint() const -> QSize {
    const auto fontMetrics = QFontMetrics(font());
    const QMargins margins = contentsMargins();
    const i32 singleLineWidth = fontMetrics.horizontalAdvance(text_);

    const i32 estimatedWidth = std::min(singleLineWidth, 400);
    const i32 height =
        heightForWidth(estimatedWidth + margins.left() + margins.right());

    return { singleLineWidth + margins.left() + margins.right(), height };
}

auto ColoredTextLabel::minimumSizeHint() const -> QSize {
    const auto fontMetrics = QFontMetrics(font());
    const QMargins margins = contentsMargins();
    return { fontMetrics.averageCharWidth() + margins.left() + margins.right(),
             fontMetrics.height() + margins.top() + margins.bottom() };
}

void ColoredTextLabel::paintEvent(QPaintEvent* const /* event */) {
    auto painter = QPainter(this);

    if (autoFillBackground()) {
        painter.fillRect(rect(), palette().brush(QPalette::Window));
    }

    const QRect rect = contentsRect();
    ensureLayout(rect.width());

    f32 yOffset = 0.0F;

    painter.setPen(palette().color(QPalette::WindowText));
    painter.setFont(font());

    painter.save();
    painter.translate(rect.left(), f32(rect.top()) + yOffset);
    layout_.draw(&painter, QPointF(0, 0));
    painter.restore();
}

void ColoredTextLabel::resizeEvent(QResizeEvent* const event) {
    QWidget::resizeEvent(event);
    dirtyLayout = true;
    updateGeometry();
    update();
}

void ColoredTextLabel::ensureLayout(const i32 availableWidth) const {
    if (!dirtyLayout && cachedWidth_ == availableWidth) {
        return;
    }

    cachedWidth_ = std::max(0, availableWidth);

    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout_.setTextOption(opt);

    QList<QTextLayout::FormatRange> formatRanges;
    formatRanges.reserve(i32(spans_.size()));

    QTextCharFormat fmt;
    fmt.setForeground(spanColor_);

    for (const auto span : spans_) {
        formatRanges.emplace_back(span.start, span.len, fmt);
    }

    layout_.setFormats(formatRanges);
    layout_.clearLayout();
    layout_.beginLayout();

    f32 y = 0.0F;
    f32 maxW = 0.0F;

    while (true) {
        QTextLine line = layout_.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(f32(cachedWidth_));

        f32 x = 0.0F;
        const f32 naturalW = f32(line.naturalTextWidth());

        line.setPosition(QPointF(x, y));
        y += f32(line.height());
        maxW = std::max(maxW, naturalW);
    }

    layout_.endLayout();

    layoutSize_ = QSizeF(std::min<f32>(maxW, f32(cachedWidth_)), y);
    dirtyLayout = false;
}
