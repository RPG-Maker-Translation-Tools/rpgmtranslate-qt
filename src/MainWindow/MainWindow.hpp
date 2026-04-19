#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FFILogger.hpp"
#include "FWD.hpp"
#include "Hasher.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "TaskWorker.hpp"

#include <QLocale>
#include <QMainWindow>
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
    [[nodiscard]] inline auto setupUi() -> Ui::mainWindow*;

    inline void initializeSettings();

    [[nodiscard]] inline auto saveSettings() -> bool;
    [[nodiscard]] inline auto saveProjectSettings() -> bool;
    [[nodiscard]] inline auto saveGlossary() -> bool;
    [[nodiscard]] inline auto saveEverything() -> bool;

    [[nodiscard]] inline auto saveCurrentTab(QString tabName = QString())
        -> bool;
    [[nodiscard]] inline auto saveMaps() -> bool;
    inline void saveBackup();

    inline void loadSettings();

    inline void openProject(const QString& folder, bool newProject);

    inline void
    changeTab(const QString& tabName, const QString& previousTabName);

    inline void showSettingsWindow();
    inline void showAboutWindow();

    inline void checkForUpdates(bool manual = false);
    inline void retranslate(QLocale::Language language);
    inline void exit();

    [[nodiscard]] inline auto search(
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

    inline void checkHashes();
    inline void closeProject();

    inline void updateTask(TaskWorker::Task task, u32 progress, u32 total);
    inline void read(
        ReadMode readMode,
        DuplicateMode duplicateMode,
        Selected selected,
        BaseFlags flags,
        bool mapEvents,
        const QString& title
    );

    inline void loadBackup(const QString& filename);

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

    SearchMenu* const searchMenu;
    BatchMenu* const batchMenu;
    GlossaryMenu* const glossaryMenu;
    TranslationsMenu* const translationsMenu;
    BookmarkMenu* const bookmarkMenu;
    ReadMenu* const readMenu;
    WriteMenu* const writeMenu;
    PurgeMenu* const purgeMenu;
    AssetMenu* const assetMenu;

    QLabel* const linesStatusLabel;
    QLabel* const progressStatusLabel;
    QLabel* const tabNameStatusLabel;

    FFILogger& ffiLogger;
    TaskWorker* const taskWorker;

    QAction* const actionGoToRow = new QAction(this);

    QAction* const actionTabPanel =
        new QAction(QIcon(u":/icons/menu.svg"_s), tr("Tab Panel"), this);
    QAction* const actionSave =
        new QAction(QIcon(u":/icons/save.svg"_s), tr("Save"), this);
    QAction* const actionWrite =
        new QAction(QIcon(u":/icons/manufacturing.svg"_s), tr("Write"), this);
    QAction* const actionSearch =
        new QAction(QIcon(u":/icons/search.svg"_s), tr("Search"), this);
    QAction* const actionBatchMenu = new QAction(
        QIcon(u":/icons/construction.svg"_s),
        tr("Batch Menu"),
        this
    );
    QAction* const actionGlossaryMenu =
        new QAction(QIcon(u":/icons/book_3.svg"_s), tr("Glossary Menu"), this);
    QAction* const actionMatchMenu = new QAction(
        QIcon(u":/icons/lab_profile.svg"_s),
        tr("Match Menu"),
        this
    );
    QAction* const actionTranslationsMenu = new QAction(
        QIcon(u":/icons/translate.svg"_s),
        tr("Translations Menu"),
        this
    );
    QAction* const actionBookmarkMenu = new QAction(
        QIcon(u":/icons/bookmarks.svg"_s),
        tr("Bookmark Menu"),
        this
    );
    QAction* const actionSourceControl =
        new QAction(QIcon(u":/icons/commit.svg"_s), tr("Source Control"), this);
    QAction* const actionAssets =
        new QAction(QIcon(u":/icons/stock_media.svg"_s), tr("Assets"), this);
    QAction* const actionLocateProjectDir = new QAction(
        QIcon(u":/icons/folder_code.svg"_s),
        tr("Locate Project Directory"),
        this
    );
    QAction* const actionSearchPanel =
        new QAction(QIcon(u":/icons/dock.svg"_s), tr("Search Panel"), this);

    bool firstReadPending = false;
};
