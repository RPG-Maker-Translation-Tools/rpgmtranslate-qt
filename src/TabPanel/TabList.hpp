#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QListView>

class TabList final : public QListView {
    Q_OBJECT

   public:
    enum Roles : u16 {
        NameRole = Qt::UserRole + 1,
        TotalRole,
        TranslatedRole,
        CompletedRole,
    };

    explicit TabList(QWidget* parent = nullptr);

    [[nodiscard]] constexpr auto model() const -> TabListModel* { return model_; }

    [[nodiscard]] auto displayingPercents() const -> bool { return displayPercents; }

    void setProgressDisplay(bool percents);
    [[nodiscard]] auto progressDisplay() const -> bool;

    void toggleCompleted(const QModelIndex& index);

    [[nodiscard]] auto tab(u16 index) -> TabListItem&;
    void clear();
    void setTabs(vector<TabListItem> tabs);

   signals:
    void indexChanged(const QModelIndex& current, const QModelIndex& previous);
    void rightClicked(const QModelIndex& index, const QPoint& pos);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    TabListModel* model_;
    TabListDelegate* delegate;

    bool displayPercents = false;
};
