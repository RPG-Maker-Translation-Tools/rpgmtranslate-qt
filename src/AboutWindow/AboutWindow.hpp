#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class AboutWindow;
}  // namespace Ui

QT_END_NAMESPACE

class AboutWindow final : public QDialog {
    Q_OBJECT

   public:
    explicit AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow() override;

   protected:
    void changeEvent(QEvent* event) override;

   private:
    inline auto setupUi() -> Ui::AboutWindow*;
    Ui::AboutWindow* const ui;
};