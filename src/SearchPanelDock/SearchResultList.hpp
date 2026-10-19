#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Hasher.hpp"
#include "Types.hpp"

#include <QAbstractListModel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTimer>

enum SearchResultListRole : u16 {
    TitleRole = Qt::UserRole + 1,
    SubtitleRole = Qt::UserRole + 2,
    SpansRole = Qt::UserRole + 3
};

// The title and subtitle are made when the item is shown, from `file` and `cellMatch`.
struct SearchResultListItem {
    FileMatches& file;
    CellMatch& cellMatch;
    QL1SV filename;
    bool replaced;
    bool hidden;
};

class SearchResultListDelegate final : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    constexpr void setProxy(const SearchResultListProxy* const proxy) { proxy_ = proxy; };

    void invalidateCache() { heightCache_.clear(); };

    [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

   private:
    [[nodiscard]] static auto
    computeHeight(const QStyleOptionViewItem& option, const QModelIndex& index, i32 contentWidth) -> i32;

    mutable HashMap<i32, i32> heightCache_;
    const SearchResultListProxy* proxy_;

    mutable i32 cachedWidth_ = -1;
};

class SearchResultListModel final : public QAbstractListModel {
   public:
    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto data(const QModelIndex& idx, i32 role) const -> QVariant override;
    [[nodiscard]] auto flags(const QModelIndex& idx) const -> Qt::ItemFlags override;

    void clear() { setItems({}); };

    // Makes the subtitles
    constexpr void setPanel(const SearchPanelDock* const panel) { panel_ = panel; }

    void setItems(vector<SearchResultListItem>&& items);

    [[nodiscard]] constexpr auto item(const i32 row) -> SearchResultListItem& { return items[row]; };

    // Counts the times the items were replaced: an item that was handed out can outlive the list.
    [[nodiscard]] constexpr auto generation() const -> u32 { return generation_; }

    // Tells the views that `changed`, an item of the list, was edited.
    void itemChanged(const SearchResultListItem& changed) {
        const QModelIndex changedIndex = index(scast<i32>(&changed - items.data()), 0);
        emit dataChanged(changedIndex, changedIndex);
    }

   private:
    vector<SearchResultListItem> items;
    const SearchPanelDock* panel_ = nullptr;
    u32 generation_ = 0;
};

class SearchResultListProxy final : public QSortFilterProxyModel {
   public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    using QSortFilterProxyModel::invalidateFilter;

   protected:
    [[nodiscard]] auto filterAcceptsRow(
        const i32 row,
        const QModelIndex& /* parent */
    ) const -> bool override {
        auto* const model = scast<SearchResultListModel*>(sourceModel());
        const auto& item = model->item(row);
        return !item.hidden;
    }
};

class SearchResultList final : public QListView {
    Q_OBJECT

   public:
    explicit SearchResultList(QWidget* parent = nullptr);

    [[nodiscard]] constexpr auto delegate() const -> SearchResultListDelegate* { return delegate_; }

    [[nodiscard]] constexpr auto model() const -> SearchResultListModel* { return model_; }

    [[nodiscard]] constexpr auto proxy() const -> SearchResultListProxy* { return proxy_; }

    void clear() { model_->clear(); }

   signals:
    void clicked(Qt::MouseButton button, const QModelIndex& index);

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

   private:
    QTimer relayoutTimer;

    SearchResultListDelegate* delegate_;
    SearchResultListModel* model_;
    SearchResultListProxy* proxy_;
};
