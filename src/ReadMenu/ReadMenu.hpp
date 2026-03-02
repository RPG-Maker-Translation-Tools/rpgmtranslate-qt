#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QDialog>
#include <QEventLoop>
#include <QWidget>

QT_BEGIN_NAMESPACE

namespace Ui {
    class ReadMenu;
}  // namespace Ui

QT_END_NAMESPACE

class ReadMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit ReadMenu(QWidget* parent = nullptr);
    ~ReadMenu() override;

    void clear();
    void addFile(const QString& file);

    [[nodiscard]] auto parseMapEvents() const -> bool;
    [[nodiscard]] auto readMode() const -> ReadMode;
    [[nodiscard]] auto duplicateMode() const -> DuplicateMode;
    [[nodiscard]] auto flags() const -> BaseFlags;
    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;

    void init(const shared_ptr<ProjectSettings>& settings);

    auto exec() -> QDialog::DialogCode {
        QEventLoop loop;
        QDialog::DialogCode code;

        connect(this, &ReadMenu::accepted, &loop, [this, &loop, &code] -> void {
            loop.quit();
            code = QDialog::DialogCode::Accepted;
        });

        connect(this, &ReadMenu::rejected, &loop, [this, &loop, &code] -> void {
            loop.quit();
            code = QDialog::DialogCode::Rejected;
        });

        loop.exec();
        return code;
    };

   signals:
    void accepted();
    void rejected();

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    inline auto setupUi() -> Ui::ReadMenu*;

    Ui::ReadMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
    shared_ptr<ProjectSettings> projectSettings;
};