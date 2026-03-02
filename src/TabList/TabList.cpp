#include "TabList.hpp"

#include "FWD.hpp"
#include "TabListDelegate.hpp"

#include <QApplication>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QVBoxLayout>

TabList::TabList(QWidget* const parent) :
    QListView(parent),
    model_(new QStandardItemModel(this)),
    delegate(new TabListDelegate(this)) {
    setModel(model_);
    setItemDelegate(delegate);
    setUniformItemSizes(true);
    setSelectionMode(QListView::SingleSelection);
    setSelectionBehavior(QListView::SelectRows);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void TabList::addItem(
    const QString& name,
    const u32 total,
    const u32 translated,
    const bool completed
) {
    auto* const item = new QStandardItem();
    item->setData(name, Roles::NameRole);
    item->setData(total, Roles::TotalRole);
    item->setData(translated, Roles::TranslatedRole);
    item->setData(completed, Roles::CompletedRole);
    item->setEditable(false);
    model_->appendRow(item);
}

void TabList::setProgress(
    const u16 row,
    const u32 total,
    const u32 translated
) {
    if (row < 0 || row >= model_->rowCount()) {
        return;
    }

    model_->item(row)->setData(total, Roles::TotalRole);
    model_->item(row)->setData(translated, Roles::TranslatedRole);
}

void TabList::setProgressDisplay(const bool percents) {
    displayPercents = percents;
    viewport()->update();
}

[[nodiscard]] auto TabList::progressDisplay() const -> bool {
    return displayPercents;
};

void TabList::toggleCompleted(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    const bool current = index.data(Roles::CompletedRole).toBool();
    setCompleted(index, !current);
}

void TabList::setCompleted(const QModelIndex& index, const bool completed) {
    if (!index.isValid()) {
        return;
    }

    auto* const item = model_->itemFromIndex(index);
    item->setData(completed, Roles::CompletedRole);
}

void TabList::mousePressEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        QListView::mousePressEvent(event);
    } else if (event->button() == Qt::RightButton) {
        const QModelIndex index = indexAt(event->pos());

        if (index.isValid()) {
            emit rightClicked(index, event->pos());
        }

        event->accept();
        return;
    } else {
        event->ignore();
    }
};