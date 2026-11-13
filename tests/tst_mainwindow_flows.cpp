// What the main window does with a game: writing translations into it, and replacing text across the translation
// files. Driven the way a user does it, through the actions and menus of the window.
#include "Permissions.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"
#include "FileSelectMenu.hpp"
#include "ReadMenu.hpp"
#include "BackupSelectDialog.hpp"
#include "BatchMenu.hpp"
#include "LintMenu.hpp"
#include "LintTable.hpp"
#include "PopupInput.hpp"
#include "PurgeMenu.hpp"
#include "SerdeMenu.hpp"
#include "TaskWorker.hpp"
#include "WriteMenu.hpp"
#include "SearchMenu.hpp"
#include "StatisticsDialog.hpp"
#include "scripts.h"

#include <QStatusBar>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QScrollBar>
#include <QThreadPool>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

void writeActorsData(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    // A write goes through the maps too, and these are listed in MapInfos
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
}

[[nodiscard]] auto outputOf(const Env& env, const QString& file) -> QByteArray {
    return readFile(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + u"/output/data/"_s + file);
}

[[nodiscard]] auto actionNamed(const MainWindow& window, const QString& text) -> QAction* {
    for (QAction* const action : window.findChildren<QAction*>()) {
        if (action->text() == text) {
            return action;
        }
    }

    return nullptr;
}

// The actions without a text are the ones that only have a shortcut
[[nodiscard]] auto actionWithShortcut(const MainWindow& window, const QString& shortcut) -> QAction* {
    for (QAction* const action : window.findChildren<QAction*>()) {
        if (action->text().isEmpty() && action->shortcut() == QKeySequence(shortcut)) {
            return action;
        }
    }

    return nullptr;
}

void replaceAll(const MainWindow& window, const QString& what, const QString& with) {
    Selected selected;
    selected.flags = FileFlags_Actors;

    emit window.findChild<SearchMenu*>()->actionRequested(
        selected,
        SearchAction::Replace,
        what,
        with,
        SearchLocation::Translation,
        -1,
        static_cast<SearchFlags>(0)
    );
}

[[nodiscard]] auto workerOf(const MainWindow& window) -> TaskWorker* {
    return window.findChild<TaskWorker*>();
}

void trimAll(const MainWindow& window) {
    Selected selected;
    selected.flags = FileFlags_Actors;

    const JSScript script{ .action = BatchAction::Trim, .name = u"Trim"_s, .source = TRIM_SCRIPT.toString() };

    emit window.findChild<BatchMenu*>()->actionRequested(
        script,
        selected,
        1,
        BatchVariant(LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace)
    );
}

constexpr auto TWO_ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"},{"id":2,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Alice","nickname":"","note":"","profile":""}])";

// Ticks `name` in the file list of the read menu. The list is a sibling of the menu, as are the lists of the other
// menus: the one whose box changes what the read menu selects is its own.
void tickForReading(const MainWindow& window, const ReadMenu& menu, const QString& name) {
    const FileFlags before = menu.selected(true).flags;

    for (FileSelectMenu* const list : window.findChildren<FileSelectMenu*>()) {
        for (QCheckBox* const box : list->findChildren<QCheckBox*>()) {
            if (box->text() != name) {
                continue;
            }

            box->setChecked(true);

            if (menu.selected(true).flags != before) {
                return;
            }

            box->setChecked(false);
        }
    }
}

// Presses "Read" for the file `name` with the mode at `modeIndex` (0 default, 1 force, 2 append, 3 force append). The
// files that are ticked are the ones that read.
void read(const MainWindow& window, const i32 modeIndex, const QString& name = u"actors"_s) {
    auto* const menu = window.findChild<ReadMenu*>();
    tickForReading(window, *menu, name);

    menu->findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(modeIndex);
    emit menu->accepted();
}
}  // namespace

class TestMainWindowFlows final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void init() { catcher = make_unique<ModalCatcher>(); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();

        if (catcher != nullptr) {
            QVERIFY2(catcher->text.isEmpty(), qPrintable(catcher->text));
            catcher.reset();
        }
    }

    void writePutsTheTranslationsIntoACopyOfTheGame() {
        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(actionNamed(window, u"Write"_s) != nullptr);
        actionNamed(window, u"Write"_s)->trigger();

        QTRY_VERIFY_WITH_TIMEOUT(!outputOf(env, u"Actors.json"_s).isEmpty(), 15000);

        const QByteArray output = outputOf(env, u"Actors.json"_s);
        QVERIFY(output.contains("Гарольд"));
        QVERIFY(output.contains("Храбрый"));
        QVERIFY(output.contains("A hero"));
        // The game itself is not touched
        QVERIFY(readFile(env.project.path() + u"/data/Actors.json"_s).contains("Harold"));
    }

    void writeTakesTheEditsOfTheOpenTabAlong() {
        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        // The two comment lines take the first two rows
        edit(window, 2, 1, u"Гарольд"_s);

        actionNamed(window, u"Write"_s)->trigger();

        QTRY_VERIFY_WITH_TIMEOUT(outputOf(env, u"Actors.json"_s).contains("Гарольд"), 15000);
        // The translation file holds the edit too, as the write read it from there
        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд"));
    }

    void writeIsRefusedWhenTheGameIsNotThere() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        // The game goes away while the project is open
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        // The copy of the game that the program keeps is what is written from, when there is one
        QVERIFY(QDir(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + BASELINE_DATA_DIRECTORY.toString()).removeRecursively());
        actionNamed(window, u"Write"_s)->trigger();

        QTRY_COMPARE_WITH_TIMEOUT(catcher->text, u"Cannot write, source files are absent."_s, 15000);
        // Said, and expected: the check in cleanup() is for the unexpected
        catcher->text.clear();
    }

    void aTaskThatThrowsIsReportedAndReleased() {
        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        workerOf(window)->writeHook = [] -> void { throw std::runtime_error("boom"); };
        actionNamed(window, u"Write"_s)->trigger();

        QTRY_COMPARE_WITH_TIMEOUT(catcher->text, u"Write failed because of an internal error."_s, 15000);
        // The task is not left registered, or every task it conflicts with would be refused forever
        QVERIFY(workerOf(window)->idle());
        catcher->text.clear();
    }

    void replaceAllRewritesTheTranslationFilesOnceConfirmed() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Hero one\nAlice<#>Hero two\nBob<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        catcher.reset();
        const ModalCatcher confirmation(QMessageBox::Yes);

        replaceAll(window, u"Hero"_s, u"Герой"_s);

        QTRY_COMPARE_WITH_TIMEOUT(
            env.readTranslation(u"actors"_s).trimmed(),
            QByteArray("Harold<#>Герой one\nAlice<#>Герой two\nBob<#>"),
            15000
        );
        QVERIFY2(confirmation.text.contains(u"2 cell(s) of 1 file"_qsv), qPrintable(confirmation.text));
    }

    void replaceAllChangesNothingWhenDeclined() {
        const Env env;
        const QByteArray original = "Harold<#>Hero one\nAlice<#>Hero two\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        catcher.reset();
        const ModalCatcher confirmation(QMessageBox::No);

        replaceAll(window, u"Hero"_s, u"Герой"_s);

        QTRY_VERIFY_WITH_TIMEOUT(!confirmation.text.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
    }

    // What the three of them guard: an open tab whose edits cannot be saved must not have its file rewritten under it,
    // as the editor would write its own text over the result
    void aReplaceIsRefusedWhenTheOpenTabCannotBeSaved() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        const QByteArray original = "Harold<#>Hero one\nAlice<#>Hero two\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 0, 1, u"Hero edited"_s);
        const QString file = env.translationFile(u"actors"_s);
        // Restored before the window goes: closing it saves the tab again, and a box nobody answers would hold it
        const auto restore = qScopeGuard([&file] -> void {
            QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        });
        QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner));

        catcher.reset();
        const ModalCatcher boxes(QMessageBox::Yes);
        replaceAll(window, u"Hero"_s, u"Герой"_s);

        // The tab is saved before the search, so the refusal comes first and nothing is asked
        QTRY_VERIFY_WITH_TIMEOUT(!boxes.seen.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(boxes.seen.size(), qsizetype{ 1 });
        QVERIFY(!boxes.seen.first().contains(u"Replace the text"_qsv));
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Hero edited"_s);

    }

    void aBatchScriptThatCannotSaveTheOpenTabChangesNothing() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        const QByteArray original = "Harold<#>\nAlice<#>Алиса\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 0, 1, u"  Гарольд  "_s);
        const QString file = env.translationFile(u"actors"_s);
        // Restored before the window goes: closing it saves the tab again, and a box nobody answers would hold it
        const auto restore = qScopeGuard([&file] -> void {
            QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        });
        QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner));

        catcher.reset();
        const ModalCatcher boxes;
        trimAll(window);

        QTRY_VERIFY_WITH_TIMEOUT(!boxes.seen.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"  Гарольд  "_s);

    }

    void aWriteThatCannotSaveTheOpenTabWritesNothing() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        writeActorsData(env);
        const QByteArray original = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Гарольд"_s);
        const QString file = env.translationFile(u"actors"_s);
        // Restored before the window goes: closing it saves the tab again, and a box nobody answers would hold it
        const auto restore = qScopeGuard([&file] -> void {
            QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        });
        QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner));

        catcher.reset();
        const ModalCatcher boxes;
        actionNamed(window, u"Write"_s)->trigger();

        QTRY_VERIFY_WITH_TIMEOUT(!boxes.seen.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QVERIFY(outputOf(env, u"Actors.json"_s).isEmpty());
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(*tableOf(window)->model()->item(2, 1), u"Гарольд"_s);

    }

    // Purging and the serde export and import rewrite or read the translation files, so they save the open tab first: a tab
    // that cannot be saved ends them before they start, and frees the task slot
    void aTaskIsNotStartedWhenTheOpenTabCannotBeSaved_data() {
        QTest::addColumn<i32>("entry");

        QTest::newRow("purge") << 0;
        QTest::newRow("serde export") << 1;
        QTest::newRow("serde import") << 2;
        QTest::newRow("statistics") << 3;
        QTest::newRow("read") << 4;
    }

    void aTaskIsNotStartedWhenTheOpenTabCannotBeSaved() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        QFETCH(const i32, entry);

        const Env env;
        writeActorsData(env);
        const QByteArray original = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Гарольд"_s);
        const QString file = env.translationFile(u"actors"_s);
        // Restored before the window goes: closing it saves the tab again, and a box nobody answers would hold it
        const auto restore = qScopeGuard([&file] -> void {
            QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        });
        QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner));

        catcher.reset();
        // Yes to the confirmation of the purge and of the import; the refusal is a box of its own
        const ModalCatcher boxes(QMessageBox::Yes);

        switch (entry) {
            case 0:
                emit window.findChild<PurgeMenu*>()->accepted();
                break;
            case 1:
                emit window.findChild<SerdeMenu*>()->exportRequested();
                break;
            case 2:
                emit window.findChild<SerdeMenu*>()->importRequested();
                break;
            case 3:
                actionNamed(window, u"Project Statistics"_s)->trigger();
                break;
            default:
                read(window, 3);
                break;
        }

        QTRY_VERIFY_WITH_TIMEOUT(!boxes.seen.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QVERIFY(window.findChild<TaskWorker*>()->idle());
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
        QCOMPARE(*tableOf(window)->model()->item(2, 1), u"Гарольд"_s);
    }

    // Every one of them rewrites or reads the translation files, which the running task already uses
    void aTaskIsRefusedWhileAnotherOneRuns_data() {
        QTest::addColumn<i32>("entry");

        QTest::newRow("write") << 0;
        QTest::newRow("purge") << 1;
        QTest::newRow("serde export") << 2;
        QTest::newRow("serde import") << 3;
        QTest::newRow("batch script") << 4;
        QTest::newRow("global lint") << 6;
        QTest::newRow("read") << 8;
    }

    void aTaskIsRefusedWhileAnotherOneRuns() {
        QFETCH(const i32, entry);

        const Env env;
        writeActorsData(env);
        const QByteArray original = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        const auto held = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(held.has_value());
        // Closing the project waits for the task, so it ends first even when the test fails
        const auto release = qScopeGuard([&window, &held] -> void { workerOf(window)->finishTask(*held); });

        catcher.reset();
        // Yes to the confirmation of the purge and of the import
        const ModalCatcher boxes(QMessageBox::Yes);

        switch (entry) {
            case 0:
                // The action of the menu is disabled by now, so the menu is the way in
                emit window.findChild<WriteMenu*>()->accepted();
                break;
            case 1:
                emit window.findChild<PurgeMenu*>()->accepted();
                break;
            case 2:
                emit window.findChild<SerdeMenu*>()->exportRequested();
                break;
            case 3:
                emit window.findChild<SerdeMenu*>()->importRequested();
                break;
            case 6: {
                Selected selected;
                selected.flags = FileFlags_Actors;
                emit window.findChild<LintMenu*>()->checkRequested(selected);
                break;
            }
            case 8:
                read(window, 3);
                break;
            default:
                trimAll(window);
                break;
        }

        QThreadPool::globalInstance()->waitForDone();
        QVERIFY(!boxes.seen.isEmpty());
        QVERIFY2(boxes.seen.last().contains(u"Cannot start"_qsv), qPrintable(boxes.seen.last()));
        // The task that was running is still the only one, and nothing was written
        QCOMPARE(workerOf(window)->runningTask(TaskKind::Replace), *held);
        QVERIFY(outputOf(env, u"Actors.json"_s).isEmpty());
        QCOMPARE(env.readTranslation(u"actors"_s), original);
    }

    // Statistics only read, so another kind of task does not stop them; one of their own kind does
    void theStatisticsAreRefusedWhileOthersAreBeingCounted() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        const auto held = workerOf(window)->startTask(TaskKind::Statistics);
        QVERIFY(held.has_value());
        const auto release = qScopeGuard([&window, &held] -> void { workerOf(window)->finishTask(*held); });

        catcher.reset();
        const ModalCatcher boxes(QMessageBox::Yes);
        // The action is disabled by now: the signal is what a shortcut or a queued click would still send
        emit actionNamed(window, u"Project Statistics"_s)->triggered();

        QVERIFY(!boxes.seen.isEmpty());
        QVERIFY2(boxes.seen.last().contains(u"Cannot start"_qsv), qPrintable(boxes.seen.last()));
        QVERIFY(window.findChild<StatisticsDialog*>() == nullptr);
    }

    // The project closes while the task runs: its result is dropped, as `openProject` or a notice would run inside the
    // close that waits for it
    void closingTheProjectDropsTheResultOfARunningTask_data() {
        QTest::addColumn<i32>("entry");

        QTest::newRow("serde export") << 0;
        QTest::newRow("serde import") << 1;
        QTest::newRow("batch script") << 2;
        QTest::newRow("replace search") << 3;
        QTest::newRow("statistics") << 4;
        QTest::newRow("global lint") << 5;
    }

    void closingTheProjectDropsTheResultOfARunningTask() {
        QFETCH(const i32, entry);

        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#> Harry \nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        catcher.reset();
        const ModalCatcher boxes(QMessageBox::Yes);

        switch (entry) {
            case 0:
                emit window.findChild<SerdeMenu*>()->exportRequested();
                break;
            case 1:
                emit window.findChild<SerdeMenu*>()->importRequested();
                break;
            case 2:
                trimAll(window);
                break;
            case 4:
                actionNamed(window, u"Project Statistics"_s)->trigger();
                break;
            case 5: {
                Selected selected;
                selected.flags = FileFlags_Actors;
                emit window.findChild<LintMenu*>()->checkRequested(selected);
                break;
            }
            default:
                replaceAll(window, u"Harry"_s, u"Garry"_s);
                break;
        }

        // The question of the import is answered by now; what the task would say or ask afterwards is not
        const qsizetype asked = boxes.seen.size();
        QVERIFY(!workerOf(window)->idle());

        closeProject(window);

        QVERIFY(workerOf(window)->idle());
        QCOMPARE(tabsOf(window)->tabCount(), 0);
        QCOMPARE(boxes.seen.size(), asked);
        QVERIFY(window.findChild<StatisticsDialog*>() == nullptr);
    }

    void aBackupIsNotLoadedWhileATaskRuns() {
        const Env env;
        const QByteArray original = "Harold<#>Гарольд\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        // The dialog is open before the task starts: the action is disabled while one runs
        window.findChild<QAction*>(u"actionLoadBackup"_s)->trigger();
        auto* const dialog = window.findChild<BackupSelectDialog*>();
        QVERIFY(dialog != nullptr);

        const auto held = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(held.has_value());
        const auto release = qScopeGuard([&window, &held] -> void { workerOf(window)->finishTask(*held); });

        emit dialog->accepted();

        QVERIFY2(catcher->text.contains(u"Cannot load a backup while a task is running"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(tabsOf(window)->tabCount(), 1);
    }

    // The counters of the tabs are worked out again for the files the script rewrote, and only for those
    void aBatchScriptOnOneFileLeavesTheTabsOfTheOthersAlone() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#> Гарольд \nAlice<#>\n");
        env.writeTranslation(u"items"_s, "Potion<#> Зелье \nElixir<#>");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabCount(), 2);
        // The script runs on the second tab, which is found after the one of the other file
        const bool actorsSecond = tabsOf(window)->tabName(1) == u"actors"_s;
        const QString rewritten = actorsSecond ? u"actors"_s : u"items"_s;
        const QString untouched = actorsSecond ? u"items"_s : u"actors"_s;
        const QByteArray untouchedContent = env.readTranslation(untouched);

        Selected selected;
        selected.flags = actorsSecond ? FileFlags_Actors : FileFlags_Items;
        const JSScript script{ .action = BatchAction::Trim, .name = u"Trim"_s, .source = TRIM_SCRIPT.toString() };
        emit window.findChild<BatchMenu*>()->actionRequested(
            script,
            selected,
            1,
            BatchVariant(LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace)
        );

        QTRY_VERIFY_WITH_TIMEOUT(!env.readTranslation(rewritten).contains(" "), 15000);
        QThreadPool::globalInstance()->waitForDone();
        // Both files hold one translated row and one empty one; the counts follow when the event loop delivers the result
        QTRY_COMPARE(tabsOf(window)->tabTranslated(0), 1);
        QTRY_COMPARE(tabsOf(window)->tabTranslated(1), 1);
        QCOMPARE(env.readTranslation(untouched), untouchedContent);
    }

    void aPurgeThatTheCoreRefusesIsReportedAndFreesTheTaskSlot() {
        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\nHarold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        // The game goes away while the project is open, as in the refused write
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        QVERIFY(QDir(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + BASELINE_DATA_DIRECTORY.toString()).removeRecursively());

        catcher.reset();
        const ModalCatcher boxes(QMessageBox::Yes);
        emit window.findChild<PurgeMenu*>()->accepted();

        QTRY_VERIFY_WITH_TIMEOUT(workerOf(window)->idle() && boxes.seen.size() == 2, 15000);
        QVERIFY2(boxes.seen.last().contains(u"Purge failed"_qsv), qPrintable(boxes.seen.last()));
    }

    void replaceAllSaysSoWhenThereIsNothingToReplace() {
        const Env env;
        const QByteArray original = "Harold<#>Hero one\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);

        replaceAll(window, u"nothing like this"_s, u"x"_s);

        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains(u"Nothing found to replace"_qsv), 5000);
        QCOMPARE(env.readTranslation(u"actors"_s), original);
    }

    void replacementsAreCountedInTheProgressOfTheTab() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\nAlice<#>Hero two\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabTranslated(0), 1);

        catcher.reset();
        const ModalCatcher confirmation(QMessageBox::Yes);

        // Clearing a translation takes it out of the count
        replaceAll(window, u"Hero two"_s, QString());

        QTRY_COMPARE_WITH_TIMEOUT(tabsOf(window)->tabTranslated(0), 0, 15000);
    }

    // The search ends and the question is asked before the replacement takes its own slot: another task can have
    // taken it by the time the answer is Yes
    void aReplacementIsRefusedWhenAnotherTaskTookTheSlotDuringTheQuestion() {
        const Env env;
        const QByteArray original = "Harold<#>Hero one\nAlice<#>Hero two\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        catcher.reset();

        decltype(workerOf(window)->startTask(TaskKind::Replace)) held = std::unexpected(TaskKind::Replace);
        const auto release = qScopeGuard([&window, &held] -> void {
            if (held.has_value()) {
                workerOf(window)->finishTask(*held);
            }
        });
        ModalCatcher boxes(QMessageBox::Yes);
        boxes.beforeAnswer = [&window, &held, &boxes] -> void {
            // Only the question of the replacement; the refusal that follows is answered as it comes
            if (!held.has_value() && boxes.seen.size() == 1) {
                held = workerOf(window)->startTask(TaskKind::Replace);
            }
        };

        replaceAll(window, u"Hero"_s, u"Герой"_s);

        QTRY_VERIFY_WITH_TIMEOUT(boxes.seen.size() == 2, 15000);
        QThreadPool::globalInstance()->waitForDone();
        QVERIFY(boxes.seen.first().contains(u"Replace the text"_qsv));
        QVERIFY2(boxes.seen.last().contains(u"Cannot start"_qsv), qPrintable(boxes.seen.last()));
        QCOMPARE(env.readTranslation(u"actors"_s), original);
    }

    // The tab of a file the replacement rewrites is closed first, and it stays open when its edits cannot be saved
    void aReplacementIsRefusedWhenTheOpenTabCannotBeClosedAfterTheQuestion() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        const QByteArray original = "Harold<#>Hero one\nAlice<#>Hero two\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        const QString file = env.translationFile(u"actors"_s);
        const auto restore = qScopeGuard([&file] -> void {
            QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        });

        catcher.reset();
        ModalCatcher boxes(QMessageBox::Yes);
        // The tab is opened and edited after the search saved it, so the save that closes it has something to write
        boxes.beforeAnswer = [&window, &file, &boxes] -> void {
            if (boxes.seen.size() != 1) {
                return;
            }

            tabsOf(window)->changeTab(u"actors"_s);
            edit(window, 0, 1, u"Hero edited"_s);
            QFile::setPermissions(file, QFileDevice::ReadOwner);
        };

        replaceAll(window, u"Hero"_s, u"Герой"_s);

        QTRY_VERIFY_WITH_TIMEOUT(boxes.seen.size() >= 2 && workerOf(window)->idle(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
    }

    void readAddsNewTextToATranslationAndKeepsWhatWasTranslated() {
        const Env env;
        writeFile(env.project.path() + u"/data/Actors.json"_s, TWO_ACTORS_DATA);
        writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        read(window, 3);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Alice<#>"), 15000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд"));
        // The project is open again on the new files
        QTRY_COMPARE_WITH_TIMEOUT(tabsOf(window)->tabCount(), 1, 15000);
    }

    void aForcedReadRewritesTheTranslationOnceConfirmed() {
        const Env env;
        writeActorsData(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        catcher.reset();
        const ModalCatcher confirmation(QMessageBox::Yes);
        read(window, 1);

        QTRY_VERIFY_WITH_TIMEOUT(!env.readTranslation(u"actors"_s).contains("Гарольд"), 15000);
        QVERIFY(confirmation.text.contains(u"rewrites"_qsv));
        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>"));
    }

    void aForcedReadChangesNothingWhenDeclined() {
        const Env env;
        writeActorsData(env);
        const QByteArray original = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        catcher.reset();
        const ModalCatcher confirmation(QMessageBox::No);
        read(window, 1);

        QTRY_VERIFY_WITH_TIMEOUT(!confirmation.text.isEmpty(), 15000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
    }

    void aReadOfAGameWithoutDataFilesFailsAndSaysWhy() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        read(window, 3);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.startsWith(u"Read failed"_qsv), 15000);
        catcher->text.clear();
        // What was translated is still there
        QCOMPARE(env.readTranslation(u"actors"_s), QByteArray("Harold<#>Гарольд\n"));
    }

    void aProjectWhoseProgramFolderIsGoneIsNotOpened() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();
        QVERIFY(QDir(env.project.path() + PROGRAM_DATA_DIRECTORY.toString()).removeRecursively());

        const MainWindow window;
        const WindowGuard guard(window);

        QVERIFY2(catcher->text.contains(u"couldn't locate"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void aBatchScriptRewritesTheTranslationsOfAFileAndTheirCounts() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>  Гарольд \nAlice<#>   \nBob<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        // A cell of whitespace is not a translation
        QCOMPARE(tabsOf(window)->tabTranslated(0), 1);

        trimAll(window);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд\n"), 15000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Alice<#>\n"));
        QCOMPARE(tabsOf(window)->tabTranslated(0), 1);
    }

    void aBatchScriptSavesTheOpenTabBeforeItRewritesTheFile() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\nAlice<#>Алиса\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 0, 1, u"  Гарольд  "_s);

        trimAll(window);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд\n"), 15000);
        // Saved first, then trimmed: the edit was not lost under the rewrite
        QVERIFY(env.readTranslation(u"actors"_s).contains("Alice<#>Алиса"));
        QCOMPARE(tabsOf(window)->currentTabName(), QString());
    }

    void trimmingACellChangesOnlyTheCell() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\nAlice<#>Алиса\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 0, 1, u"  Гарольд  "_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(0, 1));

        actionNamed(window, u"Trim Cell"_s)->trigger();

        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Гарольд"_s);
        QCOMPARE(*tableOf(window)->model()->item(1, 1), u"Алиса"_s);
    }

    void wrappingACellNeedsALineLengthHint() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Hello there\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(0, 1));
        catcher.reset();
        const ModalCatcher refusal;

        actionNamed(window, u"Wrap Cell"_s)->trigger();

        QVERIFY(refusal.text.contains(u"line length hint"_qsv));
        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Hello there"_s);
    }

    void trimmingACellThatNeedsNoTrimmingChangesNothing() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(0, 1));

        actionNamed(window, u"Trim Cell"_s)->trigger();

        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Гарольд"_s);
    }

    void trimmingTheTextOfTheOpenEditorChangesTheEditor() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>x\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        const QModelIndex index = tableOf(window)->model()->index(0, 1);
        tableOf(window)->setCurrentIndex(index);
        tableOf(window)->edit(index);
        QVERIFY(tableOf(window)->activeInput() != nullptr);
        tableOf(window)->activeInput()->setPlainText(u"  Гарольд  "_s);

        actionNamed(window, u"Trim Cell"_s)->trigger();

        QCOMPARE(tableOf(window)->activeInput()->toPlainText(), u"Гарольд"_s);
    }

    void trimmingTheSourceCellIsRefused() {
        const Env env;
        env.writeTranslation(u"actors"_s, " Harold <#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(0, 0));

        actionNamed(window, u"Trim Cell"_s)->trigger();

        QCOMPARE(*tableOf(window)->model()->item(0, 0), u" Harold "_s);
    }

    void removingARowTakesItOutOfTheCounts() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\nAlice<#>\nBob<#>Боб\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        auto* const bar = window.findChild<QProgressBar*>(u"globalProgressBar"_s);
        QCOMPARE(bar->maximum(), 3);
        QCOMPARE(bar->value(), 2);

        // A translated row, then one without translation
        QVERIFY(tableOf(window)->model()->removeRows(0, 1));
        QCOMPARE(bar->maximum(), 2);
        QCOMPARE(bar->value(), 1);
        QCOMPARE(tabsOf(window)->tabTotal(0), 2);
        QCOMPARE(tabsOf(window)->tabTranslated(0), 1);

        QVERIFY(tableOf(window)->model()->removeRows(0, 1));
        QCOMPARE(bar->maximum(), 1);
        QCOMPARE(bar->value(), 1);
    }

    void theStatisticsOfTheProjectAreShownInADialog() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\nAlice<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        actionNamed(window, u"Project Statistics"_s)->trigger();

        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<StatisticsDialog*>() != nullptr, 15000);
        QVERIFY(window.findChild<StatisticsDialog*>()->isVisible());
    }

    // The lint of a cell clears the results before it runs: with no cell being edited it does not run, and does not touch them
    void typingWithoutAnEditorOpenLeavesTheLintResultsAlone() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const table = window.findChild<LintTable*>();
        QVERIFY(table != nullptr);

        table->appendRow(LintRow{ .filename = u"actors"_s, .sourceText = u"s"_s, .translationText = u"t"_s, .info = u"i"_s, .lineNumber = 1 });
        QCOMPARE(table->model()->rowCount(), 1);

        emit tableOf(window)->textChanged();
        // Past the debounce of the lint
        QTest::qWait(300);

        QCOMPARE(table->model()->rowCount(), 1);
    }

    void aGlobalLintFindsTagsThatDoNotMatch() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Hello \\C[2]x<#>Привет x\nFine<#>Хорошо\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const table = window.findChild<LintTable*>();
        QVERIFY(table != nullptr);
        QCOMPARE(table->model()->rowCount(), 0);

        Selected selected;
        selected.flags = FileFlags_Actors;
        emit window.findChild<LintMenu*>()->checkRequested(selected);

        // The one line whose tags differ, and not the fine one
        QTRY_COMPARE_WITH_TIMEOUT(table->model()->rowCount(), 1, 15000);
    }

    void goingToARowNeedsAnOpenTab() {
        const Env env;
        env.writeTranslation(u"actors"_s, "A<#>1\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        {
            QAction* const goToRow = actionWithShortcut(window, Settings().controls.goToRow);
            QVERIFY(goToRow != nullptr);
            goToRow->trigger();
        }

        QVERIFY(window.findChild<PopupInput*>() == nullptr);
    }

    void theGoToRowPopupScrollsAndGoesAway() {
        const Env env;
        // More rows than fit on the screen, so that going to the last one has to scroll
        QByteArray rows;
        for (i32 row = 1; row <= 300; row++) {
            rows += "Row" + QByteArray::number(row) + "<#>" + QByteArray::number(row) + "\n";
        }
        env.writeTranslation(u"actors"_s, rows);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);

        tabsOf(window)->changeTab(u"actors"_s);
        {
            QAction* const goToRow = actionWithShortcut(window, Settings().controls.goToRow);
            QVERIFY(goToRow != nullptr);
            goToRow->trigger();
        }

        auto* const popup = window.findChild<PopupInput*>();
        QVERIFY(popup != nullptr);
        QCOMPARE(tableOf(window)->verticalScrollBar()->value(), 0);
        popup->setText(u"300"_s);
        emit popup->editingFinished();

        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<PopupInput*>() == nullptr, 3000);
        QVERIFY(tableOf(window)->verticalScrollBar()->value() > 0);
    }

    void aNewProjectIsReadFromTheGameOnceTheReadMenuIsAccepted() {
        const Env env;
        writeActorsData(env);
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabCount(), 0);

        answerFirstRead(window, true);
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold"), 15000);
        QTRY_COMPARE_WITH_TIMEOUT(tabsOf(window)->tabName(0), u"actors"_s, 15000);
    }

    void aNewProjectIsNotReadWhenTheReadMenuIsDismissed() {
        const Env env;
        writeActorsData(env);
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        answerFirstRead(window, false);
        openFromRecent(window, env.project.path());

        QCOMPARE(tabsOf(window)->tabCount(), 0);
        QVERIFY(env.readTranslation(u"actors"_s).isEmpty());
    }

    void aNewProjectCanReuseTheTranslationFolderOfTheGame() {
        const Env env;
        writeActorsData(env);
        forgetProgramFolder(env);
        // What rvpacker-txt-rs leaves in the game's folder
        QVERIFY(QDir().mkpath(env.project.path() + u"/translation"_s));
        writeFile(env.project.path() + u"/translation/actors.txt"_s, "Harold<#>Гарольд\n");
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        catcher.reset();
        const ModalCatcher question(QMessageBox::Yes);
        openFromRecent(window, env.project.path());

        QVERIFY(question.text.contains(u"Translation folder is found"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QCOMPARE(env.readTranslation(u"actors"_s), QByteArray("Harold<#>Гарольд\n"));
    }

    void aFolderWithoutAGameCannotBeOpened() {
        const Env env;
        forgetProgramFolder(env);
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        openFromRecent(window, env.project.path());

        QVERIFY2(catcher->text.contains(u"do not exist"_qsv) && catcher->text.contains(u"archive"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void aFolderThatIsGoneCannotBeOpened() {
        QString gone;

        {
            const Env env;
            gone = env.project.path();
            env.writeSettings([&gone](Settings& settings) -> void {
                settings.core.projectPath.clear();
                settings.core.recentProjects = { gone };
            });

            // The settings outlive the folder, as they do when a project is deleted
            const MainWindow window;
            QVERIFY(QDir(gone).removeRecursively());

            openFromRecent(window, gone);

            QVERIFY2(catcher->text.contains(gone) && catcher->text.contains(u"exist"_qsv), qPrintable(catcher->text));
            catcher->text.clear();
        }
    }

    void theWordsAndCharactersOfTheOpenTabAreCounted() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold the brave<#>one two three\n");
        env.writeSettings([](Settings& settings) -> void { settings.translation.displayWordsAndCharacters = true; });

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);

        // The count is made in the background, after a moment: words and characters, a line break counting for one
        QString text;
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] -> bool {
                for (const QLabel* const candidate : window.findChildren<QLabel*>()) {
                    if (candidate->text().startsWith(u"Source: "_qsv)) {
                        text = candidate->text();
                        return true;
                    }
                }

                return false;
            }(),
            5000
        );
        QVERIFY2(text.contains(u"Source: 3 words"_qsv), qPrintable(text));
        QVERIFY2(text.contains(u"Translation: 3 words"_qsv), qPrintable(text));
        // Characters of the source "Harold the brave" (a line break counting for one) and of the translation "one two three"
        QVERIFY2(text.contains(u"16 chars"_qsv), qPrintable(text));
        QVERIFY2(text.contains(u"13 chars"_qsv), qPrintable(text));
    }
};

QTEST_MAIN(TestMainWindowFlows)
#include "tst_mainwindow_flows.moc"
