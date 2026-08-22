#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDockWidget>

class TabPanel final : public QDockWidget {
    Q_OBJECT

   public:
    explicit TabPanel(QWidget* parent = nullptr);

    void init();
    void addTab(const QString& basename, i32 total, i32 translated, bool completed);
    [[nodiscard]] auto tabCount() const -> i32;
    [[nodiscard]] auto tabName(i32 tabIndex) const -> QString;
    [[nodiscard]] auto currentTabName() const -> QString;
    void clear();
    void changeTab(const QString& filename);

    [[nodiscard]] auto tabIndex(const QString& tabName) const -> i32;
    [[nodiscard]] auto currentTranslated() const -> i32;
    [[nodiscard]] auto currentTotal() const -> i32;
    [[nodiscard]] auto tabTotal(i32 tabIndex) const -> i32;
    [[nodiscard]] auto tabTranslated(i32 tabIndex) const -> i32;
    void setTabTranslated(i32 tabIndex, i32 translated) const;
    void setCurrentTranslated(i32 translated) const;
    void setCurrentTotal(i32 total) const;
    void setProgressDisplay(bool percents) const;

    [[nodiscard]] auto tabs() const -> QStringList;
    void setTabs(vector<TabListItem> tabs);

   signals:
    void tabChanged(const QString& tabName, const QString& previousTabName);
    void completedToggled(const QString& tabName, bool completed);
    void displayToggled();

   private:
    TabList* tabList;
};
