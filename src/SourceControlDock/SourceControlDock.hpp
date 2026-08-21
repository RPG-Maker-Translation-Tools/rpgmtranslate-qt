#pragma once

#include <QDockWidget>

#ifdef ENABLE_LIBGIT2
#include "Aliases.hpp"
#include "FWD.hpp"
#include "GitFileList.hpp"
#include "Settings.hpp"
#include "ezgit2.hpp"

#include <QFileSystemWatcher>

namespace Ui {
    class MainWindow;
}  // namespace Ui

class SourceControlDock final : public QDockWidget {
    Q_OBJECT

   public:
    explicit SourceControlDock(QWidget* const parent = nullptr) { git_libgit2_init(); }

    ~SourceControlDock() override { git_libgit2_shutdown(); }

    void init(const Ui::MainWindow& ui_);

    [[nodiscard]] auto openRepository(const QString& dataPath, const GitSettings& gitSettings) -> result<void, QString>;
    void clear();

   private:
    inline void commit(bool amend = false, bool force = false);
    inline void updateSettings(const GitSettings& gitSettings);
    inline void loadUI(const GitSettings& gitSettings);

    ezgit2::Repository repo;

    QWidget* sourceControlDockContent;

    QPushButton* commitButton;
    QToolButton* commitOptionsButton;

    QPlainTextEdit* commitMessageInput;

    QComboBox* branchSelect;
    GitFileList* fileList;
    GitCommitList* commitList;

    QPushButton* copyTranslationButton;
    QToolButton* refreshChangesButton;
};
#else
class SourceControlDock final : public QDockWidget {
   public:
    using QDockWidget::QDockWidget;
};
#endif
