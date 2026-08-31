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
    class MainWindow;
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
    [[nodiscard]] auto setupUi() -> Ui::MainWindow*;
    void retranslate(QLocale::Language language);
    void exit();

    // Functions
    void checkForUpdates(bool manual = false);
    void openProject(const QString& folder, bool newProject);
    void changeTab(const QString& tabName, const QString& previousTabName);
    void closeProject();

    [[nodiscard]] auto startTask(TaskKind kind) -> TaskToken;

    void reportSkippedFiles(span<const FilenameArray> filenames, u16 skippedCount);

    [[nodiscard]] auto search(
        Selected selected,
        const QString& searchText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags
    ) -> u16;

    void checkHashes();
    void read(
        ReadMode readMode,
        DuplicateMode duplicateMode,
        Selected selected,
        BaseFlags flags,
        bool mapEvents,
        const QString& title,
        const QString& readEncoding
    );
    [[nodiscard]] auto initDictionary() -> result<void, QString>;

    // Callbacks
    void logFFI(u8 level, const QString& message);
    void handleGoToRow();
    void handleWrite();
    void handleSearchMenuAction(
        Selected selected,
        SearchAction action,
        const QString& searchText,
        const QString& replaceText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags
    );

    void handleRowRemoved(RemovedRowInfo info);
    void handleBatchMenuAction(const JSScript& script, Selected selected, u8 columnIndex, const BatchVariant& variant);
    void handleSearchPanelAction(
        SearchPanelDock::Action action,
        const QString& filename,
        u32 rowIndex,
        u8 columnIndex,
        span<const TextMatch> matches,
        SearchResultListItem& item
    );

    void handleGlobalLint(Selected selected);
    void handleSingleLint();

    void warnIfGlossaryLanguagesUnset();
    void applyLintRows(vector<LintRow> rows);

    void showSettingsWindow();
    void showAboutWindow();
    void handleInputFocused();

    // Connections
    void setupConnections();

    // Load/save
    void initializeSettings();

    [[nodiscard]] auto saveSettings() -> bool;
    [[nodiscard]] auto saveProjectSettings() -> bool;
    [[nodiscard]] auto saveGlossary() -> bool;
    [[nodiscard]] auto saveEverything() -> bool;

    [[nodiscard]] auto saveCurrentTab(QString tabName = QString()) -> bool;
    [[nodiscard]] auto saveMaps() -> bool;
    void saveBackup();

    void loadSettings();
    void loadBackup(const QString& backupPath);

    // ...
    HashMap<u16, QString> mapSections;

    QString lockedFile;

    shared_ptr<Settings> settings;
    shared_ptr<ProjectSettings> projectSettings;

    QTimer backupTimer;
    QTimer lintDebounceTimer;

    // UI
    Ui::MainWindow* const ui;

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
    SerdeMenu* const serdeMenu;
    AssetMenu* const assetMenu;
    TaskPanel* const taskPanel;
    QMenu* const recentProjectsMenu;

    QLabel* const linesStatusLabel;
    QLabel* const progressStatusLabel;
    QLabel* const tabNameStatusLabel;

    FFILogger& ffiLogger;
    TaskWorker* const taskWorker;

    QAction* const actionGoToRow = new QAction(this);

    QAction* const actionTabPanel = new QAction(QIcon(u":/icons/menu.svg"_s), tr("Tab Panel"), this);
    QAction* const actionSave = new QAction(QIcon(u":/icons/save.svg"_s), tr("Save"), this);
    QAction* const actionWrite = new QAction(QIcon(u":/icons/manufacturing.svg"_s), tr("Write"), this);
    QAction* const actionSearch = new QAction(QIcon(u":/icons/search.svg"_s), tr("Search"), this);
    QAction* const actionBatchMenu = new QAction(QIcon(u":/icons/construction.svg"_s), tr("Batch Menu"), this);
    QAction* const actionGlossaryMenu = new QAction(QIcon(u":/icons/book_3.svg"_s), tr("Glossary Menu"), this);
    QAction* const actionLintMenu = new QAction(QIcon(u":/icons/lab_profile.svg"_s), tr("Lint Menu"), this);
    QAction* const actionTranslationsMenu =
        new QAction(QIcon(u":/icons/translate.svg"_s), tr("Translations Menu"), this);
    QAction* const actionBookmarkMenu = new QAction(QIcon(u":/icons/bookmarks.svg"_s), tr("Bookmark Menu"), this);
    QAction* const actionSourceControl = new QAction(QIcon(u":/icons/commit.svg"_s), tr("Source Control"), this);
    QAction* const actionAssets = new QAction(QIcon(u":/icons/stock_media.svg"_s), tr("Assets"), this);
    QAction* const actionLocateProjectDir =
        new QAction(QIcon(u":/icons/folder_code.svg"_s), tr("Locate Project Directory"), this);
    QAction* const actionSearchPanel = new QAction(QIcon(u":/icons/dock.svg"_s), tr("Search Panel"), this);

    bool firstReadPending = false;
};
