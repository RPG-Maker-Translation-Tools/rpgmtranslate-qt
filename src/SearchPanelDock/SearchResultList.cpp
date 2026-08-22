#include "SearchResultList.hpp"

#include <QPainter>
#include <QTextLayout>
#include <QTimer>

namespace {
constexpr f32 HIGHLIGHT_ALPHA = 0.85F;
constexpr i32 LAYOUT_BATCH_SIZE = 200;
constexpr i32 PAD_X = 8;
constexpr i32 PAD_Y = 8;
constexpr i32 GAP = 4;
}  // namespace

auto SearchResultListDelegate::computeHeight(
    const QStyleOptionViewItem& option,
    const QModelIndex& index,
    const i32 contentWidth
) -> i32 {
    const QString title = index.data(SearchResultListRole::TitleRole).toString();
    const QString subtitle = index.data(SearchResultListRole::SubtitleRole).toString();

    const auto fontMetrics = QFontMetrics(option.font);

    const i32 titleHeight =
        fontMetrics.boundingRect(QRect(0, 0, contentWidth, 1 << 30), Qt::TextWordWrap, title).height();

    const i32 subtitleHeigth =
        fontMetrics.boundingRect(QRect(0, 0, contentWidth, 1 << 30), Qt::TextWordWrap, subtitle).height();

    return titleHeight + GAP + subtitleHeigth + (PAD_Y * 2);
}

auto SearchResultListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize {
    const i32 width = option.rect.width();
    const i32 contentW = width - (PAD_X * 2);

    if (contentW != cachedWidth_) {
        heightCache_.clear();
        cachedWidth_ = contentW;
    }

    const i32 sourceRow = proxy_->mapToSource(index).row();

    if (const auto item = heightCache_.find(sourceRow); item != heightCache_.end()) {
        return { width, item->second };
    }

    const i32 height = computeHeight(option, index, contentW);
    heightCache_.insert({ sourceRow, height });
    return { width, height };
}

void SearchResultListDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    painter->save();
    painter->setClipRect(option.rect);

    const QRect rect = option.rect.adjusted(PAD_X, PAD_Y, -PAD_X, -PAD_Y);

    const QString title = index.data(SearchResultListRole::TitleRole).toString();
    const QString subtitle = index.data(SearchResultListRole::SubtitleRole).toString();

    const QColor titleColor = option.palette.color(QPalette::Active, QPalette::Text);

    const QColor subtitleColor = titleColor.darker(135);

    QTextCharFormat highlightedFormat;
    QColor highlightedBG = option.palette.color(QPalette::Highlight);
    highlightedBG.setAlphaF(HIGHLIGHT_ALPHA);
    highlightedFormat.setBackground(highlightedBG);

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setAlignment(Qt::AlignLeft);

    auto textLayout = QTextLayout(title, option.font);
    textLayout.setTextOption(textOption);

    const auto spans = index.data(SearchResultListRole::SpansRole).value<span<const TextMatch>>();

    QList<QTextLayout::FormatRange> formatRanges;
    formatRanges.reserve(scast<isize>(spans.size()));
    for (const TextMatch span : spans) {
        formatRanges.emplace_back(span.start(), span.len(), highlightedFormat);
    }
    textLayout.setFormats(formatRanges);

    textLayout.beginLayout();
    f32 yPos = 0.0F;

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(rect.width());
        line.setPosition(QPointF(0.0, yPos));
        yPos += scast<f32>(line.height());
    }

    textLayout.endLayout();

    painter->setPen(titleColor);
    textLayout.draw(painter, QPointF(rect.left(), rect.top()));

    painter->setFont(option.font);
    painter->setPen(subtitleColor);

    const i32 subTop = rect.top() + scast<i32>(std::ceil(yPos)) + GAP;
    const QRect subRect(rect.left(), subTop, rect.width(), option.rect.bottom() - subTop - PAD_Y);

    painter->drawText(subRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, subtitle);

    painter->restore();
}

SearchResultList::SearchResultList(QWidget* const parent) :
    QListView(parent),
    delegate_(new SearchResultListDelegate(this)),
    model_(new SearchResultListModel(this)),
    proxy_(new SearchResultListProxy(this)) {
    proxy_->setSourceModel(model_);
    setModel(proxy_);
    setItemDelegate(delegate_);
    delegate_->setProxy(proxy_);

    connect(model_, &QAbstractItemModel::modelReset, delegate_, &SearchResultListDelegate::invalidateCache);

    setSelectionMode(QListView::NoSelection);
    setEditTriggers(QListView::NoEditTriggers);
    setDragEnabled(false);
    setAcceptDrops(false);
    setDropIndicatorShown(false);
    setTabKeyNavigation(false);
    setFocusPolicy(Qt::NoFocus);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(false);
    setLayoutMode(QListView::Batched);
    setBatchSize(LAYOUT_BATCH_SIZE);
    setResizeMode(QListView::Adjust);

    viewport()->setCursor(Qt::PointingHandCursor);

    relayoutTimer.setSingleShot(true);
    relayoutTimer.setInterval(0);

    connect(&relayoutTimer, &QTimer::timeout, this, [this] -> void {
        doItemsLayout();
        viewport()->update();
    });
}

void SearchResultList::resizeEvent(QResizeEvent* const event) {
    QListView::resizeEvent(event);
    relayoutTimer.start();
}

void SearchResultList::mousePressEvent(QMouseEvent* const event) {
    const QModelIndex idx = indexAt(event->pos());

    if (idx.isValid()) {
        emit clicked(event->button(), idx);
    }

    QListView::mousePressEvent(event);
}

void SearchResultListModel::setItems(vector<SearchResultListItem>&& items) {
    beginResetModel();
    this->items = std::move(items);
    endResetModel();
}

[[nodiscard]] auto SearchResultListModel::flags(const QModelIndex& idx) const -> Qt::ItemFlags {
    if (!idx.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled;
}

[[nodiscard]] auto SearchResultListModel::data(const QModelIndex& idx, const i32 role) const -> QVariant {
    const SearchResultListItem& item = items[idx.row()];

    switch (role) {
        case SearchResultListRole::TitleRole:
            return item.title;
        case SearchResultListRole::SubtitleRole:
            return item.subtitle;
        case SearchResultListRole::SpansRole:
            return QVariant::fromValue(std::span<const TextMatch>(item.cellMatch.matches, item.cellMatch.matchesCount));
        default:
            return {};
    }
}

[[nodiscard]] auto SearchResultListModel::rowCount(const QModelIndex& parent) const -> i32 {
    return scast<i32>(parent.isValid() ? 0 : items.size());
}
