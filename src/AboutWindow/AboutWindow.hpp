#pragma once

#include <QDialog>

namespace Ui {
class AboutWindow;
}  // namespace Ui

class AboutWindow final : public QDialog {
    Q_OBJECT

   public:
    explicit AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow() override;

   protected:
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::AboutWindow*;
    void fill();
    Ui::AboutWindow* const ui;
};
