#include "TabListModel.hpp"

#include "FWD.hpp"

TabListModel::TabListModel(QObject* const parent) : QAbstractListModel(parent) {}

auto TabListModel::rowCount(const QModelIndex& /* parent */) const -> i32 {
    return scast<i32>(tabs.size());
}

auto TabListModel::data(const QModelIndex& /* index */, const i32 /* role */) const -> QVariant {
    return {};
}

auto TabListModel::flags(const QModelIndex& index) const -> Qt::ItemFlags {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

auto TabListModel::tab(const u32 index) const -> const TabListItem& {
    return tabs[index];
}

auto TabListModel::tab(const u32 index) -> TabListItem& {
    return tabs[index];
}

void TabListModel::clear() {
    beginResetModel();
    this->tabs = {};
    endResetModel();
}

void TabListModel::setTabs(vector<TabListItem> items) {
    beginResetModel();
    this->tabs = std::move(items);
    endResetModel();
}