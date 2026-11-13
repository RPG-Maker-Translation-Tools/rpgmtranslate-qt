// Batch actions that run a script of the user over the translation files: the approval it needs first, and what is
// done with what it changes, fails or reports.
#include "BatchMenu.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"

#include <QThreadPool>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

constexpr auto TRANSLATION = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый\n";

void prepare(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
    env.writeTranslation(u"actors"_s, TRANSLATION);
    env.writeSettings();
}

void runScript(const MainWindow& window, const QString& source, Selected selected = Selected{ .flags = FileFlags_Actors }) {
    const JSScript script{ .action = BatchAction::Custom, .name = u"Mine"_s, .source = source };

    emit window.findChild<BatchMenu*>()->actionRequested(script, selected, 1, BatchVariant(u8{ 0 }));
}
}  // namespace

class TestMainWindowScripts final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();
        catcher.reset();
    }

    void aScriptThatIsNotApprovedDoesNotRun() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::No);

        const MainWindow window;
        const WindowGuard guard(window);
        runScript(window, u"function rpgmLineCallback(src, tr) { return [tr[0] + '!']; }"_s);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"can run programs"_qsv), 5000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), QByteArray(TRANSLATION));
    }

    void anApprovedScriptRewritesTheFiles() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        runScript(window, u"function rpgmLineCallback(src, tr) { return [tr[0] + '!']; }"_s);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Гарольд!"), 15000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Храбрый!"));
    }

    void theOpenTabIsClosedForTheScriptAndShowsItsResultWhenOpenedAgain() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        runScript(window, u"function rpgmLineCallback(src, tr) { return [tr[0] + '?']; }"_s);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Гарольд?"), 15000);
        // The editor does not hold the file the script wrote
        QCOMPARE(tabsOf(window)->currentTabName(), QString());

        // The file cannot be opened while the task is still finishing
        QTRY_VERIFY_WITH_TIMEOUT(
            [&window]() -> bool {
            tabsOf(window)->changeTab(u"actors"_s);
            return tabsOf(window)->currentTabName() == u"actors"_s;
        }(),
            10000
        );
        QTRY_COMPARE_WITH_TIMEOUT(tableOf(window)->model()->rowCount(), 4, 5000);
        QCOMPARE(*tableOf(window)->model()->item(2, 1), u"Гарольд?"_s);
    }

    // The maps are kept in memory until they are saved: the script reads and writes the section, and the file follows
    void aScriptThatRewritesAMapChangesItsSectionAndSavesMapsTxt() {
        const Env env;
        prepare(env);
        env.writeTranslation(u"maps"_s, "<!>ID<#>1\nHello<#>Salut\n<!>ID<#>2\nBye<#>Пока\n");
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);

        Selected selected;
        selected.mapCount = 2;
        selected.validIndices[1] = true;
        selected.validIndices[2] = true;
        selected.mapIndices = 0b11;

        runScript(window, u"function rpgmLineCallback(src, tr) { return [tr[0] + '!']; }"_s, selected);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"maps"_s).contains("Salut!"), 15000);
        // Each section keeps its last newline, or the next one would start on the line of the previous
        QCOMPARE(
            env.readTranslation(u"maps"_s),
            QByteArray("<!>ID<#>1\nHello<#>Salut!\n<!>ID<#>2\nBye<#>Пока!\n")
        );
        // The actors were not selected
        QCOMPARE(env.readTranslation(u"actors"_s), QByteArray(TRANSLATION));

        QThreadPool::globalInstance()->waitForDone();
        tabsOf(window)->changeTab(u"map1"_s);
        QTRY_COMPARE_WITH_TIMEOUT(tableOf(window)->model()->rowCount(), 2, 5000);
        QCOMPARE(*tableOf(window)->model()->item(1, 1), u"Salut!"_s);
    }

    // The tabs of the files a script rewrote are counted again: a file without a tab has no rows to count, and one that
    // cannot be read is only logged
    void filesWithoutATabOrThatAreGoneAreSkippedWhenTheTabsAreRefreshed() {
        const Env env;
        prepare(env);
        env.writeTranslation(u"items"_s, "Sword<#>Меч\n");
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabCount(), 2);

        // Its tab stays, and the file is gone when the script is to run on it; armors never had either
        QVERIFY(QFile::remove(env.translationFile(u"actors"_s)));

        // Once for the script, which could not open each of them, and once for the refresh, which cannot open actors
        for (i32 count = 0; count < 3; count++) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));
        }

        runScript(
            window,
            u"function rpgmLineCallback(src, tr) { return [tr[0] + '!']; }"_s,
            Selected{ .flags = static_cast<FileFlags>(FileFlags_Actors | FileFlags_Items | FileFlags_Armors) }
        );

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"items"_s).contains("Меч!"), 15000);
        QTRY_VERIFY_WITH_TIMEOUT(
            ranges::any_of(catcher->seen, [](const QString& text) -> bool { return text.contains(u"unable to open"_qsv); }),
            5000
        );
        QThreadPool::globalInstance()->waitForDone();
        // The tab that was rewritten is counted, the others are as they were
        QTRY_COMPARE(tabsOf(window)->tabCount(), 2);
    }

    void aScriptThatFailsIsReportedWithWhereItFailed() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        runScript(window, u"function rpgmLineCallback() { throw new Error('nope'); }"_s);

        QTRY_VERIFY_WITH_TIMEOUT(
            [this]() -> bool {
            for (const QString& text : catcher->seen) {
                if (text.contains(u"nope"_qsv)) {
                    return true;
                }
            }

            return false;
        }(),
            15000
        );
        // Nothing changed before the first line failed, so the file is as it was, last newline included
        QCOMPARE(env.readTranslation(u"actors"_s), QByteArray(TRANSLATION));
    }

    void aScriptThatIsNotValidIsReportedWithoutAFile() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        runScript(window, u"this is not javascript ("_s);

        QTRY_VERIFY_WITH_TIMEOUT(
            [this]() -> bool {
            for (const QString& text : catcher->seen) {
                if (text.contains(u"Script failed"_qsv)) {
                    return true;
                }
            }

            return false;
        }(),
            15000
        );
        QCOMPARE(env.readTranslation(u"actors"_s).trimmed(), QByteArray(TRANSLATION).trimmed());
    }

    void whatAScriptLogsIsShown() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        runScript(window, u"function rpgmLineCallback() { rpgm.log('hello from the script'); return []; }"_s);

        QTRY_VERIFY_WITH_TIMEOUT(
            [this]() -> bool {
            for (const QString& text : catcher->seen) {
                if (text.contains(u"hello from the script"_qsv)) {
                    return true;
                }
            }

            return false;
        }(),
            15000
        );
    }
};

QTEST_MAIN(TestMainWindowScripts)
#include "tst_mainwindow_scripts.moc"
