#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"
#include "Types.hpp"

namespace Ui {
class BatchMenu;
}  // namespace Ui

class BatchMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit BatchMenu(QWidget* parent = nullptr);
    ~BatchMenu() override;

    void clear();
    void init(const vector<TabListItem>& files, const ProjectSettings* projectSettings);
    void addColumn(const QString& name);
    void renameColumn(u8 index, const QString& name);
    [[nodiscard]] auto save() -> bool;

   signals:
    void actionRequested(JSScript script, Selected selected, u8 columnIndex, BatchVariant variant);

   protected:
    void changeEvent(QEvent* event) override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::BatchMenu*;

    // Copies the text of the editor into scripts when it shows a custom script
    void storeCurrentScript();

    // Re-reads the scripts directory before the combo box opens. A removed open script
    // switches to the first built-in one
    void refreshScripts();

    // The custom scripts, and the file name each was loaded from or last saved under (empty for a new one), parallel to
    // the combo box items after the built-in ones
    vector<QString> scripts;
    vector<QString> savedNames;

    Ui::BatchMenu* const ui;

    const ProjectSettings* projectSettings = nullptr;
    FileSelectMenu* fileSelectMenu;

    i32 prevScript = 0;
};
