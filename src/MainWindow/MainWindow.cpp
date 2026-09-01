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
#include "TranslationsMenu.hpp"
#include "Utils.hpp"
#include "WriteMenu.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QCloseEvent>
#include <QTranslator>

namespace {
constexpr i32 STATUS_BAR_MARGIN = 8;
constexpr i32 STATUS_BAR_SPACING = 8;
}  // namespace

// TODO(v1.2): Display entry in search panel/bookmark menu, but make it optional
// through settings.

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
    tabNameStatusLabel(new QLabel(this)),

    ffiLogger(FFILogger::instance()),
    taskWorker(new TaskWorker(this)) {
    actionSave->setShortcut(QKeySequence::Save);
    actionWrite->setShortcut(u"Ctrl+W"_s);
    actionSearch->setShortcut(QKeySequence::Find);

    lintDebounceTimer.setSingleShot(true);

    addAction(actionSave);
    addAction(actionTabPanel);
    addAction(actionSearchPanel);
    addAction(actionGoToRow);
    addAction(actionBatchMenu);
    addAction(actionSearch);
    addAction(actionBookmarkMenu);
    addAction(actionLintMenu);

    actionTabPanel->setEnabled(false);
    actionSave->setEnabled(false);
    actionWrite->setEnabled(false);
    actionSearch->setEnabled(false);
    actionBatchMenu->setEnabled(false);
    actionGlossaryMenu->setEnabled(false);
    actionLintMenu->setEnabled(false);
    actionTranslationsMenu->setEnabled(false);
    actionBookmarkMenu->setEnabled(false);
    actionSearchPanel->setEnabled(false);
    actionSourceControl->setEnabled(false);
    actionAssets->setEnabled(false);

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
    init_lindera_dictionaries(strtoffi(qApp->property("data-location").toString().toUtf8()));

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

void MainWindow::retranslate(const QLocale::Language language) {
    settings->appearance.language = language;

    qApp->removeTranslator(translator);
    delete translator;

    translator = new QTranslator(this);
    const QString localeName = QLocale(language).bcp47Name();
    const bool success = translator->load(u":/%1.qm"_qsv.arg(QStringView(localeName).split(u'-').first()));

    if (!success) {
        present(
            this,
            NOTICE(
                "Failed to retranslate the interface. This is most like an internal application issue.",
                Warning,
                Modal
            )
        );
    } else {
        qApp->installTranslator(translator);
        ui->retranslateUi(this);
    }
}

void MainWindow::exit() {
    if (!projectSettings) {
        qApp->quit();
        return;
    }

    const auto success = saveEverything();

    if (success) {
        qApp->quit();
    }
}

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* const ui_ = new Ui::MainWindow();
    ui_->setupUi(this);
    return ui_;
};

void MainWindow::closeEvent(QCloseEvent* const event) {
    exit();
    event->ignore();
}
