// The source control dock: it keeps the translation of a project in a git repository next to the project. Driven
// through the window, since the dock is made of the window's widgets.
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"

#ifdef ENABLE_LIBGIT2
#include "GitCommitList.hpp"
#include "GitFileList.hpp"
#include "SourceControlDock.hpp"
#include "ezgit2.hpp"

#include <QComboBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QThreadPool>
#include <QTimer>
#include <QToolButton>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

void prepare(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
    env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
    env.writeSettings([](Settings& settings) -> void {
        settings.core.git.preferGitConfig = false;
        settings.core.git.username = u"Tester"_s;
        settings.core.git.useremail = u"tester@example.com"_s;
    });
}

[[nodiscard]] auto programPath(const Env& env) -> QString {
    return env.project.path() + PROGRAM_DATA_DIRECTORY.toString();
}

[[nodiscard]] auto createButton(const MainWindow& window) -> QPushButton* {
    for (QPushButton* const button : window.findChildren<QPushButton*>()) {
        if (button->text() == u"Create Repository"_qsv) {
            return button;
        }
    }

    return nullptr;
}

// The row of the file in the list of changes, -1 when it is not listed
[[nodiscard]] auto rowOf(const GitFileList* const list, const QString& path) -> i32 {
    const auto& items = list->items();

    for (i32 row = 0; row < items.size(); row++) {
        if (items[row].filePath == path) {
            return row;
        }
    }

    return -1;
}

void stage(GitFileList* const list, const QString& path) {
    const i32 row = rowOf(list, path);
    QVERIFY2(row >= 0, qPrintable(path));
    list->model()->setStaged(row, true);
    emit list->staged(path);
}

void refresh(const MainWindow& window) {
    emit window.findChild<QToolButton*>(u"refreshChangesButton"_s)->pressed();
}

// Stages `actors.txt` and commits it with the message, through the buttons of the dock
void commitActors(const MainWindow& window, const QString& message) {
    auto* const files = window.findChild<GitFileList*>();
    stage(files, u"translation/actors.txt"_s);
    window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(message);
    window.findChild<QPushButton*>(u"commitButton"_s)->click();
}

// The menu of the commit options blocks in `exec`, so the entry is picked from a timer while it is open
void pressCommitOptions(const MainWindow& window, const QString& entry) {
    auto* const timer = new QTimer(qApp);
    timer->setInterval(5);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, entry] -> void {
        auto* const menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());

        if (menu == nullptr) {
            return;
        }

        for (QAction* const action : menu->actions()) {
            if (action->text() == entry) {
                menu->setActiveAction(action);
                QTest::keyClick(menu, Qt::Key_Return);
                timer->deleteLater();
                return;
            }
        }
    });
    timer->start();

    emit window.findChild<QToolButton*>(u"commitOptionsButton"_s)->pressed();
}

void createRepository(const MainWindow& window) {
    QTRY_VERIFY_WITH_TIMEOUT(createButton(window) != nullptr, 10000);
    createButton(window)->click();
    QVERIFY(window.findChild<GitFileList*>()->items().size() > 0);
}
}  // namespace

class TestSourceControl final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void init() { catcher = make_unique<ModalCatcher>(); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();
        QVERIFY2(catcher->text.isEmpty(), qPrintable(catcher->text));
        catcher.reset();
    }

    void aProjectWithoutARepositoryOffersToCreateOne() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        QVERIFY(QFileInfo::exists(programPath(env) + u"/.git"_s));
        // What is a copy of the game, not of the translation, stays out
        const QByteArray ignore = readFile(programPath(env) + u"/.gitignore"_s);
        QVERIFY(ignore.contains("/baseline-data"));
        QVERIFY(ignore.contains("/backups"));

        auto* const files = window.findChild<GitFileList*>();
        QVERIFY(rowOf(files, u"translation/actors.txt"_s) >= 0);
        QCOMPARE(rowOf(files, u"baseline-data/data/Actors.json"_s), -1);
        QVERIFY(createButton(window) == nullptr);

        // Listing the branches of a repository with no commits is not a switch to one of them: that fails, and a
        // message about it must not greet whoever has just created the repository (the fixture fails on any message)
        QVERIFY(!window.findChild<QComboBox*>(u"branchSelect"_s)->currentText().isEmpty());
    }

    void aGitignoreOfTheUserIsNotOverwritten() {
        const Env env;
        prepare(env);
        writeFile(programPath(env) + u"/.gitignore"_s, "mine\n");

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        QCOMPARE(readFile(programPath(env) + u"/.gitignore"_s), QByteArray("mine\n"));
    }

    void stagedFilesAreCommittedWithTheMessageAndTheIdentity() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const files = window.findChild<GitFileList*>();
        auto* const commits = window.findChild<GitCommitList*>();
        stage(files, u"translation/actors.txt"_s);

        window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(u"First translation"_s);
        window.findChild<QPushButton*>(u"commitButton"_s)->click();

        QCOMPARE(commits->model()->rowCount(), 1);
        QCOMPARE(commits->model()->commitAt(0).message, u"First translation"_s);
        QCOMPARE(commits->model()->commitAt(0).author, u"Tester"_s);
        // The committed file is no longer a change
        QCOMPARE(rowOf(files, u"translation/actors.txt"_s), -1);
    }

    void aChangeAfterACommitIsListedOnRefresh() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const files = window.findChild<GitFileList*>();
        stage(files, u"translation/actors.txt"_s);
        window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(u"First"_s);
        window.findChild<QPushButton*>(u"commitButton"_s)->click();

        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n");
        emit window.findChild<QToolButton*>(u"refreshChangesButton"_s)->pressed();

        QVERIFY(rowOf(files, u"translation/actors.txt"_s) >= 0);
        QCOMPARE(files->items().at(rowOf(files, u"translation/actors.txt"_s)).staged, false);
    }

    void theHistoryIsShownWhenTheProjectIsOpenedAgain() {
        const Env env;
        prepare(env);

        {
            const MainWindow window;
            const WindowGuard guard(window);
            createRepository(window);

            stage(window.findChild<GitFileList*>(), u"translation/actors.txt"_s);
            window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(u"Kept"_s);
            window.findChild<QPushButton*>(u"commitButton"_s)->click();
        }

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const commits = window.findChild<GitCommitList*>();

        QTRY_COMPARE_WITH_TIMEOUT(commits->model()->rowCount(), 1, 10000);
        QCOMPARE(commits->model()->commitAt(0).message, u"Kept"_s);
        QVERIFY(window.findChild<QComboBox*>(u"branchSelect"_s)->count() > 0);
        QVERIFY(createButton(window) == nullptr);
    }

    void theTranslationOfTheRepositoryCanBeCopiedBesideTheProgramFolder() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        // The translation is edited in the repository, and this puts it back where the game's tools expect it
        env.writeTranslation(u"actors"_s, "Harold<#>Гарольд\n");
        window.findChild<QPushButton*>(u"copyTranslationButton"_s)->click();

        QCOMPARE(readFile(env.project.path() + u"/translation/actors.txt"_s), QByteArray("Harold<#>Гарольд\n"));
    }

    void aTranslationThatCannotBeCopiedIsReportedInsteadOfEndingTheProgram() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        QVERIFY(QDir(env.translationPath()).removeRecursively());
        window.findChild<QPushButton*>(u"copyTranslationButton"_s)->click();

        QVERIFY2(catcher->text.contains(u"Failed to copy the translation"_qsv), qPrintable(catcher->text));
        catcher->text.clear();

        // Closing the project writes into it
        QVERIFY(QDir().mkpath(env.translationPath()));
    }

    void aBranchThatCannotBeSwitchedToIsReportedWithTheErrorOfTheRepository() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        // A branch the repository does not have: picking it from the list is a request that libgit2 refuses
        auto* const branches = window.findChild<QComboBox*>(u"branchSelect"_s);
        branches->addItem(u"no-such-branch"_s);
        branches->setCurrentIndex(branches->count() - 1);

        // The code is the app's notice, the rest is libgit2's wording: only what it names is checked
        QVERIFY2(catcher->text.contains(u"libgit2 returned error code -3"_qsv) && catcher->text.contains(u"no-such-branch"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
    }

    void unstagingAFileTakesItOutOfTheIndex() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const files = window.findChild<GitFileList*>();
        const QString path = u"translation/actors.txt"_s;

        stage(files, path);
        refresh(window);
        QVERIFY(files->items().at(rowOf(files, path)).status & GIT_STATUS_INDEX_NEW);

        files->model()->setStaged(rowOf(files, path), false);
        emit files->staged(path);
        refresh(window);
        QVERIFY(!(files->items().at(rowOf(files, path)).status & GIT_STATUS_INDEX_NEW));
        QVERIFY(files->items().at(rowOf(files, path)).status & GIT_STATUS_WT_NEW);
    }

    void stagingAPathThatIsNotListedDoesNothing() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const files = window.findChild<GitFileList*>();
        emit files->staged(u"translation/no-such-file.txt"_s);
        refresh(window);

        QCOMPARE(rowOf(files, u"translation/no-such-file.txt"_s), -1);
        const auto status = files->items().at(rowOf(files, u"translation/actors.txt"_s)).status;
        QVERIFY(!(status & GIT_STATUS_INDEX_NEW));
    }

    void aFileThatCannotBeStagedIsReported() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        // The index cannot be written over a directory
        const QString index = programPath(env) + u"/.git/index"_s;
        QFile::remove(index);
        QVERIFY(QDir().mkpath(index + u"/inner"_s));

        stage(window.findChild<GitFileList*>(), u"translation/actors.txt"_s);

        QVERIFY2(catcher->text.contains(u"libgit2 returned error code -1"_qsv) && catcher->text.contains(u"index"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
    }

    void amendingReplacesTheLatestCommitInTheRepositoryAndInTheList() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const commits = window.findChild<GitCommitList*>();
        commitActors(window, u"First"_s);

        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n");
        refresh(window);
        commitActors(window, u"Second"_s);
        QCOMPARE(commits->model()->rowCount(), 2);

        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый\n");
        refresh(window);
        stage(window.findChild<GitFileList*>(), u"translation/actors.txt"_s);
        window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(u"Second, reworded"_s);
        pressCommitOptions(window, u"Commit (Amend)"_s);

        QCOMPARE(commits->model()->rowCount(), 2);
        QCOMPARE(commits->model()->commitAt(0).message, u"Second, reworded"_s);
        QCOMPARE(commits->model()->commitAt(1).message, u"First"_s);

        // The repository agrees: still two commits
        ezgit2::Repository repo;
        QVERIFY(repo.open(programPath(env).toStdString()));
        const auto history = repo.commitHistory();
        QVERIFY(history);
        QCOMPARE(history.value().size(), 2u);
        QCOMPARE(QString::fromUtf8(history.value().front().message).trimmed(), u"Second, reworded"_s);

        // No message keeps the one the commit had
        window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->clear();
        pressCommitOptions(window, u"Commit (Amend)"_s);
        QCOMPARE(commits->model()->rowCount(), 2);
        QCOMPARE(commits->model()->commitAt(0).message, u"Second, reworded"_s);
    }

    void amendingARepositoryWithoutCommitsIsRefusedAndKeepsTheChanges() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        auto* const files = window.findChild<GitFileList*>();
        stage(files, u"translation/actors.txt"_s);
        pressCommitOptions(window, u"Commit (Amend)"_s);

        QVERIFY2(catcher->text.contains(u"libgit2 returned error code -3"_qsv) && catcher->text.contains(u"refs/heads/"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
        QCOMPARE(window.findChild<GitCommitList*>()->model()->rowCount(), 0);
        QVERIFY(rowOf(files, u"translation/actors.txt"_s) >= 0);
    }

    void committingAndPushingWithoutARemoteStillCommits() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        stage(window.findChild<GitFileList*>(), u"translation/actors.txt"_s);
        window.findChild<QPlainTextEdit*>(u"commitMessageInput"_s)->setPlainText(u"Pushed"_s);
        pressCommitOptions(window, u"Commit and push"_s);

        // The commit has succeeded whatever the push did, and the push has no remote to fail loudly against
        auto* const commits = window.findChild<GitCommitList*>();
        QCOMPARE(commits->model()->rowCount(), 1);
        QCOMPARE(commits->model()->commitAt(0).message, u"Pushed"_s);
    }

    void aRepositoryThatVanishesIsReportedOnRefresh() {
        const Env env;
        prepare(env);

        const MainWindow window;
        const WindowGuard guard(window);
        createRepository(window);

        // The list of changes reads HEAD again
        writeFile(programPath(env) + u"/.git/HEAD"_s, "not a reference");
        catcher->text.clear();
        refresh(window);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"libgit2 returned error code -1"_qsv) && catcher->text.contains(u"HEAD"_qsv), 10000);
        catcher->text.clear();
    }

    void aRepositoryThatCannotBeReadIsReported() {
        const Env env;
        prepare(env);
        // A `.git` that is a file with nothing a repository would have in it
        writeFile(programPath(env) + u"/.git"_s, "not a repository");

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(!catcher->text.isEmpty(), 10000);
        QVERIFY2(catcher->text.contains(u"git repository"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
    }
};

QTEST_MAIN(TestSourceControl)
#include "tst_sourcecontrol.moc"
#else
#include <QtTest>

class TestSourceControl final : public QObject {
    Q_OBJECT

   private slots:
    void builtWithoutLibgit2() { QSKIP("the program is built without libgit2"); }
};

QTEST_MAIN(TestSourceControl)
#include "tst_sourcecontrol.moc"
#endif
