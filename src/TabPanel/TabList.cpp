#include "TabList.hpp"

#include "FWD.hpp"
#include "TabListDelegate.hpp"
#include "TabListModel.hpp"

#include <QApplication>
#include <QMouseEvent>
#include <QVBoxLayout>

TabList::TabList(QWidget* const parent) :
    QListView(parent),
    model_(new TabListModel(this)),
    delegate(new TabListDelegate(this)) {
    setModel(model_);
    setItemDelegate(delegate);
    setUniformItemSizes(true);
    setSelectionMode(QListView::SingleSelection);
    setSelectionBehavior(QListView::SelectRows);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void TabList::setProgressDisplay(const bool percents) {
    displayPercents = percents;
    viewport()->update();
}

[[nodiscard]] auto TabList::progressDisplay() const -> bool {
    return displayPercents;
}

void TabList::toggleCompleted(const QModelIndex& index) {
    auto& tab = model_->tab(index.row());
    const bool current = tab.completed;
    tab.completed = !current;
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
}

auto TabList::tab(const u16 index) -> TabListItem& {
    return model_->tab(index);
};

void TabList::clear() {
    model_->clear();
    delegate->maxCachedNameWidth = 0;
}

void TabList::setTabs(vector<TabListItem> tabs) {
    model_->setTabs(std::move(tabs));
}