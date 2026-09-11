#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PackedBits.hpp"

#include <QAbstractListModel>

using TabListItemProps = PackedBits<Field<"total", i32, 32>, Field<"translated", i32, 31>, Field<"completed", bool, 1>>;

struct TabListItem {
    QString name;
    TabListItemProps properties;
};

class TabListModel final : public QAbstractListModel {
    Q_OBJECT

   public:
    explicit TabListModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const -> i32 override;
    [[nodiscard]] auto data(const QModelIndex& index, i32 role = Qt::DisplayRole) const -> QVariant override;
    [[nodiscard]] auto flags(const QModelIndex& index) const -> Qt::ItemFlags override;

    [[nodiscard]] auto tab(u32 index) const -> const TabListItem&;
    [[nodiscard]] auto tab(u32 index) -> TabListItem&;

    void clear();
    void setTabs(vector<TabListItem> items);

   private:
    vector<TabListItem> tabs;
};
