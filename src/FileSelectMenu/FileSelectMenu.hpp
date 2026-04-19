#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Hasher.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class FileSelectMenu;
}  // namespace Ui

QT_END_NAMESPACE

class FileSelectMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit FileSelectMenu(QWidget* parent = nullptr);

    ~FileSelectMenu() override;

    void setFiles(const vector<TabListItem>& files);
    void addFile(const QString& filename, bool checked = false);
    void clear();
    [[nodiscard]] auto selected(bool skipped = false) -> Selected;
    [[nodiscard]] auto selectedCount() const -> u16;
    [[nodiscard]] auto empty() const -> bool;

   protected:
    void changeEvent(QEvent* event) override;
    auto eventFilter(QObject* obj, QEvent* event) -> bool override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::FileSelectMenu*;

    Ui::FileSelectMenu* const ui;

    bool leftButtonDown = false;
    HashSet<QCheckBox*> dragTouched;
};
