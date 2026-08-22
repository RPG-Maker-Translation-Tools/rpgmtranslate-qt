#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QListView>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

struct Bookmark {
    QString description;
    FilenameArray filename;
    u32 row;
    bool hidden;
};

class BookmarkList final : public QListView {
    Q_OBJECT

   public:
    explicit BookmarkList(QWidget* parent);

    [[nodiscard]] auto bookmark(i32 row) const -> Bookmark&;

    [[nodiscard]] auto rowCount() const -> i32;
    void appendRow(QStringView description, QStringView file, u32 row) const;
    void removeRow(i32 row) const;
    void setRowHidden(i32 row, bool hidden) const;

    void clear() const;
    void refilter() const;

   signals:
    void bookmarkClicked(QL1SV file, u32 row);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    BookmarkListModel* model_;
    BookmarkProxy* proxy;
    BookmarkListDelegate* delegate;
};

class BookmarkListModel final : public QAbstractListModel {
   public:
    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] auto data(const QModelIndex& idx, i32 role) const -> QVariant override;
    [[nodiscard]] auto bookmark(i32 row) -> Bookmark&;
    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto flags(const QModelIndex& idx) const -> Qt::ItemFlags override;

    void removeRow(i32 row);
    void appendRow(QStringView file, QStringView description, u32 row);

    void clear();

   private:
    QList<Bookmark> bookmarks;
};

class BookmarkProxy final : public QSortFilterProxyModel {
   public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

   protected:
    [[nodiscard]] auto filterAcceptsRow(
        i32 row,
        const QModelIndex& /* parent */
    ) const -> bool override;
};

class BookmarkListDelegate final : public QStyledItemDelegate {
    Q_OBJECT

   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize override;
};
