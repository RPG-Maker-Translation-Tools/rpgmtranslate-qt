#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "FileSelectMenu.hpp"

#include <QDialog>

namespace Ui {
class WriteMenu;
}  // namespace Ui

class WriteMenu final : public QDialog {
    Q_OBJECT

   public:
    explicit WriteMenu(QWidget* parent = nullptr);
    ~WriteMenu() override;

    void clear();
    void init(const vector<TabListItem>& files);

    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::WriteMenu*;

    Ui::WriteMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
};
