#pragma once

#include <QStyledItemDelegate>

class TabListDelegate final : public QStyledItemDelegate {
    Q_OBJECT

   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override;
};