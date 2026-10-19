#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QListView>

class TabList final : public QListView {
    Q_OBJECT

   public:
    explicit TabList(QWidget* parent = nullptr);

    [[nodiscard]] constexpr auto model() const -> TabListModel* { return model_; }

    void setProgressDisplay(bool percents);
    [[nodiscard]] auto progressDisplay() const -> bool;

    void toggleCompleted(const QModelIndex& index);

    [[nodiscard]] auto tab(u32 index) -> TabListItem&;
    void clear();
    void setTabs(vector<TabListItem> tabs);

   signals:
    // globalPos is in screen coordinates, as QMenu::exec takes them
    void rightClicked(const QModelIndex& index, const QPoint& globalPos);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    TabListModel* model_;
    TabListDelegate* delegate;

    bool displayPercents = false;
};
