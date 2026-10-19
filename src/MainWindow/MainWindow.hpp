#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FFILogger.hpp"
#include "FWD.hpp"
#include "Hasher.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "TaskWorker.hpp"
#include "TranslationTable.hpp"

#include <QFuture>
#include <QLocale>
#include <QMainWindow>
#include <QPointer>
#include <QProgressDialog>
#include <QTimer>

namespace Ui {
class MainWindow;
}  // namespace Ui

class MainWindow final : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] auto setupUi() -> Ui::MainWindow*;
    // Falls back to English for a language that has no translation
    void retranslate(QLocale::Language language);

    // Everything that `ui->retranslateUi()` doesn't reach
    void retranslateActions();
    void updateStatusLabels();
    void updateTaskButton(i32 count);
    void exit();
    // Stops the tasks and saves everything, what has to be done before the application quits. False if it must not
    // quit.
    [[nodiscard]] auto prepareToQuit() -> bool;

    void checkForUpdates(bool manual = false);
    // `folder` is taken by value: it may alias `settings` or `projectSettings`, which `closeProject()` clears.
    // `translationsRewritten`: a task rewrote the translation files, so `mapSections` must not be saved over them.
    void openProject(QString folder, bool newProject, bool translationsRewritten = false);
    void changeTab(const QString& tabName, const QString& previousTabName);

    // False if the project stays open (a task can't be stopped, or saving failed). The user is told why.
    // `translationsRewritten`: see `openProject()`.
    bool closeProject(bool translationsRewritten = false);

    // Stops the running tasks and waits for them. False, with a message, if one can't be stopped.
    [[nodiscard]] auto stopTasks() -> bool;

    // Saves the edits and closes the open tab before a task rewrites the translation files on its own.
    [[nodiscard]] auto flushTranslations() -> bool;

    // Writes the open tab and `mapSections` to disk, for whatever reads the translation files.
    [[nodiscard]] auto saveTranslations() -> bool;

    [[nodiscard]] auto startTask(TaskKind kind) -> TaskToken;

    // For a task whose thread threw: its continuation never runs, and without this the task would stay registered,
    // refuse every task it conflicts with and make `stopTasks()` wait for it forever.
    void taskFailed(const TaskToken& task);

    // Closes the open tab if it is one of `selected`, which saves it, before a task rewrites those files on its own.
    // False if the tab is still open (saving failed): the task must not start.
    [[nodiscard]] auto closeTabIfSelected(const Selected& selected) -> bool;

    // Enables or disables what only makes sense with a project open and no task-dependent state
    // (`updateTaskControls()` handles the rest).
    void setProjectActionsEnabled(bool enabled);

    void addRecentProject(const QString& folder);

    // Places `panel` under `button`, in the control bar.
    void showBelow(QWidget* panel, const QWidget* button) const;

    // The message about the languages that are not set in the project settings, `what` being what they are needed for.
    [[nodiscard]] auto languagesUnsetMessage(const QString& what) const -> QString;

    void write(Selected selected);

    // A running task that rewrites the translation files, if any. A tab opened meanwhile would hold the old text of a
    // file the task has not reached yet, and save it over the result.
    [[nodiscard]] auto rewritingTask() const -> TaskToken;

    // Disables what a task would be refused for. Called when a task starts or finishes and a project opens or closes.
    void updateTaskControls();

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
        const QString& readEncoding,
        bool updateBaseline = false
    );
    [[nodiscard]] auto initDictionary() -> Result<void, QString>;

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
    void applyTranslatedDelta(const QString& tabName, i32 delta);
    void refreshRewrittenTabs(const vector<FilenameArray>& filenames);
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
    void applyCellAction(BatchAction action);
    void updateCounts();
    void showProjectStatistics();

    void warnIfGlossaryLanguagesUnset();
    void applyLintRows(vector<LintRow> rows);

    void showSettingsWindow();
    void showAboutWindow();
    void handleInputFocused();

    void setupConnections();

    void initializeSettings();

    [[nodiscard]] auto saveSettings() -> bool;
    // A failed save is already shown to the user. Callers that must stop on failure check the result.
    auto saveProjectSettings() -> bool;
    auto saveGlossary() -> bool;
    // `skipTranslations`: leave the open tab and `mapSections` alone (see `closeProject`).
    [[nodiscard]] auto saveEverything(bool skipTranslations = false) -> bool;

    [[nodiscard]] auto saveCurrentTab(QString tabName = QString()) -> bool;
    [[nodiscard]] auto saveMaps() -> bool;
    void saveBackup();

    void loadSettings();
    void loadBackup(const QString& backupPath);

    void copyToBaseline(const QString& sourcePath, const QString& baselinePath);

    HashMap<u16, QString> mapSections;

    QString lockedFile;

    // The tab whose last line is the game title, which `gameTitleInput` edits. Empty if there is none.
    QString gameTitleTab;

    unique_ptr<Settings> settings;
    unique_ptr<ProjectSettings> projectSettings;

    QTimer backupTimer;
    QTimer lintDebounceTimer;
    QTimer countsDebounceTimer;
    QTimer translateDebounceTimer;
    u32 countsGeneration = 0;
    // A result of a single lint or translation request is shown only if no newer request was made since
    u32 lintGeneration = 0;
    u32 translateGeneration = 0;

    // The running single-cell lint, which is not a task: closing the project waits for it, as it reads the settings
    QFuture<LintOutcome> singleLintFuture;

    // The running backup compression. It owns its files, so only the application exit waits for it.
    QFuture<bool> backupFuture;

    Ui::MainWindow* const ui;

    // Deletes itself when closed
    QPointer<QProgressDialog> updateProgressDialog;

    // The open "go to row" popup, so that a second one isn't opened over it
    QPointer<QWidget> goToRowPopup;

    QTranslator* translator;
    QLocale::Language installedLanguage = QLocale::AnyLanguage;

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
    QLabel* const countsStatusLabel;
    QLabel* const tabNameStatusLabel;

    FFILogger& ffiLogger;
    TaskWorker* const taskWorker;

    QAction* const actionGoToRow = new QAction(this);

    // Texts are set by `retranslateActions()`, as `tr()` here would run before the translator is installed and never
    // again.
    QAction* const actionTabPanel = new QAction(QIcon(u":/icons/menu.svg"_s), QString(), this);
    QAction* const actionSave = new QAction(QIcon(u":/icons/save.svg"_s), QString(), this);
    QAction* const actionWrite = new QAction(QIcon(u":/icons/manufacturing.svg"_s), QString(), this);
    QAction* const actionSearch = new QAction(QIcon(u":/icons/search.svg"_s), QString(), this);
    QAction* const actionBatchMenu = new QAction(QIcon(u":/icons/construction.svg"_s), QString(), this);
    QAction* const actionGlossaryMenu = new QAction(QIcon(u":/icons/book_3.svg"_s), QString(), this);
    QAction* const actionLintMenu = new QAction(QIcon(u":/icons/lab_profile.svg"_s), QString(), this);
    QAction* const actionTranslationsMenu = new QAction(QIcon(u":/icons/translate.svg"_s), QString(), this);
    QAction* const actionBookmarkMenu = new QAction(QIcon(u":/icons/bookmarks.svg"_s), QString(), this);
    QAction* const actionSourceControl = new QAction(QIcon(u":/icons/commit.svg"_s), QString(), this);
    QAction* const actionAssets = new QAction(QIcon(u":/icons/stock_media.svg"_s), QString(), this);
    QAction* const actionLocateProjectDir = new QAction(QIcon(u":/icons/folder_code.svg"_s), QString(), this);
    QAction* const actionSearchPanel = new QAction(QIcon(u":/icons/dock.svg"_s), QString(), this);
    QAction* const actionStatistics = new QAction(this);
    QAction* const actionSingleTrim = new QAction(this);
    QAction* const actionSingleWrap = new QAction(this);
    QAction* const actionSingleLatinize = new QAction(this);

    bool firstReadPending = false;

    // While `stopTasks()` waits: the continuations of the tasks it interrupted have nothing left to do.
    bool stoppingTasks = false;
};
