#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "FileSelectMenu.hpp"

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class WriteMenu;
}  // namespace Ui

QT_END_NAMESPACE

class WriteMenu final : public QDialog {
    Q_OBJECT

   public:
    explicit WriteMenu(QWidget* parent = nullptr);
    ~WriteMenu() override;

    void clear();
    void addFile(const QString& file);

    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    inline auto setupUi() -> Ui::WriteMenu*;

    Ui::WriteMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
};