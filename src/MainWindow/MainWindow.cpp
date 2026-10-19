#include "MainWindow.hpp"

#include "AssetMenu.hpp"
#include "BatchMenu.hpp"
#include "BookmarkMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "Notice.hpp"
#include "PurgeMenu.hpp"
#include "ReadMenu.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "SerdeMenu.hpp"
#include "TaskPanel.hpp"
#include "TranslationTableModel.hpp"
#include "TranslationsMenu.hpp"
#include "Utils.hpp"
#include "WriteMenu.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QCloseEvent>
#include <QFile>
#include <QMessageBox>
#include <QTranslator>

namespace {
constexpr i32 STATUS_BAR_MARGIN = 8;
constexpr i32 STATUS_BAR_SPACING = 8;
}  // namespace

MainWindow::MainWindow(QWidget* const parent) :
    QMainWindow(parent),

    ui(setupUi()),

    translator(new QTranslator(this)),

    searchMenu(new SearchMenu(this)),
    batchMenu(new BatchMenu(this)),
    glossaryMenu(new GlossaryMenu(this)),
    translationsMenu(new TranslationsMenu(this)),
    bookmarkMenu(new BookmarkMenu(this)),

    readMenu(new ReadMenu(this)),
    writeMenu(new WriteMenu(this)),
    purgeMenu(new PurgeMenu(this)),
    serdeMenu(new SerdeMenu(this)),

    assetMenu(new AssetMenu(this)),
    taskPanel(new TaskPanel(this)),

    recentProjectsMenu(new QMenu(this)),

    linesStatusLabel(new QLabel(this)),
    progressStatusLabel(new QLabel(this)),
    countsStatusLabel(new QLabel(this)),
    tabNameStatusLabel(new QLabel(this)),

    ffiLogger(FFILogger::instance()),
    taskWorker(new TaskWorker(this)) {
    retranslateActions();

    actionSave->setShortcut(QKeySequence::Save);
    actionWrite->setShortcut(WRITE_SHORTCUT.toString());
    ui->actionCloseTab->setShortcut(CLOSE_TAB_SHORTCUT.toString());
    actionSearch->setShortcut(QKeySequence::Find);

    lintDebounceTimer.setSingleShot(true);
    countsDebounceTimer.setSingleShot(true);

    addAction(actionSave);
    addAction(actionTabPanel);
    addAction(actionSearchPanel);
    addAction(actionGoToRow);
    addAction(actionBatchMenu);
    addAction(actionSearch);
    addAction(actionBookmarkMenu);
    addAction(actionLintMenu);
    ui->menuFile->insertAction(ui->actionCloseProject, actionStatistics);

    addAction(actionSingleTrim);
    addAction(actionSingleWrap);
    addAction(actionSingleLatinize);

    translateDebounceTimer.setSingleShot(true);

    setProjectActionsEnabled(false);
    updateTaskControls();

    ui->tabPanelButton->setDefaultAction(actionTabPanel);
    ui->saveButton->setDefaultAction(actionSave);
    ui->writeButton->setDefaultAction(actionWrite);
    ui->openFolderButton->setDefaultAction(ui->actionOpenFolder);
    ui->searchButton->setDefaultAction(actionSearch);
    ui->batchButton->setDefaultAction(actionBatchMenu);
    ui->glossaryButton->setDefaultAction(actionGlossaryMenu);
    ui->lintMenuButton->setDefaultAction(actionLintMenu);
    ui->translationsButton->setDefaultAction(actionTranslationsMenu);
    ui->bookmarksButton->setDefaultAction(actionBookmarkMenu);
    ui->sourceControlButton->setDefaultAction(actionSourceControl);
    ui->assetsButton->setDefaultAction(actionAssets);
    ui->locateProjectDirButton->setDefaultAction(actionLocateProjectDir);
    ui->searchPanelButton->setDefaultAction(actionSearchPanel);

    init_rust_logger(&FFILogger::rustLogCallback);
    init_lindera_dictionaries(strtoffi(dataLocation().toUtf8()));

#ifdef ENABLE_LIBGIT2
    ui->sourceControlDock->init(*ui);
#endif

    ui->searchPanel->init(ui->fileSelect, ui->searchResultList, ui->clearSearchPanelButton);
    ui->searchPanel->hide();
    ui->tabPanel->hide();
    ui->sourceControlDock->hide();

    auto* const statusBarPermanentWidget = new QWidget(ui->statusBar);
    auto* const statusBarPermanentWidgetLayout = new QHBoxLayout(statusBarPermanentWidget);
    statusBarPermanentWidgetLayout->setContentsMargins(STATUS_BAR_MARGIN, 0, STATUS_BAR_MARGIN, 0);
    statusBarPermanentWidgetLayout->setSpacing(STATUS_BAR_SPACING);

    statusBarPermanentWidgetLayout->addWidget(linesStatusLabel);
    statusBarPermanentWidgetLayout->addWidget(progressStatusLabel);
    statusBarPermanentWidgetLayout->addWidget(countsStatusLabel);
    statusBarPermanentWidgetLayout->addWidget(tabNameStatusLabel);
    ui->statusBar->addPermanentWidget(statusBarPermanentWidget);

    ui->actionRecentProjects->setMenu(recentProjectsMenu);

    initializeSettings();
    setupConnections();

    checkForUpdates();

    if (!settings->core.projectPath.isEmpty()) {
        openProject(settings->core.projectPath, false);
    } else {
        ui->statusBar->showMessage(tr("Open a project by using 'Open Folder' button!"));
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::retranslate(QLocale::Language language) {
    // Every closed settings window comes here, mostly with the language that is installed already
    if (language == installedLanguage) {
        return;
    }

    auto* const newTranslator = new QTranslator(this);
    const QString localeName = QLocale(language).bcp47Name();
    QString catalog = u":/%1.qm"_qsv.arg(QStringView(localeName).split(u'-').first());

    // The default language is that of the system, which the program may not be translated into
    if (!QFile::exists(catalog)) {
        language = QLocale::English;
        catalog = u":/en.qm"_s;
    }

    if (!newTranslator->load(catalog)) {
        delete newTranslator;

        present(
            this,
            NOTICE(
                "Failed to retranslate the interface. This is most like an internal application issue.",
                Warning,
                Modal
            )
        );
        return;
    }

    settings->appearance.language = language;
    installedLanguage = language;

    qApp->removeTranslator(translator);
    delete translator;

    translator = newTranslator;

    // Installing it sends `LanguageChange` to the window, which is what retranslates it (see `changeEvent`)
    qApp->installTranslator(translator);
}

void MainWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        // The placeholder of the title input holds the source title while a project is open, and `retranslateUi`
        // would put "Game Title" back over it, which then gets saved as the title
        const QString titleSource = ui->gameTitleInput->placeholderText();

        ui->retranslateUi(this);

        if (projectSettings != nullptr) {
            ui->gameTitleInput->setPlaceholderText(titleSource);
        }

        retranslateActions();
        updateTaskButton(taskPanel->taskCount());
        updateStatusLabels();
        updateCounts();
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateActions() {
    actionTabPanel->setText(tr("Tab Panel"));
    actionSave->setText(tr("Save"));
    actionWrite->setText(tr("Write"));
    actionSearch->setText(tr("Search"));
    actionBatchMenu->setText(tr("Batch Menu"));
    actionGlossaryMenu->setText(tr("Glossary Menu"));
    actionLintMenu->setText(tr("Lint Menu"));
    actionTranslationsMenu->setText(tr("Translations Menu"));
    actionBookmarkMenu->setText(tr("Bookmark Menu"));
    actionSourceControl->setText(tr("Source Control"));
    actionAssets->setText(tr("Assets"));
    actionLocateProjectDir->setText(tr("Locate Project Directory"));
    actionSearchPanel->setText(tr("Search Panel"));
    actionStatistics->setText(tr("Project Statistics"));
    actionSingleTrim->setText(tr("Trim Cell"));
    actionSingleWrap->setText(tr("Wrap Cell"));
    actionSingleLatinize->setText(tr("Latinize Cell"));
}

void MainWindow::updateTaskButton(const i32 count) {
    ui->taskButton->setEnabled(count != 0);
    ui->taskButton->setText(count == 0 ? tr("No tasks") : tr("%n task(s)", nullptr, count));
}

void MainWindow::updateStatusLabels() {
    const QString tabName = ui->tabPanel->currentTabName();

    if (tabName.isEmpty()) {
        linesStatusLabel->clear();
        progressStatusLabel->clear();
        tabNameStatusLabel->clear();
        return;
    }

    const i32 rowCount = ui->translationTable->model()->rowCount();
    const i32 total = ui->tabPanel->currentTotal();

    linesStatusLabel->setText(tr("%1 Lines / %2 Comments").arg(qsv(qitos(rowCount)), qsv(qitos(rowCount - total))));
    progressStatusLabel->setText(
        tr("%1 Translated / %2 Total").arg(qsv(qitos(ui->tabPanel->currentTranslated())), qsv(qitos(total)))
    );
    tabNameStatusLabel->setText(tabName);
}

void MainWindow::exit() {
    if (prepareToQuit()) {
        qApp->quit();
    }
}

auto MainWindow::prepareToQuit() -> bool {
    // A task that can't be interrupted keeps running after the window is gone, until the process ends with it
    if (const TaskToken blocking = taskWorker->uninterruptibleTask(); blocking != nullptr) {
        const auto answer = QMessageBox::question(
            this,
            tr("A task is running"),
            tr("%1 is still running. Quit anyway?").arg(taskName(blocking->kind())),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (answer != QMessageBox::Yes) {
            return false;
        }

        qInfo().noquote() << u"Quitting while a task is running was confirmed by user."_qsv;
        taskWorker->interruptAll();
    } else if (!stopTasks()) {
        return false;
    }

    // Also saves the settings when no project is open: `closeProject` clears the path of the project in memory only
    return saveEverything();
}

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* const ui_ = new Ui::MainWindow();
    ui_->setupUi(this);
    return ui_;
}

void MainWindow::closeEvent(QCloseEvent* const event) {
    exit();
    event->ignore();
}
