#include "TabPanel.hpp"

#include "TabList.hpp"
#include "TabListModel.hpp"
#include "Utils.hpp"

#include <QMenu>

TabPanel::TabPanel(QWidget* const parent) :
    QDockWidget(parent),

    tabList(new TabList(this)) {
    setWidget(tabList);

    connect(
        tabList->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& previous) -> void {
        if (current.row() == previous.row()) {
            return;
        }

        emit tabChanged(
            current.isValid() ? tabList->tab(current.row()).name : QString(),
            previous.isValid() ? tabList->tab(previous.row()).name : QString()
        );
    }
    );

    connect(tabList, &TabList::rightClicked, this, [this](const QModelIndex& index, const QPoint& pos) -> void {
        auto* const menu = new QMenu(this);

        menu->addSeparator();

        const bool completed = !index.isValid() || tabList->tab(index.row()).completed;

        auto* const markCompletedAction = menu->addAction(completed ? tr("Unmark Completed") : tr("Mark as Completed"));

        menu->addSeparator();

        auto* const toggleProgressDisplayAction = menu->addAction(tr("Toggle Progress Display"));

        auto* const selectedAction = menu->exec(pos);

        if (selectedAction == markCompletedAction) {
            tabList->toggleCompleted(index);
            emit completedToggled(tabList->tab(index.row()).name, !completed);
        } else if (selectedAction == toggleProgressDisplayAction) {
            setProgressDisplay(!tabList->progressDisplay());
            emit displayToggled();
        }
    });
}

void TabPanel::setTabs(vector<TabListItem> tabs) {
    tabList->setTabs(std::move(tabs));
    adjustSize();
}

auto TabPanel::tabCount() const -> i32 {
    return tabList->model()->rowCount();
};

auto TabPanel::tabName(const i32 tabIndex) const -> QString {
    return tabList->tab(tabIndex).name;
};

auto TabPanel::currentTabName() const -> QString {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return {};
    }

    return tabList->tab(currentIndex.row()).name;
}

void TabPanel::clear() {
    tabList->clear();
}

[[nodiscard]] auto TabPanel::tabIndex(const QString& tabName) const -> i32 {
    for (i32 tab = 0; tab < tabCount(); tab++) {
        if (tabList->tab(tab).name == tabName) {
            return tab;
        }
    }

    std::unreachable();
};

[[nodiscard]] auto TabPanel::currentTranslated() const -> i32 {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return 0;
    }

    return tabList->tab(currentIndex.row()).translated;
};

[[nodiscard]] auto TabPanel::currentTotal() const -> i32 {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return 0;
    }

    return tabList->tab(currentIndex.row()).total;
};

[[nodiscard]] auto TabPanel::tabTotal(const i32 tabIndex) const -> i32 {
    return tabList->tab(tabIndex).total;
};

[[nodiscard]] auto TabPanel::tabTranslated(const i32 tabIndex) const -> i32 {
    return tabList->tab(tabIndex).translated;
};

void TabPanel::setTabTranslated(const i32 tabIndex, const i32 translated) const {
    tabList->tab(tabIndex).translated = translated;
};

void TabPanel::setCurrentTranslated(const i32 translated) const {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return;
    }

    const i32 currentTranslated = tabList->tab(currentIndex.row()).translated;
    tabList->tab(currentIndex.row()).translated = currentTranslated + translated;
}

void TabPanel::setCurrentTotal(const i32 total) const {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return;
    }

    const i32 currentTotal = tabList->tab(currentIndex.row()).total;
    tabList->tab(currentIndex.row()).total = currentTotal + total;
}

void TabPanel::setProgressDisplay(const bool percents) const {
    tabList->setProgressDisplay(percents);
};

void TabPanel::changeTab(const QString& filename) {
    if (filename.isEmpty()) {
        tabList->setCurrentIndex(QModelIndex());
        return;
    }

    for (i32 tab = 0; tab < tabCount(); tab++) {
        if (tabList->tab(tab).name == filename) {
            tabList->setCurrentIndex(tabList->model()->index(tab, 0));
            return;
        }
    }
}

auto TabPanel::tabs() const -> QStringList {
    const i32 rowCount = tabList->model()->rowCount();

    QStringList tabs;
    tabs.reserve(rowCount);

    for (i32 idx = 0; idx < rowCount; idx++) {
        tabs.append(tabList->tab(idx).name);
    }

    return tabs;
};