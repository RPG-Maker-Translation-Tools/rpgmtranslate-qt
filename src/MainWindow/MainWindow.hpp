#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FFILogger.hpp"
#include "FWD.hpp"
#include "ProjectSettings.hpp"
#include "PurgeMenu.hpp"
#include "Settings.hpp"
#include "TabPanel.hpp"
#include "TaskWorker.hpp"
#include "WriteMenu.hpp"

#include <QLocale>
#include <QMainWindow>
#include <QShortcut>
#include <QTimer>

QT_BEGIN_NAMESPACE

namespace Ui {
    class mainWindow;
}  // namespace Ui

QT_END_NAMESPACE

class MainWindow final : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;

   private:
    inline auto setupUi() -> Ui::mainWindow*;

    inline void initializeSettings();

    inline auto saveSettings() -> bool;
    inline auto saveProjectSettings() -> bool;
    inline auto saveGlossary() -> bool;
    inline auto saveEverything() -> bool;

    inline auto saveCurrentTab(QString tabName = QString()) -> bool;
    inline auto saveMaps() -> bool;
    inline void saveBackup();

    inline void loadSettings();

    inline void openProject(const QString& folder);

    inline void
    changeTab(const QString& tabName, const QString& previousTabName);

    inline void showSettingsWindow();
    inline void showAboutWindow();

    inline void checkForUpdates(bool manual = false);
    inline void retranslate(QLocale::Language language);
    inline void exit();

    inline auto search(
        Selected selected,
        const QString& searchText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags
    ) -> u16;

    inline void appendMatches(
        const QString& filename,
        QStringView source,
        QStringView translation,
        const Term& term,
        u32 idx
    );

    inline void closeProject();

    void updateTask(TaskWorker::Task task, u32 progress, u32 total);

    [[nodiscard]] inline auto
    handleOpenError(const QString& path, const QString& error) -> ControlFlow;

    HashMap<u16, QString> mapSections;

    QString lockedFile;

    shared_ptr<Settings> settings;
    shared_ptr<ProjectSettings> projectSettings;

    QTimer backupTimer;

    // UI
    Ui::mainWindow* const ui;

    QProgressDialog* updateProgressDialog = nullptr;

    QTranslator* translator;

    TabPanel* const tabPanel;

    SearchMenu* const searchMenu;
    BatchMenu* const batchMenu;
    GlossaryMenu* const glossaryMenu;
    TranslationsMenu* const translationsMenu;
    BookmarkMenu* const bookmarkMenu;
    ReadMenu* const readMenu;
    WriteMenu* const writeMenu;
    PurgeMenu* const purgeMenu;

    QLabel* const linesStatusLabel;
    QLabel* const progressStatusLabel;
    QLabel* const tabNameStatusLabel;

    FFILogger& ffiLogger;
    TaskWorker* const taskWorker;

    QAction* const actionSave =
        new QAction(QIcon(u":/icons/save.svg"_s), tr("Save"), this);
    QAction* const actionWrite =
        new QAction(QIcon(u":/icons/manufacturing.svg"_s), tr("Write"), this);
    QAction* const actionSearch =
        new QAction(QIcon(u":/icons/search.svg"_s), tr("Search"), this);

    QAction* const actionTabPanel =
        new QAction(QIcon(u":/icons/menu.svg"_s), tr("Tab Panel"), this);
    QAction* const actionSearchPanel =
        new QAction(QIcon(u":/icons/dock.svg"_s), tr("Search Panel"), this);
    QAction* const actionGoToRow = new QAction(this);
    QAction* const actionBatchMenu = new QAction(
        QIcon(u":/icons/construction.svg"_s),
        tr("Batch Menu"),
        this
    );
    QAction* const actionBookmarkMenu = new QAction(
        QIcon(u":/icons/bookmarks.svg"_s),
        tr("Bookmark Menu"),
        this
    );
    QAction* const actionMatchMenu = new QAction(
        QIcon(u":/icons/lab_profile.svg"_s),
        tr("Match Menu"),
        this
    );
    QAction* const actionGlossaryMenu =
        new QAction(QIcon(u":/icons/book_3.svg"_s), tr("Glossary Menu"), this);
    QAction* const actionTranslationsMenu = new QAction(
        QIcon(u":/icons/translate.svg"_s),
        tr("Translations Menu"),
        this
    );

    bool firstReadPending = false;
};
