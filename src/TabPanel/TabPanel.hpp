#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

class TabPanel final : public QWidget {
    Q_OBJECT

   public:
    explicit TabPanel(QWidget* parent = nullptr);

    void init();
    void
    addTab(const QString& basename, u32 total, u32 translated, bool completed);
    [[nodiscard]] auto tabCount() const -> u16;
    [[nodiscard]] auto tabName(u16 tabIndex) const -> QString;
    [[nodiscard]] auto currentTabName() const -> QString;
    void clear();
    void changeTab(const QString& filename);

    [[nodiscard]] auto tabIndex(const QString& tabName) const -> u32;
    [[nodiscard]] auto currentTranslated() const -> u32;
    [[nodiscard]] auto currentTotal() const -> u32;
    [[nodiscard]] auto tabTotal(u16 tabIndex) const -> u32;
    [[nodiscard]] auto tabTranslated(u16 tabIndex) const -> u32;
    void setTabTranslated(u16 tabIndex, u32 translated) const;
    void setCurrentTranslated(u32 translated) const;
    void setCurrentTotal(u32 total) const;
    void setProgressDisplay(bool percents) const;

    [[nodiscard]] auto tabs() const -> QStringList;

   signals:
    void tabChanged(const QString& tabName, const QString& previousTabName);
    void completedToggled(const QString& tabName, bool completed);
    void displayToggled();

   private:
    QVBoxLayout* layout;
    TabList* tabList;
};