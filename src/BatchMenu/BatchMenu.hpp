#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"
#include "Types.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class BatchMenu;
}  // namespace Ui

QT_END_NAMESPACE

class BatchMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    enum TrimFlags : u8 {
        Leading = 1,
        Trailing = 2
    };

    explicit BatchMenu(QWidget* parent = nullptr);
    ~BatchMenu() override;

    void clear();
    void setFiles(const vector<TabListItem>& files);
    void addColumn(const QString& name);
    void renameColumn(u8 index, const QString& name);
    void setEndpoints(const vector<EndpointSettings>& endpoints);

   signals:
    void actionRequested(
        Selected selected,
        BatchAction action,
        u8 columnIndex,
        std::variant<TrimFlags, tuple<u8, QString>, u8>
    );

   protected:
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::BatchMenu*;

    Ui::BatchMenu* const ui;

    FileSelectMenu* fileSelectMenu;

    bool dragging = false;
    bool dragSetValue = false;
    QSet<QCheckBox*> dragTouched;
};
