
#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Types.hpp"

#include <QAbstractListModel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTimer>

enum SearchResultListRole : i16 {
    TitleRole = Qt::UserRole + 1,
    SubtitleRole = Qt::UserRole + 2,
    SpansRole = Qt::UserRole + 3
};

struct SearchResultListItem {
    QString title;
    QString subtitle;
    QLatin1StringView filename;
    CellMatch cellMatch;
    bool replaced;
    bool hidden;
};

class SearchResultListDelegate final : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    constexpr void setProxy(const SearchResultListProxy* const proxy) {
        proxy_ = proxy;
    };

    void invalidateCache() { heightCache_.clear(); };

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override;

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

   private:
    [[nodiscard]] auto computeHeight(
        const QStyleOptionViewItem& option,
        const QModelIndex& index,
        i32 contentWidth
    ) const -> i32;

    constexpr static u8 PAD_X = 8;
    constexpr static u8 PAD_Y = 8;
    constexpr static u8 GAP = 4;

    mutable HashMap<i32, i32> heightCache_;
    const SearchResultListProxy* proxy_;

    mutable i32 cachedWidth_ = -1;
};

class SearchResultListModel final : public QAbstractListModel {
   public:
    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const
        -> i32 override;
    [[nodiscard]] auto data(const QModelIndex& idx, i32 role) const
        -> QVariant override;
    [[nodiscard]] auto flags(const QModelIndex& idx) const
        -> Qt::ItemFlags override;

    void clear() { setItems({}); };

    void setItems(vector<SearchResultListItem>&& items);

    [[nodiscard]] constexpr auto item(const i32 row) -> SearchResultListItem& {
        return items[row];
    };

   private:
    vector<SearchResultListItem> items;
};

class SearchResultListProxy final : public QSortFilterProxyModel {
   public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

   protected:
    [[nodiscard]] auto filterAcceptsRow(
        const i32 row,
        const QModelIndex& /* parent */
    ) const -> bool override {
        auto* const model = as<SearchResultListModel*>(sourceModel());
        const auto& item = model->item(row);
        return !item.hidden;
    }
};

class SearchResultList final : public QListView {
    Q_OBJECT

   public:
    explicit SearchResultList(QWidget* parent = nullptr);

    [[nodiscard]] constexpr auto delegate() const -> SearchResultListDelegate* {
        return delegate_;
    }

    [[nodiscard]] constexpr auto model() const -> SearchResultListModel* {
        return model_;
    }

    [[nodiscard]] constexpr auto proxy() const -> SearchResultListProxy* {
        return proxy_;
    }

    void clear() { model_->clear(); }

   signals:
    void clicked(Qt::MouseButton button, const QModelIndex& index);

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

   private:
    QTimer relayoutTimer;
    QPoint pressPos;

    SearchResultListDelegate* delegate_;
    SearchResultListModel* model_;
    SearchResultListProxy* proxy_;
};
