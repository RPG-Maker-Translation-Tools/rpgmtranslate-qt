#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "LintTable.hpp"

#include <QDockWidget>

namespace Ui {
class MainWindow;
}  // namespace Ui

class LintMenu final : public QDockWidget {
    Q_OBJECT

   public:
    explicit LintMenu(QWidget* parent = nullptr);

    void init(
        Ui::MainWindow* ui_,
        const vector<TabListItem>& files,
        Settings* sets,
        ProjectSettings* projS,
        GlossaryMenu* glossaryMenu
    );
    // Closing a project: the results and what the selection menus list of it
    void clear();
    // Only the results, for a new lint run
    void clearResults();

    // The settings were changed (by the settings window): what the lint selection menu holds of them is stale.
    void settingsChanged();

   signals:
    void checkRequested(Selected selected);

   private:
    void toggleAboveDock(QWidget* menu, const QWidget* anchor);

    QPushButton* clearButton = nullptr;
    QPushButton* checkButton = nullptr;
    QPushButton* fileSelectMenuButton = nullptr;
    QPushButton* lintSelectMenuButton = nullptr;
    LintTable* lintTable = nullptr;

    FileSelectMenu* fileSelectMenu;
    LintSelectMenu* lintSelectMenu;

    Settings* sets = nullptr;
    ProjectSettings* projS = nullptr;

    bool connected = false;
};
