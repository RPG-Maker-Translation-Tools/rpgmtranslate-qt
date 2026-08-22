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
        const ProjectSettings* projS,
        GlossaryMenu* glossaryMenu
    );
    void clear();

   signals:
    void checkRequested(Selected selected);

   private:
    void toggleAboveDock(QWidget* menu, const QWidget* anchor);

    QPushButton* clearButton;
    QPushButton* checkButton;
    QPushButton* fileSelectMenuButton;
    QPushButton* lintSelectMenuButton;
    LintTable* lintTable = nullptr;

    FileSelectMenu* fileSelectMenu;
    LintSelectMenu* lintSelectMenu;

    Settings* sets;
    const ProjectSettings* projS;
};
