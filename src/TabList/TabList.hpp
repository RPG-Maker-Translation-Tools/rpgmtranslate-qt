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

    [[nodiscard]] constexpr auto model() const -> QStandardItemModel* {
        return model_;
    }

    [[nodiscard]] auto displayingPercents() const -> bool {
        return displayPercents;
    }

    void
    addItem(const QString& name, u32 total, u32 translated, bool completed);
    void setProgress(u16 row, u32 total, u32 translated);

    void setProgressDisplay(bool percents);
    [[nodiscard]] auto progressDisplay() const -> bool;

    void toggleCompleted(const QModelIndex& index);
    void setCompleted(const QModelIndex& index, bool completed);

   signals:
    void indexChanged(const QModelIndex& current, const QModelIndex& previous);
    void rightClicked(const QModelIndex& index, const QPoint& pos);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    QStandardItemModel* model_;
    TabListDelegate* delegate;

    bool displayPercents = false;
};
