#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QAbstractListModel>

struct TabListItem {
    QString name;
    i32 total;
    i32 translated;
    bool completed;
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
