#include "GitCommitList.hpp"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr f32 HASH_FONT_SCALE = 0.82F;
constexpr f32 SUBTITLE_FONT_SCALE = 0.85F;

constexpr i32 ROW_HEIGHT = 58;
constexpr i32 PADDING_HORIZONTAL = 12;
constexpr i32 PADDING_VERTICAL = 6;
constexpr i32 CIRCLE_RADIUS = 6;
constexpr i32 CIRCLE_TO_TEXT = 10;
constexpr i32 HASH_MIN_WIDTH = 56;

constexpr QRgb COLOR_REMOTE = 0xFF4A90D9;
constexpr QRgb COLOR_LOCAL = 0xFF3DBE6E;

constexpr i32 HOVER_DELAY_MS = 600;
}  // namespace

GitCommitModel::GitCommitModel(QObject* const parent) : QAbstractListModel(parent) {}

auto GitCommitModel::rowCount(const QModelIndex& /* parent */) const -> i32 {
    return scast<i32>(commits.size());
}

auto GitCommitModel::data(const QModelIndex& index, i32 role) const -> QVariant {
    if (!index.isValid()) {
        return {};
    }

    const GitCommit& commit = commits.at(index.row());

    switch (role) {
        case Qt::DisplayRole:
        case Role::Message:
            return commit.message;
        case Role::Hash:
            return commit.hash;
        case Role::Author:
            return commit.author;
        case Role::IsRemote:
            return commit.isRemote;
        default:
            return {};
    }
}

void GitCommitModel::prependCommit(GitCommit commit) {
    beginInsertRows({}, 0, 0);
    commits.push_front(std::move(commit));
    endInsertRows();
}

void GitCommitModel::appendCommit(GitCommit commit) {
    const i32 row = scast<i32>(commits.size());
    beginInsertRows({}, row, row);
    commits.push_back(std::move(commit));
    endInsertRows();
}

void GitCommitModel::clear() {
    beginResetModel();
    commits.clear();
    endResetModel();
}

auto GitCommitModel::commitAt(const i32 row) const -> const GitCommit& {
    return commits.at(row);
}

GitCommitDelegate::GitCommitDelegate(QObject* const parent) : QStyledItemDelegate(parent) {}

void GitCommitDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    const bool isSelected = (option.state & QStyle::State_Selected) != 0;
    const bool isHovered = (option.state & QStyle::State_MouseOver) != 0;

    QColor bgColor;
    if (isSelected) {
        bgColor = option.palette.highlight().color();
    } else if (isHovered) {
        bgColor = option.palette.alternateBase().color();
    } else {
        bgColor = option.palette.base().color();
    }

    painter->fillRect(option.rect, bgColor);

    const bool isRemote = index.data(Role::IsRemote).toBool();
    const QString message = index.data(Role::Message).toString();
    const QString author = index.data(Role::Author).toString();

    const QString hash = index.data(Role::Hash).toString().left(7);

    const QRect& rect = option.rect;

    const i32 circleX = rect.left() + PADDING_HORIZONTAL + CIRCLE_RADIUS;
    const i32 circleY = rect.center().y();

    const QColor circleColor = isRemote ? QColor(COLOR_REMOTE) : QColor(COLOR_LOCAL);
    painter->setPen(Qt::NoPen);
    painter->setBrush(circleColor);
    painter->drawEllipse(QPoint(circleX, circleY), CIRCLE_RADIUS, CIRCLE_RADIUS);

    const QColor primaryColor = isSelected ? option.palette.highlightedText().color() : option.palette.text().color();

    const QColor secondaryColor =
        isSelected ? option.palette.highlightedText().color().darker(140) : option.palette.placeholderText().color();

    const i32 textLeft = circleX + CIRCLE_RADIUS + CIRCLE_TO_TEXT;
    const i32 textRight = rect.right() - PADDING_HORIZONTAL;
    const i32 halfHeight = rect.height() / 2;

    const i32 hashRight = textRight;
    const i32 hashLeft = hashRight - HASH_MIN_WIDTH;

    const i32 msgRight = hashLeft - PADDING_HORIZONTAL;
    const i32 msgWidth = msgRight - textLeft;

    QFont msgFont = option.font;
    msgFont.setWeight(QFont::Medium);
    painter->setFont(msgFont);
    painter->setPen(primaryColor);

    const QRect msgRect(textLeft, rect.top() + PADDING_VERTICAL, msgWidth, halfHeight - PADDING_VERTICAL);

    const QString elidedMsg = QFontMetrics(msgFont).elidedText(message, Qt::ElideRight, msgWidth);
    painter->drawText(msgRect, Qt::AlignVCenter | Qt::AlignLeft, elidedMsg);

    QFont hashFont = option.font;
    hashFont.setFamily(u"monospace"_s);
    hashFont.setPointSizeF(option.font.pointSizeF() * HASH_FONT_SCALE);
    painter->setFont(hashFont);
    painter->setPen(secondaryColor);

    const QRect hashRect(hashLeft, rect.top() + PADDING_VERTICAL, HASH_MIN_WIDTH, halfHeight - PADDING_VERTICAL);
    painter->drawText(hashRect, Qt::AlignVCenter | Qt::AlignRight, hash);

    QFont subFont = option.font;
    subFont.setPointSizeF(option.font.pointSizeF() * SUBTITLE_FONT_SCALE);
    painter->setFont(subFont);
    painter->setPen(secondaryColor);

    const i32 subWidth = textRight - textLeft;
    const QRect subRect(textLeft, rect.top() + halfHeight, subWidth, halfHeight - PADDING_VERTICAL);

    const QString elidedAuthor = QFontMetrics(subFont).elidedText(author, Qt::ElideRight, subWidth);
    painter->drawText(subRect, Qt::AlignVCenter | Qt::AlignLeft, elidedAuthor);

    painter->setPen(QPen(option.palette.mid().color(), 1));
    painter
        ->drawLine(rect.left() + PADDING_HORIZONTAL, rect.bottom(), rect.right() - PADDING_HORIZONTAL, rect.bottom());

    painter->restore();
}

auto GitCommitDelegate::sizeHint(
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& /* index */
) const -> QSize {
    return { 0, ROW_HEIGHT };
}

GitCommitList::GitCommitList(QWidget* const parent) : QListView(parent), model_(new GitCommitModel(this)) {
    setModel(model_);
    setItemDelegate(new GitCommitDelegate(this));

    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);

    setAttribute(Qt::WA_Hover, true);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    hoverTimer.setSingleShot(true);
    hoverTimer.setInterval(HOVER_DELAY_MS);

    connect(&hoverTimer, &QTimer::timeout, this, [this] -> void {
        if (pendingIndex.isValid()) {
            emit commitHovered(QModelIndex(pendingIndex));
        }
    });
}

void GitCommitList::mouseMoveEvent(QMouseEvent* const event) {
    QListView::mouseMoveEvent(event);

    const QModelIndex idx = indexAt(event->pos());

    if (idx == hoveredIndex) {
        return;
    }

    hoveredIndex = idx;
    hoverTimer.stop();

    if (idx.isValid()) {
        pendingIndex = QPersistentModelIndex(idx);
        hoverTimer.start();
    } else {
        pendingIndex = QPersistentModelIndex();
    }
}

void GitCommitList::leaveEvent(QEvent* const event) {
    QListView::leaveEvent(event);
    hoverTimer.stop();
    hoveredIndex = {};
    pendingIndex = QPersistentModelIndex();
}