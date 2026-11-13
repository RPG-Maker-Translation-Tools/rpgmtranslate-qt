// Searching the translation files and working with the results the way the search panel offers: go to a result,
// replace it, or put the replacement in its place.
#include "Permissions.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"
#include "SearchMenu.hpp"
#include "SearchResultList.hpp"
#include "TaskWorker.hpp"

#include <QPlainTextEdit>
#include <QThreadPool>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

constexpr auto TRANSLATION = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый Гарольд\n";

void prepare(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
    env.writeTranslation(u"actors"_s, TRANSLATION);
    env.writeSettings();
}

void search(const MainWindow& window, const QString& what) {
    Selected selected;
    selected.flags = FileFlags_Actors;

    emit window.findChild<SearchMenu*>()->actionRequested(
        selected,
        SearchAction::Search,
        what,
        QString(),
        SearchLocation::Translation,
        -1,
        static_cast<SearchFlags>(0)
    );
}

[[nodiscard]] auto resultsOf(const MainWindow& window) -> SearchResultList* {
    return window.findChild<SearchResultList*>();
}

void waitForResults(const MainWindow& window, const i32 count) {
    QTRY_COMPARE_WITH_TIMEOUT(resultsOf(window)->proxy()->rowCount(), count, 15000);
}

void click(const MainWindow& window, const Qt::MouseButton button, const i32 row) {
    emit resultsOf(window)->clicked(button, resultsOf(window)->proxy()->index(row, 0));
}

void setReplacement(const MainWindow& window, const QString& text) {
    window.findChild<SearchMenu*>()->findChild<QPlainTextEdit*>(u"replaceInput"_s)->setPlainText(text);
}
}  // namespace

class TestMainWindowSearch final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void init() { catcher = make_unique<ModalCatcher>(); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();
        catcher.reset();
    }

    void aSearchListsEveryCellThatMatches() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);

        // The two cells of the translation column that hold it
        waitForResults(window, 2);
    }

    void aSearchThatFindsNothingListsNothing() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);

        search(window, u"no such text"_s);
        waitForResults(window, 0);
    }

    void aLeftClickOpensTheFileOfTheResult() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        QCOMPARE(tabsOf(window)->currentTabName(), QString());

        click(window, Qt::LeftButton, 0);

        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
    }

    void aRightClickReplacesTheTextOfThatResultInTheFile() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        setReplacement(window, u"Харольд"_s);

        click(window, Qt::RightButton, 0);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Харольд"), 15000);
        // One result was replaced, the other was not
        const QByteArray file = env.readTranslation(u"actors"_s);
        QCOMPARE(file.count("Харольд"), 1);
        QCOMPARE(file.count("Гарольд"), 1);
    }

    void replacingInTheOpenTabClosesItFirstAndKeepsItsEdits() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Новый Гарольд"_s);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        setReplacement(window, u"Харольд"_s);

        click(window, Qt::RightButton, 0);

        // The edit was saved before the file was rewritten, and the replacement went into it
        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Новый Харольд"), 15000);
        // The task wrote the file, so the editor must not hold it any more
        QCOMPARE(tabsOf(window)->currentTabName(), QString());
    }

    void aResultOfACellThatChangedSinceIsRefused() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        setReplacement(window, u"Харольд"_s);

        // Someone edits the file after the search
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Совсем другое\nThe Brave<#>Храбрый\n");

        click(window, Qt::RightButton, 0);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"changed after the search"_qsv), 15000);
        QVERIFY(!env.readTranslation(u"actors"_s).contains("Харольд"));
        catcher->text.clear();
    }

    void aResultOfAFileThatIsGoneSaysItCannotBeOpened() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        setReplacement(window, u"Харольд"_s);

        QVERIFY(QFile::remove(env.translationFile(u"actors"_s)));

        click(window, Qt::RightButton, 0);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"unable to open file"_qsv), 15000);
        catcher->text.clear();
    }

    void aReplacementIsRefusedWhileAConflictingTaskRuns() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Гарольд"_s);
        waitForResults(window, 2);
        setReplacement(window, u"Харольд"_s);

        const auto blocker = window.findChild<TaskWorker*>()->startTask(TaskKind::Write);
        QVERIFY(blocker.has_value());

        click(window, Qt::RightButton, 0);

        QVERIFY(catcher->text.contains(u"Cannot start"_qsv));
        QVERIFY(!env.readTranslation(u"actors"_s).contains("Харольд"));
        catcher->text.clear();

        window.findChild<TaskWorker*>()->finishTask(*blocker);
    }

    void aMiddleClickPutsTheReplacementInPlaceOfTheTranslation() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Храбрый"_s);
        waitForResults(window, 1);
        setReplacement(window, u"Отважный"_s);

        click(window, Qt::MiddleButton, 0);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("The Brave<#>Отважный"), 15000);
        // The whole cell is put in place, not just the part that was found
        QVERIFY(!env.readTranslation(u"actors"_s).contains("Храбрый"));
    }

    void aMiddleClickThatIsDeclinedPutsNothing() {
        const Env env;
        prepare(env);
        catcher = make_unique<ModalCatcher>(QMessageBox::No);

        const MainWindow window;
        const WindowGuard guard(window);
        search(window, u"Храбрый"_s);
        waitForResults(window, 1);
        setReplacement(window, u"Отважный"_s);

        click(window, Qt::MiddleButton, 0);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"put the text"_qsv), 5000);
        QThreadPool::globalInstance()->waitForDone();
        QVERIFY(env.readTranslation(u"actors"_s).contains("Храбрый"));
        catcher->text.clear();
    }

    void aSearchNamesTheFilesItCouldNotOpen() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(QFile::setPermissions(env.translationFile(u"actors"_s), QFileDevice::WriteOwner));

        search(window, u"Гарольд"_s);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"unable to open the following files"_qsv), 15000);
        QVERIFY(catcher->text.contains(u"actors"_qsv));
        QCOMPARE(resultsOf(window)->proxy()->rowCount(), 0);
        catcher->text.clear();

        QVERIFY(QFile::setPermissions(env.translationFile(u"actors"_s), QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }
};

QTEST_MAIN(TestMainWindowSearch)
#include "tst_mainwindow_search.moc"
