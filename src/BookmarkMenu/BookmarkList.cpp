#include "BookmarkList.hpp"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

constexpr static i32 PADDING_X = 8;
constexpr static i32 PADDING_Y = 8;
constexpr static i32 GAP = 4;

BookmarkList::BookmarkList(QWidget* const parent) :
    QListView(parent),
    model_(new BookmarkListModel(this)),
    proxy(new BookmarkProxy(this)),
    delegate(new BookmarkListDelegate(this)) {
    proxy->setSourceModel(model_);
    setModel(proxy);
    setItemDelegate(delegate);

    setSelectionMode(QListView::NoSelection);
    setEditTriggers(QListView::NoEditTriggers);
    setDragEnabled(false);
    setAcceptDrops(false);
    setDropIndicatorShown(false);
    setTabKeyNavigation(false);
    setFocusPolicy(Qt::NoFocus);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeMode(QListView::Fixed);
    setMovement(QListView::Static);
    setUniformItemSizes(true);

    viewport()->setCursor(Qt::PointingHandCursor);
}

[[nodiscard]] auto BookmarkList::bookmark(const u32 row) const -> Bookmark& {
    return model_->bookmark(row);
}

[[nodiscard]] auto BookmarkList::rowCount() const -> u32 {
    return model_->rowCount();
}

void BookmarkList::appendRow(
    const QStringView description,
    const QStringView file,
    const u32 row
) const {
    model_->appendRow(description, file, row);
}

void BookmarkList::removeRow(const u32 row) const {
    model_->removeRow(row);
}

void BookmarkList::setRowHidden(const u32 row, const bool hidden) const {
    model_->bookmark(row).hidden = hidden;
}

void BookmarkList::clear() const {
    model_->clear();
}

void BookmarkList::refilter() const {
    proxy->invalidate();
}

void BookmarkList::mousePressEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        const QModelIndex index = indexAt(event->pos());

        if (!index.isValid()) {
            return;
        }

        const Bookmark& bookmark = model_->bookmark(index.row());
        emit bookmarkClicked(
            QLatin1StringView(bookmark.filename.data()),
            bookmark.row
        );
    }
}

[[nodiscard]] auto BookmarkListModel::data(
    const QModelIndex& idx,
    const i32 role
) const -> QVariant {
    return {};
}

[[nodiscard]] auto BookmarkListModel::bookmark(const u32 row) -> Bookmark& {
    return bookmarks[row];
}

[[nodiscard]] auto BookmarkListModel::rowCount(const QModelIndex& parent) const
    -> i32 {
    return bookmarks.size();
}

[[nodiscard]] auto BookmarkListModel::flags(const QModelIndex& idx) const
    -> Qt::ItemFlags {
    return Qt::NoItemFlags;
}

void BookmarkListModel::removeRow(const u32 row) {
    beginRemoveRows(QModelIndex(), row, row);
    bookmarks.removeAt(row);
    endRemoveRows();
}

void BookmarkListModel::appendRow(
    const QStringView file,
    const QStringView description,
    const u32 row
) {
    array<char, 13> filename;
    const auto filenameUtf8 = file.toUtf8();

    memcpy(filename.data(), filenameUtf8.constData(), filenameUtf8.size());
    filename[filenameUtf8.size()] = '\0';

    const i32 pos = rowCount();
    beginInsertRows(QModelIndex(), pos, pos);
    bookmarks.emplaceBack(description.toString(), filename, row, false);
    endInsertRows();
}

void BookmarkListModel::clear() {
    beginResetModel();
    bookmarks = {};
    endResetModel();
}

[[nodiscard]] auto BookmarkProxy::filterAcceptsRow(
    const i32 row,
    const QModelIndex& /* parent */
) const -> bool {
    auto* const model = as<BookmarkListModel*>(sourceModel());
    const auto& item = model->bookmark(row);
    return !item.hidden;
}

void BookmarkListDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const {
    if (!index.isValid()) {
        return;
    }

    const auto* const view = as<const BookmarkList*>(opt.widget);
    const auto* const proxy = as<const BookmarkProxy*>(view->model());
    auto* const src = as<BookmarkListModel*>(proxy->sourceModel());

    const i32 sourceRow = proxy->mapToSource(index).row();
    const Bookmark& bookmark = src->bookmark(sourceRow);

    const QString& title = bookmark.description;
    const QString subtitle =
        tr("Row %1 / File %2")
            .arg(bookmark.row + 1)
            .arg(QLatin1StringView(bookmark.filename.data()));

    painter->save();
    painter->setClipRect(opt.rect);

    QStyle* const style = opt.widget->style();
    style->drawPrimitive(
        QStyle::PE_PanelItemViewItem,
        &opt,
        painter,
        opt.widget
    );

    const QRect rect =
        opt.rect.adjusted(PADDING_X, PADDING_Y, -PADDING_X, -PADDING_Y);

    const auto fontMetrics = QFontMetrics(opt.font);
    const i32 fontHeight = fontMetrics.height();

    QRect titleRect = rect;
    titleRect.setHeight(fontHeight);

    QRect subtitleRect = rect;
    subtitleRect.setTop(titleRect.bottom() + GAP);
    subtitleRect.setHeight(fontHeight);

    const QColor titleColor =
        opt.palette.color(QPalette::Active, QPalette::Text);

    const QColor subtitleColor = titleColor.darker(125);

    painter->setPen(titleColor);
    painter->drawText(
        titleRect,
        Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
        fontMetrics.elidedText(title, Qt::ElideRight, titleRect.width())
    );

    painter->setPen(subtitleColor);
    painter->drawText(
        subtitleRect,
        Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
        fontMetrics.elidedText(subtitle, Qt::ElideRight, subtitleRect.width())
    );

    painter->restore();
}

[[nodiscard]] auto BookmarkListDelegate::sizeHint(
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const -> QSize {
    const auto fontMentrics = QFontMetrics(opt.font);

    const i32 height = (PADDING_Y * 2) + (fontMentrics.height() * 2) + GAP;
    return { opt.rect.width() + (PADDING_X * 2), height };
}
