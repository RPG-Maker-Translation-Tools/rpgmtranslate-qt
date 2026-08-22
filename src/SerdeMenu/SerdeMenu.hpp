#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "FileSelectMenu.hpp"
#include "rpgmtranslate_rs.h"

#include <QWidget>

QT_BEGIN_NAMESPACE

namespace Ui {
    class SerdeMenu;
}  // namespace Ui

QT_END_NAMESPACE

class SerdeMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit SerdeMenu(QWidget* parent = nullptr);
    ~SerdeMenu() override;

    void clear();
    void init(const vector<TabListItem>& files);

    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;
    [[nodiscard]] auto format() const -> SerdeFormat;
    [[nodiscard]] auto directory() const -> QString;

   signals:
    void exportRequested();
    void importRequested();

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::SerdeMenu*;

    Ui::SerdeMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
};
