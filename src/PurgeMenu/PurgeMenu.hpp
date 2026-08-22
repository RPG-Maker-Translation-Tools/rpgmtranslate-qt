#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "FileSelectMenu.hpp"

#include <QDialog>
#include <QEventLoop>
#include <QWidget>

QT_BEGIN_NAMESPACE

namespace Ui {
    class PurgeMenu;
}  // namespace Ui

QT_END_NAMESPACE

class PurgeMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit PurgeMenu(QWidget* parent = nullptr);
    ~PurgeMenu() override;

    void clear();
    void init(const vector<TabListItem>& files);

    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;
    [[nodiscard]] auto createIgnore() const -> bool;

    auto exec() -> QDialog::DialogCode {
        QEventLoop loop;
        QDialog::DialogCode code;

        connect(this, &PurgeMenu::accepted, &loop, [&loop, &code] -> void {
            loop.quit();
            code = QDialog::DialogCode::Accepted;
        });

        connect(this, &PurgeMenu::rejected, &loop, [&loop, &code] -> void {
            loop.quit();
            code = QDialog::DialogCode::Rejected;
        });

        loop.exec();
        return code;
    }

   signals:
    void accepted();
    void rejected();

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::PurgeMenu*;

    Ui::PurgeMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
};
