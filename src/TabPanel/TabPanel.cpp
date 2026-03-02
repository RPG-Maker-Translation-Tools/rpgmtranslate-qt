#include "TabPanel.hpp"

#include "TabList.hpp"

#include <QApplication>
#include <QDir>
#include <QListWidget>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

TabPanel::TabPanel(QWidget* const parent) :
    QWidget(parent, Qt::FramelessWindowHint),

    layout(new QVBoxLayout(this)),

    tabList(new TabList(this)) {
    hide();
    layout->addWidget(tabList);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(
        tabList->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& previous)
            -> void {
        if (current.row() == previous.row()) {
            return;
        }

        emit tabChanged(
            current.isValid() ? tabList->model()
                                    ->itemFromIndex(current)
                                    ->data(TabList::Roles::NameRole)
                                    .toString()
                              : QString(),
            previous.isValid() ? tabList->model()
                                     ->itemFromIndex(previous)
                                     ->data(TabList::Roles::NameRole)
                                     .toString()
                               : QString()
        );
    }
    );

    connect(
        tabList,
        &TabList::rightClicked,
        this,
        [this](const QModelIndex& index, const QPoint& pos) -> void {
        auto* const menu = new QMenu(this);

        menu->addSeparator();

        const bool completed =
            index.isValid() ? index.data(TabList::Roles::CompletedRole).toBool()
                            : false;

        auto* const markCompletedAction = menu->addAction(
            completed ? tr("Unmark Completed") : tr("Mark as Completed")
        );

        menu->addSeparator();

        auto* const toggleProgressDisplayAction =
            menu->addAction(tr("Toggle Progress Display"));

        auto* const selectedAction = menu->exec(pos);

        if (selectedAction == markCompletedAction) {
            tabList->toggleCompleted(index);
            emit completedToggled(
                tabList->model()->itemFromIndex(index)->text(),
                !completed
            );
        } else if (selectedAction == toggleProgressDisplayAction) {
            setProgressDisplay(!tabList->progressDisplay());
            emit displayToggled();
        }
    }
    );
}

void TabPanel::addTab(
    const QString& basename,
    const u32 total,
    const u32 translated,
    const bool completed
) {
    tabList->addItem(basename, total, translated, completed);
    adjustSize();
}

auto TabPanel::tabCount() const -> u16 {
    return tabList->model()->rowCount();
};

auto TabPanel::tabName(const u16 tabIndex) const -> QString {
    return tabList->model()
        ->item(tabIndex, 0)
        ->data(TabList::Roles::NameRole)
        .toString();
};

auto TabPanel::currentTabName() const -> QString {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return {};
    }

    return tabList->model()
        ->itemFromIndex(currentIndex)
        ->data(TabList::Roles::NameRole)
        .toString();
}

void TabPanel::clear() {
    tabList->model()->clear();
}

[[nodiscard]] auto TabPanel::tabIndex(const QString& tabName) const -> u32 {
    for (const u16 tab : range<u16>(0, tabCount())) {
        if (tabList->model()->item(tab)->data(TabList::NameRole) == tabName) {
            return tab;
        }
    }

    std::unreachable();
};

[[nodiscard]] auto TabPanel::currentTranslated() const -> u32 {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return 0;
    }

    return tabList->model()
        ->itemFromIndex(currentIndex)
        ->data(TabList::Roles::TranslatedRole)
        .toUInt();
};

[[nodiscard]] auto TabPanel::currentTotal() const -> u32 {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return 0;
    }

    return tabList->model()
        ->itemFromIndex(currentIndex)
        ->data(TabList::Roles::TotalRole)
        .toUInt();
};

[[nodiscard]] auto TabPanel::tabTotal(const u16 tabIndex) const -> u32 {
    return tabList->model()->item(tabIndex)->data(TabList::TotalRole).toUInt();
};

[[nodiscard]] auto TabPanel::tabTranslated(const u16 tabIndex) const -> u32 {
    return tabList->model()
        ->item(tabIndex)
        ->data(TabList::TranslatedRole)
        .toUInt();
};

void TabPanel::setTabTranslated(
    const u16 tabIndex,
    const u32 translated
) const {
    tabList->model()->item(tabIndex)->setData(
        translated,
        TabList::TranslatedRole
    );
};

void TabPanel::setCurrentTranslated(const u32 translated) const {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return;
    }

    tabList->model()
        ->itemFromIndex(currentIndex)
        ->setData(
            tabList->model()
                    ->itemFromIndex(currentIndex)
                    ->data(TabList::Roles::TranslatedRole)
                    .toUInt() +
                translated,
            TabList::Roles::TranslatedRole
        );
}

void TabPanel::setCurrentTotal(const u32 total) const {
    const QModelIndex currentIndex = tabList->currentIndex();

    if (!currentIndex.isValid()) {
        return;
    }

    tabList->model()
        ->itemFromIndex(currentIndex)
        ->setData(
            tabList->model()
                    ->itemFromIndex(currentIndex)
                    ->data(TabList::Roles::TotalRole)
                    .toUInt() +
                total,
            TabList::Roles::TotalRole
        );
}

void TabPanel::setProgressDisplay(const bool percents) const {
    tabList->setProgressDisplay(percents);
};

void TabPanel::changeTab(const QString& filename) {
    if (filename.isEmpty()) {
        tabList->setCurrentIndex(QModelIndex());
        return;
    }

    for (const u16 tab : range<u16>(0, tabCount())) {
        if (tabList->model()->item(tab)->data(TabList::NameRole) == filename) {
            tabList->setCurrentIndex(tabList->model()->index(tab, 0));
            return;
        }
    }
}

auto TabPanel::tabs() const -> QStringList {
    const u16 rowCount = tabList->model()->rowCount();

    QStringList tabs;
    tabs.reserve(rowCount);

    for (const u16 idx : range<u16>(0, rowCount)) {
        tabs.append(tabList->model()
                        ->item(idx, 0)
                        ->data(TabList::Roles::NameRole)
                        .toString());
    }

    return tabs;
};