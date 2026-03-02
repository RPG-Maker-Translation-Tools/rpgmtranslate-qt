#pragma once

#include "Aliases.hpp"

#include <QStandardItemModel>

class TranslationTableModel final : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit TranslationTableModel(QObject* parent);

    [[nodiscard]] auto flags(const QModelIndex& index) const
        -> Qt::ItemFlags override;
    auto setData(const QModelIndex& index, const QVariant& value, int role)
        -> bool override;

   signals:
    void bookmarkChanged(u32 row);
    void translatedChanged(i8 count);
};