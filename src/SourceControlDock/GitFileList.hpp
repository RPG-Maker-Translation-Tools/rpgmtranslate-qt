#pragma once

#ifdef ENABLE_LIBGIT2
#include "Aliases.hpp"
#include "Utils.hpp"

#include <git2/status.h>

#include <QAbstractListModel>
#include <QAction>
#include <QApplication>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFontMetrics>
#include <QIcon>
#include <QListView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QStyledItemDelegate>

struct GitFileItem final {
    // Relative
    QString filePath;
    git_status_t status;
    bool staged = false;

    [[nodiscard]] auto fileName() const -> QStringView { return lastPathComponent(filePath); }
};

class GitFileModel final : public QAbstractListModel {
    Q_OBJECT

   public:
    enum Roles : u16 {
        GitChangeItemRole = Qt::UserRole + 1,
    };

    explicit GitFileModel(QObject* const parent = nullptr) : QAbstractListModel(parent) {}

    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const -> i32 override {
        return scast<i32>(items_.size());
    }

    [[nodiscard]] auto data(const QModelIndex& index, const i32 role = Qt::DisplayRole) const -> QVariant override {
        if (!index.isValid()) {
            return {};
        }

        if (role == GitChangeItemRole) {
            return QVariant::fromValue(items_.at(index.row()));
        }

        return {};
    }

    [[nodiscard]] auto items() const -> const QList<GitFileItem>& { return items_; }

    [[nodiscard]] auto selectedItems() const -> QList<const GitFileItem*> {
        QList<const GitFileItem*> items;
        items.reserve(4);

        for (const auto& [idx, item] : views::enumerate(items_)) {
            if (item.staged) {
                items.push_back(&item);
            }
        }

        return items;
    }

    void removeSelectedItems() {
        beginResetModel();
        items_ = ranges::to<QList>(views::filter(items_, [](const auto& item) -> bool { return !item.staged; }));
        endResetModel();
    }

    void setItems(const QList<GitFileItem>& newItems) {
        beginResetModel();
        items_ = newItems;
        endResetModel();
    }

    void appendItem(const GitFileItem& item) {
        beginInsertRows({}, scast<i32>(items_.size()), scast<i32>(items_.size()));
        items_.append(item);
        endInsertRows();
    }

    void removeItem(const i32 row) {
        beginRemoveRows({}, row, row);
        items_.removeAt(row);
        endRemoveRows();
    }

    void setStaged(const i32 row, const bool staged) {
        items_[row].staged = staged;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, { GitChangeItemRole });
    }

    void clear() {
        beginResetModel();
        items_.clear();
        endResetModel();
    }

    QString repoRootPath;

   private:
    QList<GitFileItem> items_;
};

class GitFileDelegate final : public QStyledItemDelegate {
    Q_OBJECT

    static constexpr i32 ITEM_HEIGHT = 24;
    static constexpr i32 ICON_SIZE = 16;
    static constexpr i32 H_PADDING = 4;
    static constexpr i32 ICON_TEXT_GAP = 4;
    static constexpr i32 NAME_PATH_GAP = 8;
    static constexpr i32 BADGE_RIGHT_PAD = 4;
    static constexpr i32 CHECKBOX_SIZE = 20;
    static constexpr i32 CHECKBOX_PAD = 4;
    static constexpr i32 BUTTON_SIZE = 16;
    static constexpr i32 BUTTON_GAP = 4;
    static constexpr i32 BUTTON_AREA_PAD = 6;

   public:
    explicit GitFileDelegate(GitFileModel* const model, QObject* const parent = nullptr) :
        QStyledItemDelegate(parent),
        model(model) {}

    enum class HoverButton : u8 {
        None,
        Stage,
        Revert,
        Diff
    };

    [[nodiscard]] static auto hitTestButton(const QRect& itemRect, const QPoint& pos) -> HoverButton {
        const auto [stageRect, revertRect, diffRect] = buttonRects(itemRect);

        if (stageRect.contains(pos)) {
            return HoverButton::Stage;
        }

        if (revertRect.contains(pos)) {
            return HoverButton::Revert;
        }

        if (diffRect.contains(pos)) {
            return HoverButton::Diff;
        }

        return HoverButton::None;
    }

    [[nodiscard]] auto hoveredButton() const -> HoverButton { return hoveredButton_; }

    void setHoveredButton(const HoverButton btn) { hoveredButton_ = btn; }

    [[nodiscard]] constexpr auto sizeHint(
        const QStyleOptionViewItem& /* opt */,
        const QModelIndex& /* idx */
    ) const -> QSize override {
        return { 0, ITEM_HEIGHT };
    }

    [[nodiscard]] auto hoveredRow() const -> i32 { return hoveredRow_; }

    void setHoveredRow(const i32 row) { hoveredRow_ = row; }

    void paint(QPainter* const painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyledItemDelegate::paint(painter, option, index);

        const auto item = index.data(GitFileModel::GitChangeItemRole).value<GitFileItem>();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipRect(option.rect);

        const QRect& rect = option.rect;
        const i32 midY = rect.y() + (rect.height() / 2);
        const bool selected = (option.state & QStyle::State_Selected) != 0;
        const QPalette& palette = option.palette;

        const QRect checkboxRect = checkboxHitRect(rect);

        QStyleOptionButton checkOpt;
        checkOpt.rect = checkboxRect;
        checkOpt.state = QStyle::State_Enabled;
        checkOpt.state |= item.staged ? QStyle::State_On : QStyle::State_Off;

        qApp->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOpt, painter);

        const auto [stageRect, revertRect, diffRect] = buttonRects(rect);

        if (hoveredRow_ == index.row()) {
            const auto drawHighlight = [&](const QRect& rect) -> void {
                painter->setPen(Qt::NoPen);
                painter->setBrush(palette.color(QPalette::Highlight));
                painter->drawRoundedRect(rect.adjusted(-2, -2, 2, 2), 3, 3);
            };

            if (hoveredButton_ == HoverButton::Stage) {
                drawHighlight(stageRect);
            }

            if (hoveredButton_ == HoverButton::Revert) {
                drawHighlight(revertRect);
            }

            if (hoveredButton_ == HoverButton::Diff) {
                drawHighlight(diffRect);
            }

            stageIcon_.paint(painter, stageRect);
            revertIcon_.paint(painter, revertRect);
            diffIcon_.paint(painter, diffRect);
        }

        QChar badgeLetter;
        QColor badgeColor;
        resolveBadge(item.status, badgeLetter, badgeColor);

        QFont font = option.font;
        font.setBold(true);

        const auto fontMetrics = QFontMetrics(font);
        const i32 badgeWidth = fontMetrics.horizontalAdvance(badgeLetter);
        const i32 badgeX = stageRect.x() - BUTTON_AREA_PAD - badgeWidth;

        const QColor primaryColor = selected ? palette.highlightedText().color() : palette.text().color();
        const QColor dimColor = selected ? palette.highlightedText().color() : palette.placeholderText().color();

        i32 xPos = rect.x() + H_PADDING;

        const QIcon icon = fileIcon(item.filePath);
        const QRect iconRect(xPos, midY - (ICON_SIZE / 2), ICON_SIZE, ICON_SIZE);
        icon.paint(painter, iconRect);
        xPos += ICON_SIZE + ICON_TEXT_GAP;

        const QString fileName = item.fileName().toString();
        const i32 nameWidth = fontMetrics.horizontalAdvance(fileName);
        const i32 totalTextAvail = badgeX - xPos - NAME_PATH_GAP;
        const i32 actualNameWidth = min(nameWidth, totalTextAvail);

        font.setStrikeOut(badgeLetter == u'D');
        painter->setFont(font);
        painter->setPen(primaryColor);
        painter->drawText(
            QRect(xPos, rect.y(), actualNameWidth, rect.height()),
            Qt::AlignVCenter | Qt::AlignLeft,
            fileName
        );

        xPos += actualNameWidth + NAME_PATH_GAP;

        const i32 pathAvail = badgeX - xPos;

        if (pathAvail > 0) {
            painter->setFont(font);
            painter->setPen(dimColor);
            painter->drawText(
                QRect(xPos, rect.y(), pathAvail, rect.height()),
                Qt::AlignVCenter | Qt::AlignLeft,
                item.filePath
            );
        }

        font.setStrikeOut(false);
        painter->setFont(font);
        painter->setPen(badgeColor);
        painter->drawText(
            QRect(badgeX, rect.y(), badgeWidth, rect.height()),
            Qt::AlignVCenter | Qt::AlignLeft,
            QChar(badgeLetter)
        );

        painter->restore();
    }

    auto editorEvent(
        QEvent* const event,
        QAbstractItemModel* const /* abstractModel */,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) -> bool override {
        if (event->type() != QEvent::MouseButtonRelease) {
            return false;
        }

        const auto* const mouseEvent = scast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton) {
            return false;
        }

        const auto item = index.data(GitFileModel::GitChangeItemRole).value<GitFileItem>();

        if (hoveredRow_ == index.row()) {
            const auto [stageRect, revertRect, diffRect] = buttonRects(option.rect);

            if (stageRect.contains(mouseEvent->pos())) {
                emit stageRequested(item.filePath);
                return true;
            }
            if (revertRect.contains(mouseEvent->pos())) {
                emit revertRequested(item.filePath);
                return true;
            }
            if (diffRect.contains(mouseEvent->pos())) {
                emit diffRequested(item.filePath);
                return true;
            }
        }

        const QRect hitRect = checkboxHitRect(option.rect);
        if (!hitRect.contains(mouseEvent->pos())) {
            return false;
        }

        model->setStaged(index.row(), !item.staged);

        emit staged(item.filePath);

        return true;
    }

   signals:
    void staged(const QString& path);
    void stageRequested(const QString& path);
    void revertRequested(const QString& path);
    void diffRequested(const QString& path);

   private:
    [[nodiscard]] static auto checkboxHitRect(const QRect& itemRect) -> QRect {
        const i32 midY = itemRect.y() + (itemRect.height() / 2);
        return { itemRect.right() - CHECKBOX_PAD - CHECKBOX_SIZE,
                 midY - (CHECKBOX_SIZE / 2),
                 CHECKBOX_SIZE,
                 CHECKBOX_SIZE };
    }

    [[nodiscard]] static auto buttonRects(const QRect& itemRect) -> array<QRect, 3> {
        const i32 midY = itemRect.y() + (itemRect.height() / 2);
        const i32 checkboxReserved = CHECKBOX_PAD + CHECKBOX_SIZE + CHECKBOX_PAD;

        i32 xPos = itemRect.right() - checkboxReserved - BUTTON_AREA_PAD - BUTTON_SIZE;
        const QRect diffRect(xPos, midY - (BUTTON_SIZE / 2), BUTTON_SIZE, BUTTON_SIZE);

        xPos -= (BUTTON_SIZE + BUTTON_GAP);
        const QRect revertRect(xPos, midY - (BUTTON_SIZE / 2), BUTTON_SIZE, BUTTON_SIZE);

        xPos -= (BUTTON_SIZE + BUTTON_GAP);
        const QRect stageRect(xPos, midY - (BUTTON_SIZE / 2), BUTTON_SIZE, BUTTON_SIZE);

        return { stageRect, revertRect, diffRect };
    }

    [[nodiscard]] auto fileIcon(const QString& absoluteFilePath) const -> QIcon {
        QIcon icon = iconProvider.icon(QFileInfo(absoluteFilePath));

        if (icon.isNull()) {
            icon = iconProvider.icon(QFileIconProvider::File);
        }

        return icon;
    }

    static void resolveBadge(const git_status_t status, QChar& letter, QColor& color) {
        if ((status & GIT_STATUS_CONFLICTED) != 0) {
            letter = u'!';
            color = qRgb(0xF1, 0x4C, 0x4C);
            return;
        }
        if ((status & GIT_STATUS_IGNORED) != 0) {
            letter = u'I';
            color = qRgb(0x8C, 0x8C, 0x8C);
            return;
        }
        if ((status & (GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED)) != 0) {
            letter = u'D';
            color = qRgb(0xF1, 0x4C, 0x4C);
            return;
        }
        if ((status & (GIT_STATUS_INDEX_RENAMED | GIT_STATUS_WT_RENAMED)) != 0) {
            letter = u'R';
            color = qRgb(0xE2, 0xC0, 0x8D);
            return;
        }
        if (((status & GIT_STATUS_INDEX_TYPECHANGE) != 0) || ((status & GIT_STATUS_WT_TYPECHANGE) != 0)) {
            // TODO(v1.2)
            return;
        }
        if ((status & GIT_STATUS_WT_NEW) != 0) {
            letter = u'U';
            color = qRgb(0x73, 0xC9, 0x91);
            return;
        }
        if ((status & GIT_STATUS_INDEX_NEW) != 0) {
            letter = u'A';
            color = qRgb(0x73, 0xC9, 0x91);
            return;
        }
        if ((status & (GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED)) != 0) {
            letter = u'M';
            color = qRgb(0xE2, 0xC0, 0x8D);
            return;
        }
    }

    mutable QFileIconProvider iconProvider;
    GitFileModel* model;
    i32 hoveredRow_ = -1;

    HoverButton hoveredButton_ = HoverButton::None;

    QIcon stageIcon_ = QIcon(u":/icons/add.svg"_s);
    QIcon revertIcon_ = QIcon(u":/icons/undo.svg"_s);
    QIcon diffIcon_ = QIcon(u":/icons/difference.svg"_s);
};

class GitFileList final : public QListView {
    Q_OBJECT

   public:
    explicit GitFileList(QWidget* const parent = nullptr) :
        QListView(parent),
        model_(new GitFileModel(this)),
        delegate(new GitFileDelegate(model_, this)) {
        setModel(model_);
        setItemDelegate(delegate);

        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSelectionMode(QListView::ExtendedSelection);
        setUniformItemSizes(true);
        setMouseTracking(true);
        setContextMenuPolicy(Qt::CustomContextMenu);

        connect(delegate, &GitFileDelegate::staged, this, &GitFileList::staged);
        connect(delegate, &GitFileDelegate::stageRequested, this, &GitFileList::stageRequested);
        connect(delegate, &GitFileDelegate::revertRequested, this, &GitFileList::revertRequested);
        connect(delegate, &GitFileDelegate::diffRequested, this, &GitFileList::diffRequested);
        connect(this, &QListView::customContextMenuRequested, this, &GitFileList::showContextMenu);
    }

    [[nodiscard]] auto model() const -> GitFileModel* { return model_; }

    void appendItem(const GitFileItem& item) { model_->appendItem(item); }

    [[nodiscard]] auto items() const -> const QList<GitFileItem>& { return model_->items(); }

    [[nodiscard]] auto selectedItems() const -> QList<const GitFileItem*> { return model_->selectedItems(); }

    void removeSelectedItems() { model_->removeSelectedItems(); }

    void clear() { model_->clear(); }

   signals:
    void staged(const QString& path);
    void stageRequested(const QString& path);
    void revertRequested(const QString& path);
    void diffRequested(const QString& path);
    void stageMultipleRequested(const QStringList& paths);
    void revertMultipleRequested(const QStringList& paths);

   protected:
    void mouseMoveEvent(QMouseEvent* const event) override {
        QListView::mouseMoveEvent(event);

        const QModelIndex idx = indexAt(event->pos());
        const i32 row = idx.isValid() ? idx.row() : -1;

        auto newButton = GitFileDelegate::HoverButton::None;
        if (row >= 0) {
            newButton = GitFileDelegate::hitTestButton(visualRect(idx), event->pos());
        }

        const bool rowChanged = row != delegate->hoveredRow();
        const bool buttonChanged = newButton != delegate->hoveredButton();

        if (rowChanged || buttonChanged) {
            const i32 oldRow = delegate->hoveredRow();
            delegate->setHoveredRow(row);
            delegate->setHoveredButton(newButton);

            if (oldRow >= 0) {
                update(model_->index(oldRow));
            }

            if (row >= 0) {
                update(model_->index(row));
            }
        }
    }

    void leaveEvent(QEvent* const event) override {
        QListView::leaveEvent(event);

        const i32 oldRow = delegate->hoveredRow();
        if (oldRow >= 0) {
            delegate->setHoveredRow(-1);
            delegate->setHoveredButton(GitFileDelegate::HoverButton::None);
            update(model_->index(oldRow));
        }
    }

   private:
    void showContextMenu(const QPoint& pos) {
        const QModelIndexList selected = selectionModel()->selectedIndexes();
        if (selected.isEmpty()) {
            return;
        }

        QStringList paths;
        paths.reserve(selected.size());
        for (const QModelIndex& idx : selected) {
            paths.push_back(model_->items().at(idx.row()).filePath);
        }

        auto menu = QMenu(this);
        QAction* const stageAction = menu.addAction(tr("Stage Selected"));
        QAction* const revertAction = menu.addAction(tr("Revert Selected"));

        QAction* const chosen = menu.exec(viewport()->mapToGlobal(pos));
        if (chosen == stageAction) {
            emit stageMultipleRequested(paths);
        } else if (chosen == revertAction) {
            emit revertMultipleRequested(paths);
        }
    }

    GitFileModel* const model_;
    GitFileDelegate* const delegate;
};
#else
#include <QListView>

class GitFileList final : public QListView {
    Q_OBJECT

   public:
    using QListView::QListView;
};
#endif
